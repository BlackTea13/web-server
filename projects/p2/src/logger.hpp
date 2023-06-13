#pragma once
#include <iostream>
#include <string.h>
#include "httpobjects.hpp"

inline void log_connection(char* host, char* ident, char* authuser, char* date, char* request, char* status, char* bytes){
    std::cout << host << " " << ident << " " << authuser << " " << date << " " << request << " " << status << " " << bytes << '\n';
}
