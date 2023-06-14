#pragma once
#include "httpobjects.hpp"

#define SUCCESS 0




//Result of parsing
struct ParseResult {
	bool success;
	Request* request;
	int response_code;
	std::string response_reason;
};

ParseResult parse(char *buffer, int size);
inline std::string get_reason(ResponseCode code);

// functions decalred in parser.y
int yyparse();
void set_parsing_options(char *buf, size_t i, Request *request);
// to allow resetting the parser the request failed to properly parse
void yyrestart(FILE *input_file); 
