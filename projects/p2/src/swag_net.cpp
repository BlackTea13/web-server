#include "swag_net.hpp"
#include <poll.h>
#include <chrono>
#include <iostream>
#include <sys/socket.h>

int read_line_swag(int connFd, char *usrbuf, size_t maxlen, BufferInfo& bufinfo, int timeout){
    char c, *bufp = usrbuf;
 
    // check if buffer is still available
    if (bufinfo.buffer_offset < bufinfo.buffer_size){
        // read from the buffer
        size_t n;
        for(n = 0; n < maxlen && bufinfo.buffer_offset < bufinfo.buffer_size; n++){
            c = bufinfo.buffer[bufinfo.buffer_offset++];
            *bufp++ = c;
            if (c == '\n') { n++; break; }
        }
        *bufp = '\0';
        return n;
    }
    
    // check if buffer is exhausted
    else if (bufinfo.buffer_offset >= bufinfo.buffer_size){
        int n = 0;
        memset(bufinfo.buffer, 0, READBUFCAPACITY);

        // read from the socket
        struct pollfd pollfd;
        pollfd.fd = connFd;
        pollfd.events = POLLIN;

        std::chrono::time_point start = std::chrono::steady_clock::now();
        while(n <= 0){
            int pollRet = poll(&pollfd, 1, timeout * 1000);
            if (pollRet == 0) return -2; // timeout
            else if (pollRet < 0) return -1; // error
            else {
                n = read(connFd, bufinfo.buffer, READBUFCAPACITY);
            }

            if (n < 0) return -1; // error
            if(std::chrono::steady_clock::now() - start > std::chrono::seconds(timeout)) return -2; // timeout
        }

        bufinfo.buffer_size = n;
        bufinfo.buffer_offset = 0;
    }
    // now we have a new buffer to read from, luckily we already wrote that code
    return read_line_swag(connFd, usrbuf, maxlen, bufinfo, timeout);
}