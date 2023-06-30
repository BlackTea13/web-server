#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <filesystem>
#include <limits>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "httpobjects.hpp"

#define overwrite 1

Response create_timeout_response(){
    Response response;
    response.response_code = 408;
    response.response_reason = "Request Timeout";
    response.response_body = "<html><h1>408 Request Timeout</h1></html>";

    response.headers = {
        {"Content-Type", "text/html"},
        {"Content-Length", std::to_string(response.response_body.size())},
        {"Connection", "close"},
        {"Server", SERVER_VALUE},
        {"Date", datetime_rfc1123()},
    };

    return response;
}

Response create_bad_request_response(std::string connection_value){
    Response response;
    response.response_code = 400;
    response.response_reason = "Bad Request";
    response.response_body = "<html><h1>400 Bad Request</h1></html>";

    response.headers = {
        {"Content-Type", "text/html"},
        {"Content-Length", std::to_string(response.response_body.size())},
        {"Connection", connection_value},
        {"Server", SERVER_VALUE},
        {"Date", datetime_rfc1123()},
    };

    return response;
}

Response create_not_implemented_response(std::string connection_value){
    Response response;
    response.response_code = 501;
    response.response_reason = "Not Implemented";
    response.response_body = "<html><h1>501 Not Implemented</h1></html>";

    response.headers = {
        {"Content-Type", "text/html"},
        {"Content-Length", std::to_string(response.response_body.size())},
        {"Connection", connection_value},
        {"Server", SERVER_VALUE},
        {"Date", datetime_rfc1123()},
    };

    return response;
}

Response create_http_version_not_supported_response(std::string connection_value){
    Response response;
    response.response_code = 505;
    response.response_reason = "HTTP Version Not Supported";
    response.response_body = "<html><h1>505 HTTP Version Not Supported</h1></html>";

    response.headers = {
        {"Content-Type", "text/html"},
        {"Content-Length", std::to_string(response.response_body.size())},
        {"Connection", connection_value},
        {"Server", SERVER_VALUE},
        {"Date", datetime_rfc1123()},
    };

    return response;
}

std::string create_internal_server_error_response(std::string connection_value){
    Response response;
    response.response_code = 500;
    response.response_reason = "Internal Server Error";
    response.response_body = "<html><h1>500 Internal Server Error</h1></html>";

    response.headers = {
        {"Content-Type", "text/html"},
        {"Content-Length", std::to_string(response.response_body.size())},
        {"Connection", connection_value},
        {"Server", SERVER_VALUE},
        {"Date", datetime_rfc1123()},
    };

    return response_to_string(response);
}


Response create_not_found_response(std::string connection){
    Response response;
    response.response_code = 404;
    response.response_reason = "Not Found";
    response.response_body = "<html><h1>404 Not Found</h1></html>";

    response.headers = {
        {"Content-Type", "text/html"},
        {"Content-Length", std::to_string(response.response_body.size())},
        {"Connection", connection},
        {"Server", SERVER_VALUE},
        {"Date", datetime_rfc1123()},
    };

    return response;
}

std::string get_content_type(std::string filepath){
    std::string content_type = "text/html";
    if(filepath.find(".html") != std::string::npos){
        content_type = "text/html";
    } else if(filepath.find(".jpg") != std::string::npos){
        content_type = "image/jpeg";
    } else if(filepath.find(".png") != std::string::npos){
        content_type = "image/png";
    } else if(filepath.find(".gif") != std::string::npos){
        content_type = "image/gif";
    } else if(filepath.find(".txt") != std::string::npos){
        content_type = "text/plain";
    } else if(filepath.find(".css") != std::string::npos){
        content_type = "text/css";
    } else if(filepath.find(".js") != std::string::npos){
        content_type = "text/javascript";
    }
    return content_type;
}

std::string get_connection_value(Request request){
    std::string connection = "keep-alive";
    for(int i = 0; i < request.header_count; i++){
        if(strcmp(request.headers[i].header_name, "Connection") == 0){
            connection = request.headers[i].header_value;
        }
    }
    return connection;
}


std::string response_to_string(Response response){
    std::string response_string = "";
    response_string += "HTTP/1.1 " + std::to_string(response.response_code) + " " + response.response_reason + "\r\n";
    for (auto header : response.headers){
        response_string += header.header_name + ": " + header.header_value +"\r\n";
    }
    response_string += "\r\n";
    response_string += response.response_body;

    return response_string;
}

std::string datetime_rfc1123(){
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    auto gmtm = std::gmtime(&currentTime);
    char buffer[RFC1123_TIME_LEN + 1];

    std::strftime(buffer, sizeof(buffer), RFC1123_TIME_FMT, gmtm);
    return std::string(buffer);
}

std::string last_modified_rfc1123(int fileDescript) {
    struct stat file_info;
    auto gmtm = gmtime(&file_info.st_mtime);
    char buffer[RFC1123_TIME_LEN + 1];

    strftime(buffer, sizeof(buffer), RFC1123_TIME_FMT, gmtm);
    return std::string(buffer);


}

bool header_name_in_request(Request request, std::string header_name){
    Request_header* headers = request.headers;
    for(int i = 0; i < request.header_count; i++){
        if(strcmp(headers[i].header_name, header_name.c_str()) == 0){
            return true;
        }
    }
    return false;
}

std::string get_header_value(Request request, std::string header_name){
    Request_header* headers = request.headers;
    for(int i = 0; i < request.header_count; i++){
        if(strcmp(headers[i].header_name, header_name.c_str()) == 0){
            return std::string(headers[i].header_value);
        }
    }
    return "";
}

std::vector<Response_header> create_response_header_vec(
    std::string date,
    std::string last_modified,
    std::string content_type,
    std::string content_length,
    std::string connection
){
    std::vector<Response_header> headers;
    headers.push_back(Response_header("Date", date));
    headers.push_back(Response_header("Server", SERVER_VALUE));
    headers.push_back(Response_header("Connection", connection));
    headers.push_back(Response_header("Content-Type", content_type));
    headers.push_back(Response_header("Content-Length", content_length));
    headers.push_back(Response_header("Last-Modified", last_modified));
    return headers;
}

Response create_get_response(Request request, std::string root_dir){
    std::string connection = get_connection_value(request);

    std::string filepath = request.http_uri;
    if(filepath == ""){
        return create_not_found_response(connection);
    }

    std::string content_type = get_content_type(filepath);

    std::string full_filepath = root_dir + filepath;

    std::string body = "";
    
    // check if uri is '/' then we want to serve index.html
    if(filepath == "/"){
        full_filepath = root_dir + "/index.html";
    }

    std::ifstream file(full_filepath, std::ios::binary);
    file.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize length = file.gcount();
    file.clear();
    file.seekg(0, std::ios_base::beg);
    
    if(file.is_open()){
        std::vector<char> buffer (BUFSIZE, 0);
        while(!file.eof()) {
            file.read(buffer.data(), buffer.size());
            std::streamsize s = file.gcount();
            body.append(buffer.data(), s);
        }
        file.close();
    } else {
        return create_not_found_response(connection);
    }

    auto filefd = open(full_filepath.data(), O_RDONLY);
    if(filefd < 0){
        return create_not_found_response(connection);
    }

    // now we can create our response
    Response response;
    response.response_code = 200;
    response.response_reason = "OK";
    response.response_body = body;
    response.headers = create_response_header_vec(
        datetime_rfc1123(),
        last_modified_rfc1123(filefd),
        content_type,
        std::to_string(length),
        connection
    );
    close(filefd);
    return response;
}

Response create_head_response(Request request, std::string root_dir){
    std::string connection = get_connection_value(request);

    std::string filepath = request.http_uri;
    if(filepath == ""){
        Response response = create_not_found_response(connection);
        response.response_body = "";
        return response;
    }

    std::string content_type = get_content_type(filepath);

    std::string full_filepath = root_dir + filepath;

    // check if uri is '/' then we want to serve index.html
    if(filepath == "/"){
        full_filepath = root_dir + "/index.html";
    }

    std::ifstream file(full_filepath, std::ios::binary);
    file.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize length = file.gcount();
    file.clear();
    file.seekg(0, std::ios_base::beg);

    if(file.is_open()){
        file.close();
    } else {
        Response response = create_not_found_response(connection);
        response.response_body = "";
        return response;
    }

    auto filefd = open(full_filepath.data(), O_RDONLY);
    if(filefd < 0){
        Response response = create_not_found_response(connection);
        response.response_body = "";
        return response;
    }

    // now we can create our response
    Response response;
    response.response_code = 200;
    response.response_reason = "OK";
    response.response_body = "";
    response.headers = create_response_header_vec(
        datetime_rfc1123(),
        last_modified_rfc1123(filefd),
        content_type,
        std::to_string(length),
        connection
    );
    return response;
}

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

std::map<std::string, std::string> get_cgi_env_variables(Request request,
                                                        std::string server_port,
                                                        std::string script_name,
                                                        std::string remote_addr){

    std::map<std::string, std::string> env_vars = {};
    for(std::string header : cgi_headers){
        if(header_name_in_request(request, header)){
            std::string header_value = get_header_value(request, header);
            env_vars[cgi_env_variables[header]] = header_value;
        }
    }
    std::string uri = request.http_uri;
    std::string args = uri.substr(uri.find('?') + 1);
    env_vars["QUERY_STRING"] = args;
    env_vars["SERVER_PORT"] = server_port;
    env_vars["SCRIPT_NAME"] = script_name;
    env_vars["REMOTE_ADDR"] = remote_addr;

    return env_vars;
}

void set_default_cgi_env_variables(){
    setenv("GATEWAY_INTERFACE", "CGI/1.1", overwrite);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", overwrite);
    setenv("SERVER_SOFTWARE", SERVER_VALUE, overwrite);
}

cgi_result pipe_cgi_process(std::string cgi_path, std::string body, std::map<std::string, std::string> env_variables){
    int c2pFds[2]; /* Child to parent pipe */
    int p2cFds[2]; /* Parent to child pipe */
    std::string response = "";
    if (pipe(c2pFds) < 0){
        return cgi_result({500, "c2p pip failed.","Internal Server Error"});
    } 
    if (pipe(p2cFds) < 0){
        return cgi_result({500, "p2c pip failed.","Internal Server Error"});
    }

    cgi_result result;
    int pid = fork();

    if (pid < 0) return cgi_result({500, "fork failed.","Internal Server Error"});
    if (pid == 0) { /* Child - set up the conduit & run inferior cmd */

        /* Wire pipe's incoming to child's stdin */
        /* First, close the unused direction. */
        if (close(p2cFds[1]) < 0) return cgi_result({500, "failed to close p2c[1]","Internal Server Error"});
        if (p2cFds[0] != STDIN_FILENO) {
            if (dup2(p2cFds[0], STDIN_FILENO) < 0)
                return cgi_result({500, "dup2 stdin failed.","Internal Server Error"});
            if (close(p2cFds[0]) < 0)
                return cgi_result({500, "close p2c[0] failed.","Internal Server Error"});
        }

        /* Wire child's stdout to pipe's outgoing */
        /* But first, close the unused direction */
        if (close(c2pFds[0]) < 0) return cgi_result({500, "failed to close c2p[0]","Internal Server Error"});
        if (c2pFds[1] != STDOUT_FILENO) {
            if (dup2(c2pFds[1], STDOUT_FILENO) < 0)
                return cgi_result({500, "dup2 stdin failed.","Internal Server Error"});
            if (close(c2pFds[1]) < 0)
                return cgi_result({500, "close pipeFd[0] failed.","Internal Server Error"});
        }

        /* Set environment variables */
        for(auto& [key, value] : env_variables){
            setenv(key.c_str(), value.c_str(), overwrite);
        }

        char* inferiorArgv[] = {cgi_path.data(), NULL};
        if (execvpe(inferiorArgv[0], inferiorArgv, environ) < 0)
            return cgi_result({500, "exec failed.","Internal Server Error"});
    }
    else { /* Parent - send a random message */
        /* Close the write direction in parent's incoming */
        if (close(c2pFds[1]) < 0) return cgi_result({500, "failed to close c2p[1]","Internal Server Error"});

        /* Close the read direction in parent's outgoing */
        if (close(p2cFds[0]) < 0) return cgi_result({500,"failed to close p2c[0]","Internal Server Error"});

        if(body != ""){
            char *message = body.data();
            /* Write a message to the child - replace with write_all as necessary */
            write(p2cFds[1], message, strlen(message));
        }
        /* Close this end, done writing. */
        if (close(p2cFds[1]) < 0) return cgi_result({500,"close p2c[01] failed.","Internal Server Error"});

        char buf[BUFSIZE+1];
        ssize_t numRead;
        /* Begin reading from the child */

        while ((numRead = read(c2pFds[0], buf, BUFSIZE)) > 0) {
            buf[numRead] = '\0'; /* Printing hack; won't work with binary data */
            response += buf;
        }

        /* Close this end, done reading. */
        if (close(c2pFds[0]) < 0) return cgi_result({500,"close c2p[01] failed.","Internal Server Error"});
        /* Wait for child termination & reap */
        int status;

        if (waitpid(pid, &status, 0) < 0) return cgi_result({500,"waitpid failed.","Internal Server Error"});

        // printf("Child exited... parent's terminating as well.\n");
    }
    return cgi_result({200, "OK","OK", response});
}


std::string create_cgi_get_response(Request request, std::string port, std::string cgi_path, std::string remote_addr){
    /*
    * Get path and message to send off to CGI program
    */
    std::string uri = request.http_uri;
    std::string message = request.body;
    std::map<std::string, std::string> env_variables = get_cgi_env_variables(request, port, cgi_path, remote_addr);
    
    cgi_result result = pipe_cgi_process(cgi_path, message, env_variables);
    if(result.error_code == 200){
        return result.response;
    }
    else{
        return create_internal_server_error_response("close");
    }
}

std::string create_cgi_head_response(Request request, std::string port, std::string cgi_path, std::string remote_addr){
    std::string uri = request.http_uri;
    std::string message = request.body;
    std::map<std::string, std::string> env_variables = get_cgi_env_variables(request, port, cgi_path, remote_addr);
    
    cgi_result result = pipe_cgi_process(cgi_path, message, env_variables);
    if(result.error_code == 200){
        return result.response.substr(0, result.response.find("\r\n\r\n") + 4);
    }
    else{
        return create_internal_server_error_response("close");
    }
}

std::string create_cgi_post_response(Request request, std::string port, std::string cgi_path, std::string remote_addr){
    /*
    * Get path and message to send off to CGI program
    */
    std::string uri = request.http_uri;
    std::string message = request.body;
    std::map<std::string, std::string> env_variables = get_cgi_env_variables(request, port, cgi_path, remote_addr);
    
    cgi_result result = pipe_cgi_process(cgi_path, message, env_variables);
    if(result.error_code == 200){
        return result.response;
    }
    else{
        return create_internal_server_error_response("close");
    }
}

