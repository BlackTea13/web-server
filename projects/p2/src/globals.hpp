#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <chrono>
#include <cstring>

#define BUFSIZE 1024
#define RFC1123_TIME_LEN 29
#define TIMEOUT_DURATION 5000 // 5 seconds timeout

#define READBUFCAPACITY 8192


static const char* RFC1123_TIME_FMT = "%a, %d %b %Y %H:%M:%S GMT";
static const char* SERVER_VALUE = "Rob's Cool P2 Server B)";