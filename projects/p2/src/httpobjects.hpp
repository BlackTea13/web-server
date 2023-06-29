#pragma once
#include "globals.hpp"
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <iostream>

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
class Request {
	public:
		char http_version[50];
		char http_method[50];
		char http_uri[4096];
		Request_header *headers;
		int header_count;
		std::string body;

};

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
bool header_name_in_request(Request request, std::string header_name);
std::string get_header_value(Request request, std::string header_name);

std::string datetime_rfc1123();
std::string get_connection_value(Request request);

Response create_get_response(Request request, std::string root_dir);
Response create_head_response(Request request, std::string root_dir);

Response create_timeout_response();
Response create_not_found_response(std::string connection);
Response create_bad_request_response( std::string connection_value);
Response create_not_implemented_response(std::string connection_value);
Response create_http_version_not_supported_response(std::string connection_value);

Response create_cgi_get_response(Request request, std::string cgi_path);
Response create_cgi_head_response(Request request, std::string cgi_path);
std::string create_cgi_post_response(Request request, std::string port, std::string cgi_path, std::string remote_addr);

/*
    We only consider the following headers:
    - CONTENT_LENGTH
    - CONTENT_TYPE
    - HTTP_ACCEPT
    - HTTP_REFERER
    - HTTP_ACCEPT_ENCODING
    - HTTP_ACCEPT_LANGUAGE
    - HTTP_ACCEPT_CHARSET
    - HTTP_COOKIE
    - HTTP_USER_AGENT
    - HTTP_CONNECTION
*/

struct cgi_result {
    int error_code;
    std::string reason;
    std::string http_reason;
	std::string response;
};

std::map<std::string, std::string> get_cgi_env_variables(Request request,
                                                        std::string server_port,
                                                        std::string script_name,
                                                        std::string remote_addr);
void set_default_cgi_env_variables();
cgi_result pipe_cgi_process(std::string cgi_path, std::string body, std::map<std::string, std::string> env_variables);
