#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <filesystem>
#include "httpobjects.hpp"


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


std::string RFC1123_DateTimeNow(){
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
 
    // <img src=""../static/cat.jpg"" alt=""cat"">

    std::string curDateTime = RFC1123_DateTimeNow();
    response.headers.push_back(Response_header("Date", curDateTime));
    response.headers.push_back(Response_header("Server", SERVER_VALUE));
    response.headers.push_back(Response_header("Connection", "keep-alive"));
    response.headers.push_back(Response_header("Content-Type", "text/html"));
    response.headers.push_back(Response_header("Content-Length", std::to_string(response.response_body.size())));
    response.headers.push_back(Response_header("Last-Modified", curDateTime));
    return response;
}

Response unimplemented_response(){
        Response response = Response(
        501,
        "Not Implemented",
        std::vector<Response_header>(),
        "<html><body><h1>501 Not Implemented</h1></body></html>"
    );

    std::string curDateTime = RFC1123_DateTimeNow();
    response.headers.push_back(Response_header("Date", curDateTime));
    response.headers.push_back(Response_header("Server", SERVER_VALUE));
    response.headers.push_back(Response_header("Connection", "keep-alive"));
    response.headers.push_back(Response_header("Content-Type", "text/html"));
    response.headers.push_back(Response_header("Content-Length", std::to_string(response.response_body.size())));
    response.headers.push_back(Response_header("Last-Modified", curDateTime));
    return response;
}

Response timeout_response(){
    Response response = Response(
        408,
        "Request Timeout",
        std::vector<Response_header>(),
        "<html><body><h1>408 Request Timeout</h1></body></html>"
    );

    std::string curDateTime = RFC1123_DateTimeNow();
    response.headers.push_back(Response_header("Date", curDateTime));
    response.headers.push_back(Response_header("Server", SERVER_VALUE));
    response.headers.push_back(Response_header("Connection", "close"));
    response.headers.push_back(Response_header("Content-Type", "text/html"));
    response.headers.push_back(Response_header("Content-Length", std::to_string(response.response_body.size())));
    response.headers.push_back(Response_header("Last-Modified", curDateTime));
    return response;
}

Response create_error_response(int code, std::string reason, std::string connection){
    std::string curDateTime = RFC1123_DateTimeNow();
    std::vector<Response_header> new_headers;

    std::ostringstream bodystream;
    bodystream << "<html><h1> " << code << " " << reason << "</h1></html>"; 
    std::string body = bodystream.str();

    std::cout<<"CONNECTIOH: "<<connection<<"\n";

    new_headers.push_back(Response_header("Date", curDateTime));
    new_headers.push_back(Response_header("Server", SERVER_VALUE));
    new_headers.push_back(Response_header("Connection", connection));
    new_headers.push_back(Response_header("Content-Type", "text/html"));
    new_headers.push_back(Response_header("Content-Length", std::to_string(body.size())));
    new_headers.push_back(Response_header("Last-Modified", curDateTime));

    return Response(code, reason, new_headers, body);
}

Response create_good_response(std::string connection, std::string body, std::string content_type){
    std::string curDateTime = RFC1123_DateTimeNow();
    std::vector<Response_header> new_headers;

    new_headers.push_back(Response_header("Date", curDateTime));
    new_headers.push_back(Response_header("Server", SERVER_VALUE));
    new_headers.push_back(Response_header("Connection", connection));
    new_headers.push_back(Response_header("Content-Type", content_type));
    new_headers.push_back(Response_header("Content-Length", std::to_string(body.size())));
    new_headers.push_back(Response_header("Last-Modified", curDateTime));

    return Response(200, "OK", new_headers, body);
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

    std::string full_filepath = root_dir + filepath;
    std::cout << "filepath " << full_filepath << std::endl;

    std::string body = "";
    
    std::filesystem::path cwd = std::filesystem::current_path() / "filename.txt";
    std::ofstream file2(cwd.string());
    file2.close();
    std::cout << "pwd:" << std::filesystem::current_path() << '\n';

    std::ifstream file(root_dir + filepath, std::ios::binary);
    if(file.is_open()){
        std::string line;
        while(getline(file, line)){
            body += line;
        }
        file.close();
    } else {
        return create_error_response(404, "Not Found", connection);
    }

    return create_good_response(connection, body, content_type);
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

    std::string full_filepath = root_dir + filepath;
    std::cout << "filepath " << full_filepath << std::endl;

    std::string body = "";
    
    std::filesystem::path cwd = std::filesystem::current_path() / "filename.txt";
    std::ofstream file2(cwd.string());
    file2.close();
    std::cout << "pwd:" << std::filesystem::current_path() << '\n';

    std::ifstream file(root_dir + filepath, std::ios::binary);
    if(file.is_open()){
        file.close();
    } else {
        return create_error_response(404, "Not Found", connection);
    }

    return create_good_response(connection, body, content_type);
}