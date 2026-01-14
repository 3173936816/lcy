#include "protocol/protocol.hpp"

#include <iostream>
#include <string>

void test_parse_http_request_line() {
    lcy::protocol::http::Parser parser;
    lcy::protocol::http::Request request;
	lcy::protocol::http::Parser::RetCode retcode;
    
    //std::string line = "GET / HTTP/1.1\r\n"
    //                   "Transfer-Encoding: chunked\r\n"
    //                   "Content-Type: text/html\r\n"
    //                   "\r\n"
    //                   "F\r\n{ lcy: l12345 }\r\n"  // 16进制F = 15，数据15个字符
    //                   "F\r\n{ lcy2: 12345 }\r\n"   // 第二个chunk，也是15个字符
    //                   "0\r\n\r\n";                  // 结束chunk

    //retcode = parser.parse(&line[0], line.length(), request);
    //if (retcode == lcy::protocol::http::Parser::RetCode::ERROR) {
    //    std::cout << "error" << std::endl;
    //} else if (retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA) {
    //    std::cout << "waiting data" << std::endl;
    //} else {
    //    std::cout << "ready" << std::endl;

    //    std::cout << "method : " << lcy::protocol::http::MethodToString(request.method()) << std::endl;
    //    std::cout << "uri : " << request.uri() << std::endl;
    //    std::cout << "version : " << lcy::protocol::http::VersionToString(request.version()) << std::endl;

    //    for (auto& kv : request.getHeadersRef()) {
    //        std::cout << kv.first << "--" << kv.second << std::endl;
    //    }

    //    std::cout << "body : " << request.body() << std::endl;
    //    std::cout << "nparse : " << parser.nparse() << std::endl;
    //}

    //std::cout << std::endl;
    
	// test chunk
    //parser.reset();
    //request.clear();
    
    std::string line1 = "GET / HTTP/1.1\r\n"
                        "Transfer-Encoding: chunked\r\n"
						"Content-Type: text/html\r\n"
                        "\r\n"
                        "F\r\n{ lcy: l12345 }";

	std::cout << "line1 len : " << line1.length() << std::endl;
   
    retcode = parser.parse(&line1[0], line1.length(), request);
    if (retcode == lcy::protocol::http::Parser::RetCode::ERROR) {
        std::cout << "error" << std::endl;
    } else if (retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA) {
        std::cout << "waiting data" << std::endl;
    } else {
        std::cout << "ready" << std::endl;
    }
	
    std::cout << "nparse : " << parser.nparse() << std::endl;

    std::cout << std::endl;
    
    std::string line2 = line1 + 
						"\r\nF\r\n{ lcy2: 12345 }\r\n"
                        "0\r\n\r\n";
	std::cout << "len2 len : " << line2.length() << std::endl;
    
    retcode = parser.parse(&line2[0], line2.length(), request);
    if (retcode == lcy::protocol::http::Parser::RetCode::ERROR) {
        std::cout << "error" << std::endl;
    } else if (retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA) {
        std::cout << "waiting data" << std::endl;
    } else {
        std::cout << "ready" << std::endl;
        
        std::cout << "method : " << lcy::protocol::http::MethodToString(request.method()) << std::endl;
        std::cout << "uri : " << request.uri() << std::endl;
        std::cout << "version : " << lcy::protocol::http::VersionToString(request.version()) << std::endl;

        for (auto& kv : request.getHeadersRef()) {
            std::cout << kv.first << "--" << kv.second << std::endl;
        }

        std::cout << "body : " << request.body() << std::endl;
        std::cout << "nparse : " << parser.nparse() << std::endl;
    }
}

void test_parse_http_response_line() {
	lcy::protocol::http::Parser parser;
	lcy::protocol::http::Response response;
	std::string line = "HTTP/1.1 200 ok\r\n";
	
	auto retcode = parser.parse(&line[0], line.length(), response);
	if ( retcode == lcy::protocol::http::Parser::RetCode::ERROR ) {
		std::cout << "error" << std::endl;
	} else if ( retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA ) {
		std::cout << "waiting data" << std::endl;
	} else {
		std::cout << "ready" << std::endl;

		std::cout << "version : " << lcy::protocol::http::VersionToString(response.version()) << std::endl;
		std::cout << "state : " << (int)response.state() << std::endl;
		std::cout << "description : " << lcy::protocol::http::StateToDesc(response.state()) << std::endl;
		std::cout << "nparse : " << parser.nparse() << std::endl;
	}
}

int main() {
	 test_parse_http_request_line();
	// test_parse_http_response_line();

	return 0;
}
