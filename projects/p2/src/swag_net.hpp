#include <unistd.h>
#include <memory>
#include <mutex>
#include "globals.hpp"

struct BufferInfo{
    char buffer[READBUFCAPACITY + 1];
    size_t buffer_size = 0;
    // the offset of the buffer that has been read
    size_t buffer_offset = 0;
    bool is_closed = false;
};


int read_line_swag(int connFd, char *usrbuf, size_t maxlen, BufferInfo& bufinfo, int timeout);
