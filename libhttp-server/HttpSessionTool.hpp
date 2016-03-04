#ifndef __HTTP_SESSION_TOOL_HPP__
#define __HTTP_SESSION_TOOL_HPP__

#include <liboslayer/Text.hpp>
#include <string>
#include "HttpSessionManager.hpp"

namespace HTTP {
	
	class HttpSessionTool {
	private:
	public:
		HttpSessionTool() {}
		virtual ~HttpSessionTool() {}

		static std::string getSessionId(HttpRequest & request) {
			return request.getParameter("sessionId");
		}

		static HttpSession & getSession(HttpSessionManager & sessionManager, HttpRequest & request) {
			std::string sessionId = getSessionId(request);
			HttpSession & session = (sessionId.empty() ||
									 !sessionManager.hasSession(UTIL::Text::toInt(sessionId))) ?
				sessionManager.createSession() : sessionManager.getSession(UTIL::Text::toInt(sessionId));
		
			if (session.outdated()) {
				sessionManager.destroySession(session.getId());
				return sessionManager.createSession();
			}
			return session;
		}

		static std::string urlMan(const std::string & u, HttpSession & session) {
			size_t f = u.find("?");
			std::string path = (f == std::string::npos) ? u : u.substr(0, f);
			std::string rest = (f == std::string::npos) ? "" : u.substr(f);
			return path + ";sessionId=" + UTIL::Text::toString(session.getId()) + rest;
		}
	};
	
}

#endif