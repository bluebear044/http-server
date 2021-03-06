#ifndef __ANOTHER_HTTP_SERVER_HPP__
#define __ANOTHER_HTTP_SERVER_HPP__

#include "Connection.hpp"
#include "Communication.hpp"
#include "ConnectionManager.hpp"
#include "HttpHeader.hpp"
#include "HttpHeaderReader.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ChunkedReader.hpp"
#include "DataTransfer.hpp"
#include "HttpServerConfig.hpp"
#include "HttpRequestHandler.hpp"
#include "HttpRequestHandlerDispatcher.hpp"
#include "SimpleHttpRequestHandlerDispatcher.hpp"
#include "HttpCommunication.hpp"
#include "HttpCommunicationMaker.hpp"

#include <liboslayer/os.hpp>
#include <liboslayer/AutoRef.hpp>
#include <liboslayer/Text.hpp>

namespace http {

	/**
	 * @brief AnotherHttpServer
	 */
	class AnotherHttpServer {
	private:
		HttpServerConfig config;
		osl::AutoRef<HttpRequestHandlerDispatcher> dispatcher;
		ConnectionManager _connectionManager;
		osl::Thread * thread;
	public:
		AnotherHttpServer(HttpServerConfig config);
        AnotherHttpServer(HttpServerConfig config, osl::AutoRef<ServerSocketMaker> serverSocketMaker);
		virtual ~AnotherHttpServer();
		void registerRequestHandler(const std::string & pattern, osl::AutoRef<HttpRequestHandler> handler);
		void unregisterRequestHandler(const std::string & pattern);
		osl::InetAddress getServerAddress();
		void start();
		void startAsync();
		void poll(unsigned long timeout);
		void stop();
		size_t connections();
        ConnectionManager & connectionManager();
	};
}

#endif
