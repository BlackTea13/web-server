#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <poll.h>
#include <unordered_map>
#include <fcntl.h>
#include <functional>
#include "parse.h"
#include "pcsa_net.hpp"
#include "swag_net.hpp"
#include "threadpool.hpp"
#include "globals.hpp"


// a map for storing the buffer info for each connection
std::unordered_map<int, BufferInfo> buffer_map;

// mutex for parse, it is not thread friendly :(
std::mutex parse_mutex;

const option long_opts[] =
{
    {"root", required_argument, nullptr, 'r'},
    {"port", required_argument, nullptr, 'p'},
    {"numThreads", optional_argument, nullptr, 'n'},
    {"timeout", optional_argument, nullptr, 'h'},
    {nullptr, no_argument, nullptr, 0}
};

const char* root = nullptr;
const char* port = nullptr;
int numThreads = 4;
int timeout = 100;

int process_args(int argc, char* argv[]){
    int opt;
    const char* short_opts = "";
    while((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1){
        switch(opt){
            case 'r':
                root = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'n':
                numThreads = atoi(optarg);
                break;
            case 'h':
                timeout = atoi(optarg);
                break;
            default:
                std::cerr << "Error: Invalid Option" << '\n';
                return EXIT_FAILURE;
        }
    }

    if(root == nullptr || port == nullptr){
        std::cerr << "Error: Missing required args" << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

Response error_response(ParseResult result){
        int response_code = result.response_code;
        std::string response_reason = result.response_reason;
        Request* request = result.request;
        std::string date = datetime_rfc1123();
        std::string last_modified = date;
        
        // we take the connection parameter from the response
        // if it does not exist, we assume to keep the connection alive
        bool keep_alive = true;
        if(header_name_in_request(request, "Connection")){
            keep_alive = strcmp(request->headers[0].header_value, "close") != 0;
        }
        
        return create_error_response(response_code, response_reason, keep_alive ? "keep-alive" : "close");
}

int write_response_to_socket(int socketFd, Response response){
    // send headers first
    std::string response_string = response_to_string(response);
    char* response_ptr = response_string.data();
    size_t totalBytes = response_string.size();
    size_t bytesWritten = 0;

    while (bytesWritten < totalBytes) {
        size_t bytesRemaining = totalBytes - bytesWritten;
        size_t bytesToWrite = (bytesRemaining < BUFSIZE) ? bytesRemaining : BUFSIZE;

        write_all(socketFd, (response_ptr + bytesWritten), bytesToWrite);

        bytesWritten += bytesToWrite;
    }

    return EXIT_SUCCESS;
}


void serve_http(int socketFd){
    BufferInfo buffer_info;


    if(buffer_map.count(socketFd) <= 0){ 
        buffer_info = BufferInfo();
        buffer_map[socketFd] = buffer_info;
    }
    buffer_info = buffer_map[socketFd];

    char buf[BUFSIZE];
    int readBytes;
    std::string line;
    std::string request_string = "";

    // we need to time this while loop for bad requests that never end
    std::chrono::time_point start = std::chrono::steady_clock::now();
    while ((readBytes = read_line_swag(socketFd, buf, BUFSIZE, buffer_info, timeout * 1000)) > 0) {
        line = std::string(buf);
        request_string += line;
        if(line == "\r\n"){
            break;
        }
        // if the while loop doesn't end in 20 seconds, we assume that the request is bad
        if(std::chrono::steady_clock::now() - start > std::chrono::seconds(timeout)){
            readBytes = -2; // not sure if this is a good idea but whatever...
            break;
        } 
    }
    // error
    if(readBytes == -1){
        std::cout << "DEBUG: ERROR 123" << '\n';
        Response response = bad_request_response();
        std::string response_string = response_to_string(response);
        std::cout << "DEBUG: RESPONSE STRING ON ERROR:"  << "\n" << response_string << "\n";

        write_all(socketFd, response_string.data(), get_response_size(response));
        std::this_thread::sleep_for(std::chrono::seconds(1000));
        serve_http(socketFd);
    }
    // timeout
    if(readBytes == -2){
        std::cout << "DEBUG: REQUEST TIMEOUT" << '\n';
        Response response = timeout_response();

        std::string response_string = response_to_string(response);
        std::cout << "DEBUG: RESPONSE STRING ON TIMEOUT:"  << "\n" << response_string << "\n";
        write_all(socketFd, response_string.data(), get_response_size(response));
        close(socketFd);
        return;
    }
    


    std::cout << "DEBUG: REQUEST STRING:\n" << request_string << "\n";
    Response response;

    parse_mutex.lock();
    ParseResult result = parse(request_string.data(), request_string.size());
    parse_mutex.unlock();

    if(!result.success){
        response = error_response(result);
        std::string response_string = response_to_string(response);
        std::cout << "DEBUG: RESPONSE STRING ON SUCCESS ON THREAD:" << std::this_thread::get_id() << "\n" << response_string << "\n";
        write_all(socketFd, response_string.data(), get_response_size(response));
        serve_http(socketFd);
    }
    
    // we can finally handle our good requests, unless its a 404 or unimplemented :(
    // 404 is handled in the GET and HEAD functions
    Request* request = result.request;
    std::string method = request->http_method;
    if(method == "GET"){
        response = get_response(*request, root);
    }
    else if(method == "HEAD"){
        response = head_response(*request, root);
        response.response_body = "";
    }
    else{
        response = unimplemented_response();
    }

    std::string connection_value = get_connection_value(*request);

    std::string response_string = response_headers_to_string(response);
    write_response_to_socket(socketFd, response);
    std::cout << "DEBUG: RESPONSE STRING ON SUCCESS ON THREAD:" << std::this_thread::get_id() << "\n";
    
    if(connection_value == "close"){
        close(socketFd);
        return;
    }
    serve_http(socketFd);
} 

int start_server(){
    std::cout << "Starting server on port:" << port << " at root directory: " << root << '\n';
    ThreadPool pool;

    pool.start(numThreads);


    int listenFd = open_listenfd(port);
    int connFd;
    while(1){
        struct sockaddr_storage clientAddr; // to store addr of the client
        socklen_t clientLen = sizeof(struct sockaddr_storage); // size of the above

        // block until connection with client
        connFd = accept(listenFd, NULL, NULL);
        char hostBuf[BUFSIZE], svcBuf[BUFSIZE];
        if (getnameinfo((sockaddr *) &clientAddr, clientLen, hostBuf, BUFSIZE, svcBuf, BUFSIZE, 0) == 0) 
            std::cout << "Connection from " << hostBuf << ":" << svcBuf << "\n";
        else 
            std::cout << "Connection from ?unknown?" << "\n";

        if(connFd < 0){
            std::cerr << "Error: accept failed" << '\n';
            continue;
        }

        std::function f = [&](){
            serve_http(connFd);
        };

        pool.add_job(f);
    }
    close(listenFd);
    pool.stop();
    return EXIT_SUCCESS;
}


int main(int argc, char* argv[]){
    if(process_args(argc, argv) == EXIT_FAILURE){
        return EXIT_FAILURE;
    };

    start_server();
    return 0;
}