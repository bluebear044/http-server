#ifndef __HTTP_HEADER_PARSER_HPP__
#define __HTTP_HEADER_PARSER_HPP__

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cstdlib>

#include "HttpHeader.hpp"

namespace http {

	/**
	 * @brief http header parse result
	 */
	class HttpHeaderParseResult {
	private:
		bool success;
		int errorCode;
		std::string errorMessage;
        HttpHeader header;

	public:
		HttpHeaderParseResult();
		HttpHeaderParseResult(bool success, int errorCode, std::string errorMessage);
		virtual ~HttpHeaderParseResult();

		void clear();
		int setResult(bool success, int errorCode, std::string errorMessage);
		HttpHeader & getHeader();
		bool succeeded();
		int getErrorCode();
		std::string getErrorMessage();
	};

	/**
	 * @brief http header parser
	 */
	class HttpHeaderParser {
	private:
        HttpHeaderParseResult result;
	public:
		HttpHeaderParser();
		virtual ~HttpHeaderParser();
		
		int parse(const std::string & header);
		bool isEmptyLine(std::string & line);
		std::string readLine(const std::string & full, size_t & f);
		int parseFirstLine(HttpHeader & header, std::string & line);
		int parseHeaderField(HttpHeader & header, std::string line);
		HttpHeaderParseResult & getResult();
		HttpHeader & getHeader();
		std::string removeCarriageReturnNewLine(std::string line);
	};
}

#endif
