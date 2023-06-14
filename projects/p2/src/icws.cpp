#include <iostream>
#include <sstream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <poll.h>
#include "parse.h"
#include "pcsa_net.hpp"
#include "globals.hpp"


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

Response create_error_response(ParseResult result){
        int response_code = result.response_code;
        std::string response_reason = result.response_reason;
        Request* request = result.request;
        Response response = Response(response_code, response_reason, std::vector<Response_header>(), "");
        std::string date = RFC1123_DateTimeNow();
        std::string last_modified = date;
        
        std::cout << "VARIABLES INITIALIZED...." << "\n";

        // we take the connection parameter from the response
        // if it does not exist, we assume to keep the connection alive
        bool keep_alive = true;
        if(header_name_in_request(request, "Connection")){
            keep_alive = strcmp(request->headers[0].header_value, "close") != 0;
        }
        std::string content_type = "text/html";
        std::ostringstream bodystream;
        bodystream << "<html><h1> " << response_code << " " << response_reason << "</h1></html>"; 
        std::string body = bodystream.str();
        int content_length = body.length();

        std::cout << "ADDING HEADERS...." << '\n';
        
        response.headers.push_back(Response_header("Date", date));
        response.headers.push_back(Response_header("Server", SERVER_VALUE));
        response.headers.push_back(Response_header("Connection", keep_alive ? "keep-alive" : "close"));
        response.headers.push_back(Response_header("Content-Type", content_type));
        response.headers.push_back(Response_header("Content-Length", std::to_string(content_length)));
        response.headers.push_back(Response_header("Last-Modified", last_modified));
        
        return response;
}


Response process_request(char* buf, int socketFd){
    ParseResult result = parse(buf, strlen(buf));

    if(!result.success){
        return create_error_response(result);
    }

    return Response();
} 

int write_response_to_socket(Response response, int socketFd){

    std::string response_string = response_to_string(response);
    int response_size = get_response_size(response);
    std::cout << "TEST====== \n" << response_string << '\n';
    char* response_buf = response_string.data();
    write_all(socketFd, response_buf, response_size);
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
        char buf[BUFSIZE];
        ssize_t len;

        // TODO: THIS IS BLOCKING, BAD REQUESTS NEVER FINISH
        while ((len = read(connFd, buf, BUFSIZE) > 0)) {
            if (strstr(buf, "\r\n\r\n") != NULL) break;
        }

        std::cout << "len: " << len << '\n';
        std::cout << "buf: " << buf << '\n';
        write_all(connFd, buf, len);

        std::cout << "START PROCESS" << "\n";
        Response response = process_request(buf, connFd);

        std::cout << "DEBUG: finished processing" << "\n";
        write_response_to_socket(response, connFd);
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