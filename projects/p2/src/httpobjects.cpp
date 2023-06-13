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