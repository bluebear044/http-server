libhttp_serverincdir=${includedir}/libhttp-server
libhttp_serverinc_HEADERS = \
	AnotherHttpClient.hpp \
	AnotherHttpServer.hpp \
	BasicAuth.hpp \
	BufferIndicator.hpp \
	Cache.hpp \
	CacheManager.hpp \
	ChunkedReader.hpp \
	ChunkedTransfer.hpp \
	Communication.hpp \
	Connection.hpp \
	ConnectionManager.hpp \
	ConnectionThreadPool.hpp \
	Cookie.hpp \
	DataSink.hpp \
	DataSource.hpp \
	DataTransfer.hpp \
	FileDataSource.hpp \
	FixedTransfer.hpp \
	HttpAuth.hpp \
	HttpCommunication.hpp \
	HttpCommunicationMaker.hpp \
	UrlEncoderDecoder.hpp \
	HttpException.hpp \
	HttpHeader.hpp \
	HttpHeaderDelegator.hpp \
	HttpHeaderParser.hpp \
	HttpHeaderReader.hpp \
	HttpParameter.hpp \
	HttpRequest.hpp \
	HttpRequestHandler.hpp \
	HttpRequestHandlerDispatcher.hpp \
	HttpResponse.hpp \
	HttpServerConfig.hpp \
	HttpSession.hpp \
	HttpSessionManager.hpp \
	HttpSessionTool.hpp \
	HttpStatusCodes.hpp \
	LispPage.hpp \
	MaxConnectionException.hpp \
	MimeTypes.hpp \
	Packet.hpp \
	HttpRange.hpp \
	SimpleHttpRequestHandlerDispatcher.hpp \
	StringDataSink.hpp \
	StringDataSource.hpp \
	Url.hpp

lib_LTLIBRARIES = libhttp-server.la
libhttp_server_la_SOURCES = \
	AnotherHttpClient.cpp \
	AnotherHttpClient.hpp \
	AnotherHttpServer.cpp \
	AnotherHttpServer.hpp \
	BasicAuth.cpp \
	BasicAuth.hpp \
	BufferIndicator.cpp \
	BufferIndicator.hpp \
	Cache.cpp \
	Cache.hpp \
	CacheManager.cpp \
	CacheManager.hpp \
	ChunkedReader.cpp \
	ChunkedReader.hpp \
	ChunkedTransfer.cpp \
	ChunkedTransfer.hpp \
	Communication.cpp \
	Communication.hpp \
	Connection.cpp \
	Connection.hpp \
	ConnectionManager.cpp \
	ConnectionManager.hpp \
	ConnectionThreadPool.cpp \
	ConnectionThreadPool.hpp \
	Cookie.cpp \
	Cookie.hpp \
	DataSink.hpp \
	DataSource.hpp \
	DataTransfer.cpp \
	DataTransfer.hpp \
	FileDataSource.hpp \
	FixedTransfer.cpp \
	FixedTransfer.hpp \
	HttpAuth.cpp \
	HttpAuth.hpp \
	HttpCommunication.cpp \
	HttpCommunication.hpp \
	HttpCommunicationMaker.cpp \
	HttpCommunicationMaker.hpp \
	UrlEncoderDecoder.cpp \
	UrlEncoderDecoder.hpp \
	HttpException.hpp \
	HttpHeader.cpp \
	HttpHeader.hpp \
	HttpHeaderDelegator.cpp \
	HttpHeaderDelegator.hpp \
	HttpHeaderParser.cpp \
	HttpHeaderParser.hpp \
	HttpHeaderReader.cpp \
	HttpHeaderReader.hpp \
	HttpParameter.cpp \
	HttpParameter.hpp \
	HttpRequest.cpp \
	HttpRequest.hpp \
	HttpRequestHandler.cpp \
	HttpRequestHandler.hpp \
	HttpRequestHandlerDispatcher.hpp \
	HttpResponse.cpp \
	HttpResponse.hpp \
	HttpServerConfig.hpp \
	HttpSession.cpp \
	HttpSession.hpp \
	HttpSessionManager.cpp \
	HttpSessionManager.hpp \
	HttpSessionTool.cpp \
	HttpSessionTool.hpp \
	HttpStatusCodes.cpp \
	HttpStatusCodes.hpp \
	LispPage.cpp \
	LispPage.hpp \
	MaxConnectionException.hpp \
	MimeTypes.cpp \
	MimeTypes.hpp \
	Packet.cpp \
	Packet.hpp \
	HttpRange.hpp \
	SimpleHttpRequestHandlerDispatcher.cpp \
	SimpleHttpRequestHandlerDispatcher.hpp \
	StringDataSink.hpp \
	StringDataSource.hpp \
	Url.cpp \
	Url.hpp

# cflags
libhttp_server_la_CXXFLAGS=$(PTHREAD_CXXFLAGS)
libhttp_server_la_CXXFLAGS+=$(LIBOSLAYER_CFLAGS)
libhttp_server_la_CXXFLAGS+=$(LIBOPENSSL_CFLAGS)

# libs
libhttp_server_la_LIBADD=$(LIBOSLAYER_LIBS)
libhttp_server_la_LIBADD+=$(LIBOPENSSL_LIBS)
