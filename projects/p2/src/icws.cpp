#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <algorithm>
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


// mutex for parse, it is not thread friendly :(
std::mutex parse_mutex;

const option long_opts[] =
{
    {"root", required_argument, nullptr, 'r'},
    {"port", required_argument, nullptr, 'p'},
    {"numThreads", required_argument, nullptr, 'n'},
    {"timeout", required_argument, nullptr, 'h'},
    {"cgiHandler", required_argument, nullptr, 'c'},
    {nullptr, no_argument, nullptr, 0}
};

const char* root = nullptr;
const char* port = nullptr;
const char* cgi_program = nullptr;
int numThreads = 8;
int timeout = 20;

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
                numThreads = std::stoi(optarg);
                break;
            case 'h':
                timeout = std::stoi(optarg);
                break;
            case 'c':
                cgi_program = optarg;
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

int write_response_to_socket(int socketFd, Response response){
    std::string response_string = response_to_string(response);
    char* response_ptr = response_string.data();
    size_t totalBytes = response_string.size();
    size_t bytesWritten = 0;

    while (bytesWritten < totalBytes) {
        size_t bytesRemaining = totalBytes - bytesWritten;
        size_t bytesToWrite = (bytesRemaining < BUFSIZE) ? bytesRemaining : BUFSIZE;

        struct pollfd pfd;
        pfd.fd = socketFd;
        pfd.events = POLLIN;
        int pollStatus = poll(&pfd, 1, 0);

        if (pollStatus) {
            return -1;
        }
        write_all(socketFd, (response_ptr + bytesWritten), bytesToWrite);
        bytesWritten += bytesToWrite;
    }

    return EXIT_SUCCESS;
}

std::string read_body_from_buffer(int socketFd, int content_length, BufferInfo& buffer_info){
    int bytes_read = 0;
    char buf[BUFSIZE]; 
    memset(buf, 0, BUFSIZE);
    std::string body = "";
    std::chrono::time_point start = std::chrono::steady_clock::now();
    while(bytes_read < content_length){
        int bytes_to_read = std::min(BUFSIZE, content_length - bytes_read);
        int bytes_read_now = read_line_swag(socketFd, buf, bytes_to_read, buffer_info, timeout);
        if(bytes_read_now < 0){
            return "";
        }
        body += std::string(buf, bytes_read_now);
        bytes_read += bytes_read_now;

        if(std::chrono::steady_clock::now() - start > std::chrono::seconds(timeout)){
            return "timeout error";
        } 
    }
    return body;
}

void handle_cgi_request(int socketFd, Request request, BufferInfo& buffer_info, std::string host){
    if (strcmp(request.http_method, "GET") == 0){
        std::string response = create_cgi_get_response(request, port, cgi_program, host);
        write_all(socketFd, response.data(), response.size());
        return;
        
    }
    else if (strcmp(request.http_method, "HEAD") == 0){
        std::string response = create_cgi_head_response(request, port, cgi_program, host);
        write_all(socketFd, response.data(), response.size());
        return;
    }
    else if (strcmp(request.http_method, "POST") == 0){
        /*
        * We have to read the buffer for the body of the request,
        * first check if the content-length header is nice, then
        * we can read and process
        */
        if(header_name_in_request(request, "Content-Length")){
            int content_length = 0;
            /*
             * If the content-length header is not a number, we should
             * return a bad request response 
            */
            try{
                content_length = std::stoi(get_header_value(request, "Content-Length"));
            }
            catch(...){
                write_response_to_socket(socketFd, create_bad_request_response("close"));
                return;
            }

            /*
             * Read body from buffer and handle errors 
            */
            std::string body = read_body_from_buffer(socketFd, content_length, buffer_info);
            if(body == "" && content_length != 0){
                write_response_to_socket(socketFd, create_bad_request_response("close"));
                return;
            }
            else if(body == "timeout error"){
                write_response_to_socket(socketFd, create_timeout_response());
                return;
            }
            else{ 
                request.body = body;
            }

            /*
             *  Now we can process the request 
            */  
            std::string response = create_cgi_post_response(request, port, cgi_program, host);
            write_all(socketFd, response.data(), response.size());
            return;
        }
        else {
            write_response_to_socket(socketFd, create_bad_request_response("close"));
            return;
        }
    }
    else {
        write_response_to_socket(socketFd, create_not_implemented_response("close"));
        return;
    }
}


void serve_http(int socketFd, std::string host){
    // a map for storing the buffer info for each connection
    std::unordered_map<int, std::unique_ptr<BufferInfo>> buffer_map;

    if(buffer_map.count(socketFd) <= 0){ 
        buffer_map[socketFd] = std::make_unique<BufferInfo>();
        memset(buffer_map[socketFd]->buffer, 0, READBUFCAPACITY + 1);
    }
    std::unique_ptr<BufferInfo>& buffer_info = buffer_map[socketFd];

    bool keep_alive = true;
    while(keep_alive){
        /*
        * We read from the socket until we get a \r\n\r\n,
        * we time the while loop and if it takes longer than the timeout
        * we respond with a timeout error, 
        * 
        * Secretly, we know there is another timeout check in read_line_swag ;)
        */
        int readBytes;
        std::string request_string;
        char buf[BUFSIZE];
        std::chrono::time_point start = std::chrono::steady_clock::now();
        while((readBytes = read_line_swag(socketFd, buf, BUFSIZE, *buffer_info, timeout)) >= 0){
            request_string += buf;
            if(request_string.find("\r\n\r\n") != std::string::npos){
                break;
            }

            if(std::chrono::steady_clock::now() - start > std::chrono::seconds(timeout)){
                readBytes = -2; // timeout condition
                break;
            } 
        }

        /*
        * Handle Timeout
        */
        if(readBytes == -2){
            write_response_to_socket(socketFd, create_timeout_response());
            keep_alive = false;
            break;
        }
        /*
        * Error occured, we'll just respond with a bad request
        * we assume to keep the connection alive
        */
        else if(readBytes < 0){
            write_response_to_socket(socketFd, create_bad_request_response(std::string("keep-alive")));
            continue;
        }

        /*
        * We can now parse the request and handle errors from there
        */
        parse_mutex.lock();
        ParseResult parse_result = parse(request_string.data(), request_string.size());
        parse_mutex.unlock();

        bool parse_success = parse_result.success;

        /*
        * Check if the connection is still alive, we don't
        * want to write to a closed socket
        */
        struct pollfd pfd;
        pfd.fd = socketFd;
        pfd.events = POLLIN;
        int pollStatus = poll(&pfd, 1, 0);

        if (pollStatus) {
            free(parse_result.request->headers);
            free(parse_result.request);
            keep_alive = false;
            break;
        }


        /*
        * Parser did not like the request, so bad!
        * since the request is not in the correct format,
        * we assume to keep the connection alive
        */
        if(!parse_success){
            if (parse_result.response_reason == "Bad Request"){
                write_response_to_socket(socketFd, create_bad_request_response(std::string("keep-alive")));
            }
            else if (parse_result.response_reason == "HTTP Version Not Supported"){
                write_response_to_socket(socketFd, create_http_version_not_supported_response(std::string("keep-alive")));
            }
            else{
                write_response_to_socket(socketFd, create_bad_request_response(std::string("keep-alive")));
            }
            continue;
        }

        /*
        * Parser likes our request if we get here!
        */
        Request* request = parse_result.request;

        std::string connection_value = get_connection_value(*request);
        if(connection_value == "close"){
           keep_alive = false;
        }

        /*
         * We can now handle the request, check if its a cgi request
        */
        std::string request_uri = request->http_uri;
        if(request_uri.find("/cgi/") != std::string::npos){
            handle_cgi_request(socketFd, *request, *buffer_info, host);
            free(request->headers);
            free(request);
            keep_alive = false;
            break;
        }

        std::string request_method = request->http_method;
        //std::cout << "writing to socket: " << socketFd << " on thread: " << std::this_thread::get_id() << '\n';
        if(request_method == "GET"){
            write_response_to_socket(socketFd, create_get_response(*request, root));
        }
        else if(request_method == "HEAD"){
            write_response_to_socket(socketFd, create_head_response(*request, root));
        }
        /*
         * We don't support this method, respond with unimplemented method
        */
        else {
            write_response_to_socket(socketFd, create_not_implemented_response(connection_value));
        }
        free(request->headers);
        free(request);
    }   
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
        memset(hostBuf, 0, BUFSIZE);
        if (getnameinfo((sockaddr *) &clientAddr, clientLen, hostBuf, BUFSIZE, svcBuf, BUFSIZE, 0) == 0) 
            std::cout << "Connection from " << hostBuf << ":" << svcBuf << "\n";
        else 
           //std::cout << "Connection from ?unknown?" << "\n";

        if(connFd < 0){
            std::cerr << "Error: accept failed" << '\n';
            continue;
        }
        
        std::string host = hostBuf;
        std::function f = [=](){
            serve_http(connFd, host);
            close(connFd);
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

    set_default_cgi_env_variables();
    start_server();
    return 0;
}