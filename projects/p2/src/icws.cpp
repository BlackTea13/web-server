#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "parse.hpp"
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


int process_request(char* buf, int socketFd){

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
        ssize_t len = read_line(connFd, buf, BUFSIZE);
        std::cout << "len: " << len << '\n';
        std::cout << "buf: " << buf << '\n';
        write_all(connFd, buf, len);

        process_request(buf, connFd);
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