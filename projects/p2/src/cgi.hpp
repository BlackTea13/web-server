#include "httpobjects.hpp"
#include "globals.hpp"

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

void set_cgi_env_variables(Request request);
void set_serverport_scriptname_remoteaddr_cgi_env_variables(std::string server_port, std::string script_name, std::string remote_addr);
void set_default_cgi_env_variables();
    