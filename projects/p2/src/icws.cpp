#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <poll.h>
#include <unordered_map>
#include "parse.h"
#include "pcsa_net.hpp"
#include "swag_net.hpp"
#include "globals.hpp"


static const std::unordered_map<std::string, std::string> mime_types = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".mp4", "video/mp4"},
    {".mp3", "audio/mpeg"},
    {".ico", "image/x-icon"}
};

// a map for storing the buffer info for each connection
std::unordered_map<int, BufferInfo> buffer_map;

const option long_opts[] =
{
    {"root", required_argument, nullptr, 'r'},
    {"port", required_argument, nullptr, 'p'},
    {nullptr, no_argument, nullptr, 0}
};

const char* root = nullptr;
const char* port = nullptr;

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
        std::string date = RFC1123_DateTimeNow();
        std::string last_modified = date;
        
        // we take the connection parameter from the response
        // if it does not exist, we assume to keep the connection alive
        bool keep_alive = true;
        if(header_name_in_request(request, "Connection")){
            keep_alive = strcmp(request->headers[0].header_value, "close") != 0;
        }
        
        return create_error_response(response_code, response_reason, keep_alive ? "keep-alive" : "close");
}

int serve_http(int socketFd){
    std::cout << "PROCESSING REQUEST" << "\n";
    BufferInfo buffer_info;


    if(buffer_map.count(socketFd) < 0){ // TRUE if socketFd is not in buffer_map
        buffer_info = BufferInfo();
        buffer_map[socketFd] = buffer_info;
    }
    buffer_info = buffer_map[socketFd];

    char buf[BUFSIZE];
    int readBytes;
    std::string line;
    std::string request_string = "";


    // we need to time this while loop for bad requests that never end for some time
    std::chrono::time_point start = std::chrono::steady_clock::now();
    while ((readBytes = read_line_swag(socketFd, buf, BUFSIZE, buffer_info, 8000)) > 0) {
        std::cout << "DEBUG: READ BYTES: " << readBytes << "\n";
        std::cout << "DEBUG: BUFFER: " << buf << "\n";
        line = std::string(buf);
        request_string += line;
        if(line == "\r\n"){
            break;
        }
        // if the while loop doesn't end in 20 seconds, we assume that the request is bad
        if(std::chrono::steady_clock::now() - start > std::chrono::seconds(20)){
            readBytes = -2; // not sure if this is a good idea but whatever...
            break;
        } 
    }
    // error
    if(readBytes == -1){
        std::cout << "DEBUG: ERROR" << '\n';
        Response response = bad_request_response();
        std::string response_string = response_to_string(response);
        std::cout << "DEBUG: RESPONSE STRING ON ERROR:"  << "\n" << response_string << "\n";

        write_all(socketFd, response_string.data(), get_response_size(response));
        return -1;
    }
    // timeout
    if(readBytes == -2){
        std::cout << "DEBUG: REQUEST TIMEOUT" << '\n';
        Response response = timeout_response();

        std::string response_string = response_to_string(response);
        std::cout << "DEBUG: RESPONSE STRING ON TIMEOUT:"  << "\n" << response_string << "\n";
        write_all(socketFd, response_string.data(), get_response_size(response));
        return -2;
    }


    std::cout << "DEBUG: REQUEST STRING:\n" << request_string << "\n";
    Response response;

    ParseResult result = parse(request_string.data(), request_string.size());
    if(!result.success){
        response = error_response(result);
        std::string response_string = response_to_string(response);
        std::cout << "DEBUG: RESPONSE STRING ON SUCCESS:"  << "\n" << response_string << "\n";
        write_all(socketFd, response_string.data(), get_response_size(response));
        return EXIT_SUCCESS;
    }
    
    // we can finally handle our good requests, unless its a 404 or unimplemented :(

    Request* request = result.request;
    std::string method = request->http_method;
    if(method == "GET"){
        response = get_response(*request, root);
    }
    else if(method == "HEAD"){
        response = head_response(*request, root);
    }
    else{
        response = unimplemented_response();
    }


    std::string response_string = response_to_string(response);
    std::cout << "DEBUG: RESPONSE STRING ON SUCCESS:"  << "\n" << response_string << "\n";
    write_all(socketFd, response_string.data(), get_response_size(response));
    std::cout <<"finished write\n";
    return EXIT_SUCCESS;

} 

int start_server(){
    std::cout << "Starting server on port:" << port << " at root directory: " << root << '\n';
    
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

        // TODO: pass work to thread later
        int res = serve_http(connFd);
        close(connFd);
    }
    close(listenFd);
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]){
    if(process_args(argc, argv) == EXIT_FAILURE){
        return EXIT_FAILURE;
    };

    start_server();
    return 0;
}