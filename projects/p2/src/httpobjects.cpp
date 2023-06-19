#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <filesystem>
#include <limits>
#include "httpobjects.hpp"



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
        response_string += header.header_name + ": " + header.header_value + "\r\n";
    }
    response_string += "\r\n";
    response_string += response.response_body;

    return response_string;
}

std::string response_headers_to_string(Response response){
    std::string response_string = "";
    response_string += "HTTP/1.1 " + std::to_string(response.response_code) + " " + response.response_reason + "\r\n";
    for (auto header : response.headers){
        response_string += header.header_name + ": " + header.header_value + "\r\n";
    }
    response_string += "\r\n";
    return response_string;
}


std::string datetime_rfc1123(){
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* gmtm = std::gmtime(&currentTime);

    char buffer[RFC1123_TIME_LEN + 1];

    std::strftime(buffer, sizeof(buffer), RFC1123_TIME_FMT, gmtm);
    return std::string(buffer);
}

bool header_name_in_request(Request* request, std::string header_name){
    Request_header* headers = request->headers;
    for(int i = 0; i < request->header_count; i++){
        if(strcmp(headers[i].header_name, header_name.c_str()) == 0){
            return true;
        }
    }
    return false;
}

int get_response_size(Response response){
    int total = 0;
    for(auto header : response.headers){
        total += header.header_name.size() + header.header_value.size();
    }
    total += response.response_reason.size();
    total += std::to_string(response.response_code).size();
    return total;
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


Response bad_request_response(){
    Response response = Response(
        400,
        "Bad Request",
        std::vector<Response_header>(),
        "<html><body><h1>400 Bad Request</h1></body></html>"
    );

    std::string curDateTime = datetime_rfc1123();
    response.headers = create_response_header_vec(
        curDateTime,
        curDateTime,
        "text/html",
        std::to_string(response.response_body.size()),
        "keep-alive"
    );
    return response;
}

Response unimplemented_response(){
        Response response = Response(
        501,
        "Not Implemented",
        std::vector<Response_header>(),
        "<html><body><h1>501 Not Implemented</h1></body></html>"
    );

    std::string curDateTime = datetime_rfc1123();
    response.headers = create_response_header_vec(
        curDateTime,
        curDateTime,
        "text/html",
        std::to_string(response.response_body.size()),
        "keep-alive"
    );
    
    return response;
}

Response timeout_response(){
    Response response = Response(
        408,
        "Request Timeout",
        std::vector<Response_header>(),
        "<html><body><h1>408 Request Timeout</h1></body></html>"
    );
    std::string curDateTime = datetime_rfc1123();
    response.headers = create_response_header_vec(
        curDateTime,
        curDateTime,
        "text/html",
        std::to_string(response.response_body.size()),
        "keep-alive"
    );
    return response;
}

Response create_error_response(int code, std::string reason, std::string connection){
    std::string curDateTime = datetime_rfc1123();

    std::ostringstream bodystream;
    bodystream << "<html><h1> " << code << " " << reason << "</h1></html>"; 
    std::string body = bodystream.str();

    std::vector<Response_header> new_headers = create_response_header_vec(
        curDateTime,
        curDateTime,
        "text/html",
        std::to_string(body.size()),
        connection
    );

    return Response(code, reason, new_headers, body);
}

Response get_response(Request request, std::string root_dir){
    std::string connection = "keep-alive";
    if(header_name_in_request(&request, "Connection")){
        for(int i = 0; i < request.header_count; i++){
            if(strcmp(request.headers[i].header_name, "Connection") == 0){
                connection = request.headers[i].header_value;
            }
        }
    }

    std::string filepath = request.http_uri;
    if(filepath == ""){
        return create_error_response(404, "Not Found", connection);
    }

    std::string content_type = get_content_type(filepath);

    std::string full_filepath = root_dir + filepath;
    std::cout << "filepath " << full_filepath << std::endl;

    std::string body = "";
    
    std::filesystem::path cwd = std::filesystem::current_path() / full_filepath;
    std::cout << "pwd:" << std::filesystem::current_path() << '\n';

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
    } else {
        return create_error_response(404, "Not Found", connection);
    }


    // now we can create our response
    Response response;
    response.response_code = 200;
    response.response_reason = "OK";
    response.response_body = body;
    response.headers = create_response_header_vec(
        datetime_rfc1123(),
        datetime_rfc1123(),
        content_type,
        std::to_string(length),
        connection
    );
    return response;
}

Response head_response(Request request, std::string root_dir){
    std::string connection = "keep-alive";
    if(header_name_in_request(&request, "Connection")){
        for(int i = 0; i < request.header_count; i++){
            if(strcmp(request.headers[i].header_name, "Connection") == 0){
                connection = request.headers[i].header_value;
            }
        }
    }

    std::string filepath = request.http_uri;
    if(filepath == ""){
        return create_error_response(404, "Not Found", connection);
    }

    std::string content_type = get_content_type(filepath);

    std::string full_filepath = root_dir + filepath;
    std::cout << "filepath " << full_filepath << std::endl;

    std::string body = "";
    std::cout << "pwd:" << std::filesystem::current_path() << '\n';

    std::ifstream file(full_filepath, std::ios::binary);
    file.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize length = file.gcount();
    file.clear();
    file.seekg(0, std::ios_base::beg);

    if(file.is_open()){
        file.close();
    } else {
        return create_error_response(404, "Not Found", connection);
    }

    // now we can create our response
    Response response;
    response.response_code = 200;
    response.response_reason = "OK";
    response.response_body = "";
    response.headers = create_response_header_vec(
        datetime_rfc1123(),
        datetime_rfc1123(),
        content_type,
        std::to_string(length),
        connection
    );
    return response;
}