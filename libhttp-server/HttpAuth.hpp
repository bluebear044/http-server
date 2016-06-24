#ifndef __HTTP_AUTH_HPP__
#define __HTTP_AUTH_HPP__

#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

namespace HTTP {
	
	class HttpAuth {
	private:
		std::string _realm;
	public:
		HttpAuth();
		virtual ~HttpAuth();
		virtual bool validate(HttpRequest & request) = 0;
		virtual void setAuthentication(HttpResponse & response) = 0;
		std::string & realm();
	};
}

#endif
