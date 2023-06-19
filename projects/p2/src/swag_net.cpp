#include "swag_net.hpp"
#include <poll.h>
#include <iostream>

int read_line_swag(int connFd, char *usrbuf, size_t maxlen, BufferInfo& bufinfo, float timeout){
    auto& [buffer, buffer_size, buffer_offset, isClosed] = bufinfo;
    char c, *bufp = usrbuf;

    int r = 0;
 
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
        int n;
        memset(buffer, 0, READBUFCAPACITY);

        // read from the socket
        struct pollfd pollfd;
        pollfd.fd = connFd;
        pollfd.events = POLLIN;
        int pollRet = poll(&pollfd, 1, timeout);
        if (pollRet == 0) return -2; // timeout
        else if (pollRet < 0) return -1; // error
        else {
            // socket is ready
            n = read(connFd, bufinfo.buffer, READBUFCAPACITY);
        }
        if (n == 0){
            return -1;
        }
        if (n < 0) return -1;

        bufinfo.buffer_size = n;
        // reset the buffer offset
        bufinfo.buffer_offset = 0;
    }
    
    // now we have a new buffer to read from, luckily we already wrote that code
    return read_line_swag(connFd, usrbuf, maxlen, bufinfo, timeout);
}