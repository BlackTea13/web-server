#include <string>
#include <iostream>
#include "parse.h"

/**
 * Given a ResponseCode Enum, give a string of the reason
*/
inline std::string get_reason(ResponseCode code){
	switch(code){
		case ResponseCode::OK: return "OK";
		case ResponseCode::BAD_REQUEST: return "Bad Request";
		case ResponseCode::NOT_FOUND: return "Not Found";
		case ResponseCode::REQUEST_TIMEOUT: return "Request Timeout";
		case ResponseCode::NOT_IMPLEMENTED: return "Not Implemented";
		case ResponseCode::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
		default: return "Unknown Response Code";
	}
}




/*
* Given a char buffer returns the parsed request headers
*/
ParseResult parse(char *buffer, int size) {
  //Differant states in the state machine
	
	enum {
		STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
	};

	int i = 0, state;
	size_t offset = 0;
	char ch;
	char buf[8192];
	memset(buf, 0, 8192);

	state = STATE_START;
	while (state != STATE_CRLFCRLF) {
		char expected = 0;

		if (i == size)
			break;

		ch = buffer[i++];
		buf[offset++] = ch;

		switch (state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;

	}

    // Valid End State
	Request* request;
	if (state == STATE_CRLFCRLF) {
		request = (Request *) calloc(1, sizeof(Request));
        request->header_count=0;
        //TODO: You will need to handle resizing this in parser.y
        request->headers = (Request_header *) calloc(1, sizeof(Request_header));

		yyrestart(NULL); // reset parser state
		set_parsing_options(buf, i, request);

		if (yyparse() == SUCCESS) {
            return ParseResult({true, request, 200, get_reason(ResponseCode::OK)});
		}
	}

    // If we get here, parsing has resulted in an invalid End State
    printf("Parsing Failed\n");
	if (state != STATE_CRLFCRLF){
		// Request does not have the correct syntax
		std::cerr << "DEBUG: Request does not have the correct syntax" << "\n";
		free(request->headers);
		free(request);
		return ParseResult({false, NULL, 400, get_reason(ResponseCode::BAD_REQUEST)});
	}

	// We'll have to look at the request object now :(
	if (strcmp(request->http_version, "HTTP/1.1") != 0){
		// We only support HTTP/1.1
		std::cout << request->http_version << "HTTP VERSION HERE\n";
		std::cerr << "DEBUG: We only support HTTP/1.1" << "\n";
		free(request->headers);
		free(request);
		return ParseResult({false, NULL, 505, get_reason(ResponseCode::HTTP_VERSION_NOT_SUPPORTED)});
	}

	// if we somehow get here, we still return a 400 Bad Request 
	free(request->headers);
	free(request);
	return ParseResult({false, NULL, 400, get_reason(ResponseCode::BAD_REQUEST)});
}

