#pragma once
#include "globals.hpp"
#include <sstream>
#include <vector>
#include <unordered_map>

//Enum ResponseCodes
enum class ResponseCode {
	OK = 200,
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	REQUEST_TIMEOUT = 408,	
	NOT_IMPLEMENTED = 501,
	HTTP_VERSION_NOT_SUPPORTED = 505
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

/**
 * HTTP Response Header
 * In this project we only need to support a subset of the headers
 * - Date
 * - Server
 * - Connection
 * - Content-Type
 * - Content-Length
 * - Last-Modified
*/
struct Response_header {
    std::string header_name;
    std::string header_value;

    Response_header(std::string name, std::string value) : header_name(name), header_value(value) {}
};

//HTTP Response 
struct Response {
    int response_code;
    std::string response_reason;
    std::vector<Response_header> headers;
    std::string response_body;

    // Default Constructor
    Response() : response_code(0), response_reason(""), headers(std::vector<Response_header>()), response_body("") {}

    // Constructor
    Response(int code, std::string reason, std::vector<Response_header> headers, std::string body) : 
        response_code(code), response_reason(reason), headers(headers), response_body(body) {}


};

// Default values for connection header for different respones
static std::unordered_map<int, std::string> connection_header = {
	{200, "keep-alive"},
	{400, "keep-alive"},
	{404, "close"},
	{408, "keep-alive"},
	{501, "keep-alive"},
	{505, "keep-alive"}
};



// HTTP Response functions
std::string response_to_string(Response response);
std::vector<std::string> request_headers_to_vec(Request request);
bool header_name_in_request(Request* request, std::string header_name);
int get_response_size(Response response);
std::string datetime_rfc1123();

Response timeout_response();
Response bad_request_response();
Response unimplemented_response();
Response get_response(Request request, std::string root_dir);
Response head_response(Request request, std::string root_dir);

Response create_error_response(int code, std::string reason, std::string connection);
Response create_good_response(std::string connection, std::string body, std::string content_type);
