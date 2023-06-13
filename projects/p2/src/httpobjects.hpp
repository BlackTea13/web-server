#pragma once//Header field
#include <vector>
#include <string>
#include <unordered_set>
#include "globals.hpp"

//Enum ResponseCodes
enum class ResponseCode {
	OK = 200,
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	REQUEST_TIMEOUT = 408,	
	NOT_IMPLEMENTED = 501,
	HTTP_VERSION_NOT_SUPPORTED = 505
};

const std::unordered_set<std::string> response_headers = {
    "Date",
    "Server",
    "Connection",
    "Content-Type",
    "Content-Length",
    "Last-Modified"  
};

//HTTP Request Header
typedef struct
{
	char header_name[4096];
	char header_value[4096];
} Request_header;

//HTTP Request
typedef struct
{
	char http_version[50];
	char http_method[50];
	char http_uri[4096];
	Request_header *headers;
	int header_count;
} Request;

//HTTP Response Header
struct Response_header {
    std::string header_name;
    std::string header_value;
};

//HTTP Response 
struct Response {
    int response_code;
    std::string response_reason;
    std::vector<Response_header> headers;
    std::string response_body;

    // Constructor
    Response(int code, std::string reason, std::vector<Response_header> headers, std::string body) : 
        response_code(code), response_reason(reason), headers(headers), response_body(body) {}

};


// HTTP Response functions
std::string response_to_string(Response response);
std::vector<std::string> request_headers_to_vec(Request request);