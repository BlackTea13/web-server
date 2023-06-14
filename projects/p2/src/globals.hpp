#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <chrono>
#include <cstring>

#define BUFSIZE 1024
#define BUFSIZE_REQUEST_LINE 4196
#define BUFSIZE_HEADER_NAME 4096
#define BUFSIZE_HEADER_VALUE 4096
#define BUFSIZE_URI 4096
#define BUFSIZE_METHOD 50
#define BUFSIZE_VERSION 50
#define BUFSIZE_BODY 4096
#define BUFSIZE_RESPONSE_REASON 50
#define RFC1123_TIME_LEN 29
#define TIMEOUT_DURATION 5000 // 5 seconds timeout

#define READBUFCAPACITY 8192


static const char* RFC1123_TIME_FMT = "%a, %d %b %Y %H:%M:%S GMT";
static const char* SERVER_VALUE = "Rob's Cool P2 Server B)";