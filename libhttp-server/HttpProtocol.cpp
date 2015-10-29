#include <liboslayer/Text.hpp>
#include "HttpProtocol.hpp"
#include "HttpStatusCodes.hpp"

namespace HTTP {

	using namespace std;
	using namespace UTIL;
    using namespace OS;

	/**
	 * @brief http protocol
	 */
    HttpProtocol::HttpProtocol() : handlerSem(1), connSem(1) {
		string msg404 = HttpStatusCodes::getMessage(404);
		string msg500 = HttpStatusCodes::getMessage(500);
		page404 = "<!DOCTYPE html><html><head><title>404 " + msg404 + "</title></head><body><h1>404 " + msg404 + "</h1><p>Page Not Found...</p></body></html>";
		page500 = "<!DOCTYPE html><html><head><title>500 " + msg500 + "</title></head><body><h1>500 " + msg500 + "</h1><p>Server Error...</p></body></html>";
	}
	HttpProtocol::~HttpProtocol() {
	}
	
	void HttpProtocol::onClientConnect(MultiConn & server, ClientSession & client) {
        
        AutoLock lock(connSem);
        
		HttpConnection * conn = new HttpConnection(this);
		conns[client.getId()] = conn;

		conn->onClientConnect(server, client);
	}

	void HttpProtocol::onClientReceive(MultiConn & server, ClientSession & client, Packet & packet) {
        
        AutoLock lock(connSem);
       
		HttpConnection * conn = conns[client.getId()];
		if (conn) {
			conn->onClientReceive(server, client, packet);
		}
	}

	void HttpProtocol::onClientDisconnect(MultiConn & server, ClientSession & client) {

        AutoLock lock(connSem);
        
		HttpConnection * conn = conns[client.getId()];
        if (conn) {
            conn->onClientDisconnect(server, client);
            delete conn;
        }
		conns.erase(client.getId());
	}
	
	string HttpProtocol::pathOnly(string unclearPath) {
		size_t f = unclearPath.find("?");
		if (f == string::npos) {
			return unclearPath;
		}
		return unclearPath.substr(0, f);
	}
	
	void HttpProtocol::vpath(string path, OnHttpRequestHandler * handler) {
        
        AutoLock lock(handlerSem);
        
		if (!handler) {
			handlers.erase(path);
		} else {
			handlers[path] = handler;
		}
	}
	
	OnHttpRequestHandler * HttpProtocol::getHandler(string path) {
		map<string, OnHttpRequestHandler*, vpath_comp>::iterator iter;
		string p = pathOnly(path);
        
        AutoLock lock(handlerSem);
        
		for (iter = handlers.begin(); iter != handlers.end(); iter++) {
			if (Text::match(iter->first, p)) {
				return iter->second;
			}
		}
		return NULL;
	}

	void HttpProtocol::onRequest(HttpRequest & request, HttpResponse & response) {
		OnHttpRequestHandler * handler = getHandler(request.getPath());
		if (handler) {
			handler->onRequest(request, response);
		} else {
			response.setStatusCode(404, HttpStatusCodes::getMessage(404));
			response.write(page404);
			response.setComplete();
		}
	}

	void HttpProtocol::setPage404(const std::string & html) {
		this->page404 = html;
	}

	void HttpProtocol::setPage500(const std::string & html) {
		this->page500 = html;
	}
}
