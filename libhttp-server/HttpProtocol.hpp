#ifndef __HTTP_PROTOCOL_HPP__
#define __HTTP_PROTOCOL_HPP__

#include <vector>
#include <string>
#include <map>

#include <liboslayer/os.hpp>

#include "MultiConn.hpp"
#include "HttpHeader.hpp"
#include "OnHttpRequestHandler.hpp"
#include "HttpConnection.hpp"

namespace HTTP {

	/**
	 * @brief vpath comparator
	 */
	struct vpath_comp {
		bool operator() (const std::string & lhs, const std::string & rhs) const {
			return lhs > rhs;
		}
	};

	/**
	 * @brief http protocol
	 */
	class HttpProtocol : public MultiConnProtocol, public OnHttpRequestHandler {
	private:
        OS::Semaphore handlerSem;
        OS::Semaphore connSem;
		std::map<int, HttpConnection*> conns;
		std::map<std::string, OnHttpRequestHandler*, vpath_comp> handlers;
		HttpRequest * request;
		HttpResponse * response;
		std::string page404;
		std::string page500;
		
	public:
		HttpProtocol();
		virtual ~HttpProtocol();

		virtual void onClientConnect(MultiConn & server, ClientSession & client);
		virtual void onClientReceive(MultiConn & server, ClientSession & client, Packet & packet);
		virtual void onClientDisconnect(MultiConn & server, ClientSession & client);

		std::string pathOnly(std::string unclearPath);
		void vpath(std::string path, OnHttpRequestHandler * handler);
		OnHttpRequestHandler * getHandler(std::string path);

		virtual void onRequest(HttpRequest & request, HttpResponse & response);

		void setPage404(const std::string & html);
		void setPage500(const std::string & html);
	};
}

#endif
