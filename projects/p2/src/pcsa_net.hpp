#ifndef __PCSA_NET_
#define __PCSA_NET_
#include <unistd.h>

int open_listenfd(const char *port);
int open_clientfd(const char *hostname, const char *port);
ssize_t read_line(int connFd, char *usrbuf, size_t maxlen);
void write_all(int connFd, char *buf, size_t len);

#endif
