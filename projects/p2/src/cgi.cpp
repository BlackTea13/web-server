#include <map>
#include "cgi.hpp"

# define overwrite 1

std::vector<std::string> cgi_headers = {
    "HTTP_ACCEPT",
    "HTTP_ACCEPT_ENCODING",
    "HTTP_ACCEPT_LANGUAGE",
    "HTTP_ACCEPT_CHARSET",
    "HTTP_COOKIE",
    "HTTP_USER_AGENT",
    "HTTP_CONNECTION",
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "HTTP_REFERER",
};

// a map of the env variables above to their corresponding request headers
std::map<std::string, std::string> cgi_env_variables = {
    {"HTTP_ACCEPT", "Accept"},
    {"HTTP_ACCEPT_ENCODING", "Accept-Encoding"},
    {"HTTP_ACCEPT_LANGUAGE", "Accept-Language"},
    {"HTTP_ACCEPT_CHARSET", "Accept-Charset"},
    {"HTTP_COOKIE", "Cookie"},
    {"HTTP_USER_AGENT", "User-Agent"},
    {"HTTP_CONNECTION", "Connection"},
    {"CONTENT_LENGTH", "Content-Length"},
    {"CONTENT_TYPE", "Content-Type"},
    {"HTTP_REFERER", "Referer"},
};

void set_cgi_env_variables(Request request, std::string server_port, std::string script_name, std::string remote_addr){
    for(std::string header : cgi_headers){
        if(header_name_in_request(request, header)){
            std::string header_value = get_header_value(request, header);
            setenv(cgi_env_variables[header].data(), header_value.data(), overwrite);
        }
    }
}


void set_serverport_scriptname_remoteaddr_cgi_env_variables(std::string server_port, std::string script_name, std::string remote_addr){
    setenv("SERVER_PORT", server_port.data(), overwrite);
    setenv("SCRIPT_NAME", script_name.data(), overwrite);
    setenv("REMOTE_ADDR", remote_addr.data(), overwrite);
}

void set_default_cgi_env_variables(){
    setenv("GATEWAY_INTERFACE", "CGI/1.1", overwrite);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", overwrite);
    setenv("SERVER_SOFTWARE", SERVER_VALUE, overwrite);
}
