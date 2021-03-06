#include <liboslayer/os.hpp>
#include <liboslayer/Arguments.hpp>
#include <liboslayer/SecureSocket.hpp>
#include <liboslayer/FileStream.hpp>
#include <liboslayer/AutoRef.hpp>
#include <liboslayer/AutoLock.hpp>
#include <liboslayer/Logger.hpp>
#include <liboslayer/Properties.hpp>
#include <liboslayer/DatabaseDriver.hpp>
#include <liboslayer/Pool.hpp>
#include <libhttp-server/AnotherHttpServer.hpp>
#include <libhttp-server/HttpSessionManager.hpp>
#include <libhttp-server/HttpSessionTool.hpp>
#include <libhttp-server/LispPage.hpp>
#include <libhttp-server/Url.hpp>
#include <libhttp-server/MimeTypes.hpp>
#include <libhttp-server/StringDataSink.hpp>
#include <liboslayer/Lisp.hpp>
#include <liboslayer/Base64.hpp>
#include <liboslayer/Hash.hpp>
#include <liboslayer/File.hpp>
#include <libhttp-server/AnotherHttpClient.hpp>
#include <libhttp-server/BasicAuth.hpp>


using namespace std;
using namespace osl;
using namespace http;

static AutoRef<Logger> logger = LoggerFactory::instance().
	getObservingLogger(File::basename(__FILE__));

/**
 * @brief dump response handler
 */
class DumpResponseHandler : public OnHttpResponseListener {
private:
	HttpResponseHeader responseHeader;
	string dump;
public:
    DumpResponseHandler() {}
    virtual ~DumpResponseHandler() {}
	virtual AutoRef<DataSink> getDataSink() {
		return AutoRef<DataSink>(new StringDataSink);
	}
	virtual void onTransferDone(HttpResponse & response, AutoRef<DataSink> sink, AutoRef<UserData> userData) {
		responseHeader = response.header();
        if (!sink.nil()) {
			dump = ((StringDataSink*)&sink)->data();
        } else {
			throw Exception("error");
		}
    }
    virtual void onError(Exception & e, AutoRef<UserData> userData) {
		logger->error("Error/e: " + e.toString());
    }
	HttpResponseHeader & getResponseHeader() {
		return responseHeader;
	}
	string & getDump() {
		return dump;
	}
};

/**
 * @brief 
 */
class ServerConfig : public HttpServerConfig {
private:
    
public:
    ServerConfig() {/**/}
    virtual ~ServerConfig() {/**/}
    string getHost() { return getProperty("domain.host"); }
	void setHost(const string & host) { setProperty("domain.host", host); }
    int getPort() { return getIntegerProperty("listen.port"); }
	void setPort(int port) { setProperty("listen.port", port); }
    bool isSecure() { return (getProperty("secure").empty() == false && getProperty("secure").compare("y") == 0); }
	void setSecure(bool secure) { setProperty("secure", "y"); }
    string getCertPath() { return getProperty("cert.path"); }
	void setCertPath(const string & certPath) { setProperty("cert.path", certPath); }
    string getKeyPath() { return getProperty("key.path"); }
	void setKeyPath(const string & keyPath) { setProperty("key.path", keyPath); }
};

/**
 * @brief 
 */
static void redirect(ServerConfig & config, HttpRequest & request, HttpResponse & response, AutoRef<HttpSession> session, const string & uri) {
    
    HttpResponseHeader & header = response.header();
    
    string host = config.getHost();
    string port = Text::toString(request.getLocalAddress().getPort());
    if (!request.getHeaderField("Host").empty()) {
        Url url("http://" + request.getHeaderField("Host"));
        host = url.getHost();
        port = url.getPort();
    }
    
    response.setStatus(302);
    header.setHeaderField("Location", (config.isSecure() ? "https://" : "http://") +
                          host + ":" + port + "/" +
                          HttpSessionTool::url(request, uri, session));
    response.setContentType("text/html");
	response.setContentLength(0);
}

class Cache {
private:
	Date _date;
	string _content;
public:
    Cache() {
	}
	Cache(string content) : _content(content) {
	}
    virtual ~Cache() {
	}
	Date date() {
		return _date;
	}
	string & content() {
		return _content;
	}
	bool needUpdate(const File & file) {
		return ((file.size() != (filesize_t)size()) ||
				(file.lastModifiedDate() != _date));
	}
	void update(File & file) {
		FileStream reader(file, "rb");
		_date = file.lastModifiedDate();
		_content = reader.readFullAsString();
		reader.close();
	}
	size_t size() {
		return _content.size();
	}
};

/**
 * @brief 
 */
class StaticHttpRequestHandler : public HttpRequestHandler {
private:
	HttpSessionManager sessionManager;
	bool dedicated;
	ServerConfig config;
    string basePath;
	string prefix;
	string indexName;
	string driverPath;
	string driverName;
	map<string, string> mimeTypes;
    map<string, Cache> lspMemCache;
	Pool<LispPage> lspPool;
	HttpSessionTool sessionTool;
public:
	StaticHttpRequestHandler(ServerConfig & config, const string & prefix)
		: sessionManager(30 * 60 * 1000), dedicated(false), config(config), prefix(prefix), lspPool(config.getIntegerProperty("thread.count", 50))
	{
		mimeTypes = MimeTypes::getMimeTypes();
		basePath = config["static.base.path"];
		indexName = config["index.name"];
		driverPath = config["db.driver.path"];
		driverName = config["db.driver.name"];

		_init();
	}

	StaticHttpRequestHandler(bool dedicated, ServerConfig & config, const string & prefix)
		: sessionManager(30 * 60 * 1000), dedicated(dedicated), config(config), prefix(prefix), lspPool(config.getIntegerProperty("thread.count", 50))
	{
		mimeTypes = MimeTypes::getMimeTypes();
		basePath = config["static.base.path"];
		indexName = config["dedicated.index.name"];
		driverPath = config["db.driver.path"];
		driverName = config["db.driver.name"];
        
        _init();
	}

private:
	void _init() {

		if (config.contains("session.timeout")) {
			unsigned long timeout = (unsigned long)config.getIntegerProperty("session.timeout");
			if (timeout < 60) {
				timeout = 60;
			}
			sessionManager.timeout() = timeout * 1000;
		}
		
		// lisp page pool
		deque<LispPage*> qu = lspPool.avail_queue();
		for (deque<LispPage*>::iterator iter = qu.begin(); iter != qu.end(); iter++) {
			(*iter)->applyWeb(config);
		}
	}
public:
	
    virtual ~StaticHttpRequestHandler() {/* empty */}
    
    virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {
		try {
			doHandle(request, sink, response);
			if (request.getMethod() == "HEAD") {
				response.setTransfer(AutoRef<DataTransfer>(NULL));
			}
		} catch (Exception & e) {
			logger->error(" ** error");
			response.setStatus(500);
			response.setContentType("text/html");
			response.setFixedTransfer("<html><head><title>500 Server Error</title></head>"
							 "<body><h1>Server Error</h1><p>" + e.toString() + "</p></body></html>");
		}
	}

	/**
	 * handle web
	 */
	void doHandle(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {
		
		if (request.isWwwFormUrlEncoded()) {
			request.parseWwwFormUrlencoded();
		}
		
		string path = request.getPath();
		path = path.substr(prefix.size());
		if (path.empty()) {
			AutoRef<HttpSession> session = sessionTool.handleSession(request, response, sessionManager);
			redirect(config, request, response, session, prefix.substr(1) + "/");
			return;
		}
		string log = Text::format("| %s:%d | STATIC | %s '%s'",
								  request.getRemoteAddress().getHost().c_str(),
								  request.getRemoteAddress().getPort(),
								  request.getMethod().c_str(),
								  path.c_str());
		
		File file(File::merge(basePath, path));
		if (dedicated) {
			file = File(File::merge(basePath, indexName));
		}
        if (!file.exists() || !file.isFile()) {
			if (path != "/") {
				logger->debug(log + " | 404 Not Found (" + file.path() + ")");
				setErrorPage(response, 404);
				return;
			}
			file = File(File::merge(basePath, indexName));
			if (!file.exists()) {
				logger->debug(log + " | 404 No Index File, " + File::merge(basePath, indexName));
				setErrorPage(response, 404);
				return;
			}
        }
		
		if (file.extension() == "lsp") {
            handleLispPage(file, request, sink, response);
			logger->debug(log + " : (LISP PAGE '" + file.basename() + "') | " + Text::toString(response.getStatusCode()));
			return;
		}
		
		logger->debug(log + " | 200 OK");
        response.setStatus(200);
		setContentTypeWithFile(request, response, file);
		if (request.getParameter("transfer") == "download") {
			setContentDispositionWithFile(request, response, file);
		}
        response.setFileTransfer(file);
    }

	bool needCacheUpdate(const File & file) {
		string path = file.path();
		if (lspMemCache.find(path) == lspMemCache.end()) {
			return true;
		}
		return lspMemCache[path].needUpdate(file);
	}

	/**
	 * handle lisp page
	 */
	void handleLispPage(File & file, HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {
		AutoRef<HttpSession> session = sessionTool.handleSession(request, response, sessionManager);
		session->updateLastAccessTime();
		response.setStatus(200);
		response.setContentType("text/html");
		if (needCacheUpdate(file)) {
			logger->debug("[UPDATE CACHE]");
			lspMemCache[file.path()].update(file);
		}
		string dump = lspMemCache[file.path()].content();
		string content = procLispPage(request, response, session, dump);
		if (response.redirectRequested()) {
			redirect(config, request, response, session, response.getRedirectLocation());
			return;
		}
		if (response.forwardRequested()) {
			string location = response.getForwardLocation();
			logger->debug(Text::format(" ** Forward :: location : %s", location.c_str()));
			request.setPath(location);
			response.cancelForward();
			doHandle(request, sink, response);
			return;
		}
		if (response["set-file-transfer"].empty() == false) {
			File file(response["set-file-transfer"]);
			if (!file.exists() || !file.isFile()) {
                logger->error("set-file-transfer error - file not found \"" +
							  response["set-file-transfer"] + "\"");
				setErrorPage(response, 404);
				return;
			}
			setContentTypeWithFile(request, response, file);
			setContentDispositionWithFile(request, response, file);
			if (request.containsRange()) {
				response.setPartialFileTransfer(request.getRange(), file);
			} else {
				response.setStatus(200);
				response.setFileTransfer(file);
			}
			return;
		}
		response.setFixedTransfer(content);
	}

	/**
	 * proc lisp page
	 */
	string procLispPage(HttpRequest & request, HttpResponse & response, AutoRef<HttpSession> session, const string & dump) {

		LispPage * page = lspPool.acquire();
		if (page == NULL) {
			throw Exception("no available lisp page processor");
		}

		try {
			page->applyAuth(request, response);
			page->applySession(request, session);
			page->applyRequest(request);
			page->applyResponse(response);
			unsigned long tick = tick_milli();
			string content = page->parseLispPage(dump);
			logger->debug(Text::format(" ** processing : %ld ms.", tick_milli() - tick));
			lspPool.release(page);
			return content;
		} catch (Exception e) {
			lspPool.release(page);
			throw e;
		}
	}

	/**
	 * error page
	 */
	void setErrorPage(HttpResponse & response, int errorCode) {
		response.setStatus(errorCode);
		switch (errorCode) {
		case 404:
			response.setFixedTransfer("Not Found");
			break;
		default:
			break;
		}
	}

	/**
	 * set file transfer
	 */
	void setFileTransferX(HttpRequest & request, HttpResponse & response, File & file) {
		setContentTypeWithFile(request, response, file);
		setContentDispositionWithFile(request, response, file);
		if (request.containsRange()) {
			try {
				response.setPartialFileTransfer(request.getRange(), file);
			} catch (Exception e) {
				logger->error("partial file transfer failed - " + e.message());
				response.setContentType("text/html");
				response.setStatus(500);
				response.setFixedTransfer(e.toString());
			}
			return;
		}
		response.setStatus(200);
		response.setFileTransfer(file);
	}

	/**
	 * set content type
	 */
	void setContentTypeWithFile(HttpRequest & request, HttpResponse & response, File & file) {
		map<string, string> types = mimeTypes;

		if (types.find(file.extension()) != types.end()) {
			response.setContentType(types[file.extension()]);
		} else {
			response.setContentType("Application/octet-stream");
		}
	}

	/**
	 * set cotnent disposition
	 */
	void setContentDispositionWithFile(HttpRequest & request, HttpResponse & response, File & file) {
		response.setHeaderField("Content-Disposition",
										 "attachment; filename=\"" + file.basename() + "\"");
	}
};

/**
 * @brief 
 */
class ProxyHandler : public HttpRequestHandler {
private:

	/**
	 * @brief 
	 */
	class DumpResult {
	private:
		int _statusCode;
		string _contentType;
		string _dump;
	public:
		DumpResult() : _statusCode(0) {}
		virtual ~DumpResult() {}
		int & statusCode() {
			return _statusCode;
		}
		string & contentType() {
			return _contentType;
		}
		string & dump() {
			return _dump;
		}
	};

	/**
	 * @brief 
	 */
	class Maker : public SocketMaker {
	public:
		Maker() {}
		virtual ~Maker() {}
		virtual AutoRef<Socket> make(const string & protocol, const InetAddress & addr) {
			if (protocol == "https") {
#if defined(USE_OPENSSL)
				class MyVerifier : public CertificateVerifier {
				public:
					MyVerifier() {}
					virtual ~MyVerifier() {}
					virtual bool onVerify(const VerifyError & err, const Certificate & cert) {
						return true;
					}
				};

				SecureSocket * sock = new SecureSocket(addr);
				sock->setVerifier(AutoRef<CertificateVerifier>(new MyVerifier));
				return AutoRef<Socket>(sock);
#else
				throw Exception("openssl not supported");
#endif
			}
			return AutoRef<Socket>(new Socket(addr));
		}
	};

	ServerConfig config;
	SimpleHash hash;
public:
	ProxyHandler(const ServerConfig & config) : config(config) {}
    virtual ~ProxyHandler() {}
    
    virtual AutoRef<DataSink> getDataSink() {
        return AutoRef<DataSink>(new StringDataSink);
    }
    
    virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {

		string url = request.getParameter("u");
        string nocache = request.getParameter("nocache");
        
        if (config["proxy.cache"] == "y" && nocache != "yes") {
			
			unsigned long h = hash.hash(url.c_str());
			string path = config["proxy.cache.path"];
			if (!path.empty()) {
				File p(path);
				p.mkdir();
				path = File::merge(path, Text::toString(h));
			} else {
				path = Text::toString(h);
			}

			File file(path);
			if (file.exists()) {

				FileStream in(file, "rb");
				int statusCode = Text::toInt(in.readline());
				string contentType = in.readline();
				string dump = in.readFullAsString();
				in.close();
				
				response.setStatus(statusCode);
				response.setContentType(contentType);
				response.setFixedTransfer(dump);
				
			} else {

				DumpResult result = getHttpDump(request.getMethod(), url);
				response.setStatus(result.statusCode());
				if (result.statusCode() != 500) {

					FileStream out(file, "wb");
					out.writeline(Text::toString(result.statusCode()));
					out.writeline(result.contentType());
					out.write(result.dump());
					out.close();
					
					response.setContentType(result.contentType());
					response.setFixedTransfer(result.dump());
				}
			}
			
		} else {

			DumpResult result = getHttpDump(request.getMethod(), url);
			response.setStatus(result.statusCode());
			if (result.statusCode() != 500) {
				response.setContentType(result.contentType());
				response.setFixedTransfer(result.dump());
			}
		}
	}

	DumpResult getHttpDump(const string & method, const string & url) {

		DumpResult result;
		DumpResponseHandler handler;
		AnotherHttpClient client(AutoRef<SocketMaker>(new Maker));
		client.setOnHttpResponseListener(&handler);
		client.setConnectionTimeout(3000);
		client.setRecvTimeout(3000);
		client.setFollowRedirect(true);
		client.setUrl(url);
		client.setRequest(method, LinkedStringMap());
		try {
			client.execute();
			result.statusCode() = handler.getResponseHeader().getStatusCode();
			result.contentType() = handler.getResponseHeader()["content-type"];
			result.dump() = handler.getDump();
		} catch (Exception & e) {
			logger->error(e.toString());
			result.statusCode() = 500;
		}
		return result;
	}
};

/**
 * @brief 
 */
class AuthHttpRequestHandler : public HttpRequestHandler {
private:
	AutoRef<BasicAuth> auth;
public:
	AuthHttpRequestHandler(AutoRef<BasicAuth> auth) : auth(auth) {}
    virtual ~AuthHttpRequestHandler() {}
    virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {
		try {
			if (!auth.nil() && !auth->validate(request)) {
				auth->setAuthentication(response);
				response.setContentType("text/plain");
				response.setFixedTransfer("Authentication required");
				return;
			}
			doHandle(request, sink, response);
			if (request.getMethod() == "HEAD") {
				response.setTransfer(AutoRef<DataTransfer>(NULL));
			}
		} catch (Exception & e) {
			logger->error(" ** error");
			response.setStatus(500);
			response.setContentType("text/html");
			response.setFixedTransfer("Server Error/" + e.toString());
		}
	}

	void doHandle(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {
		string log;
		logger->debug(Text::format("** Part2: %s [%s:%d]", request.getRawPath().c_str(),
                                 request.getRemoteAddress().getHost().c_str(),
								 request.getRemoteAddress().getPort()));
		if (request.isWwwFormUrlEncoded()) {
			request.parseWwwFormUrlencoded();
		}
		response.setStatus(200);
		response.setFixedTransfer("hello");
    }
};

string readline() {
	FileStream fs(stdin);
	return fs.readline();
}

string prompt(const char * msg) {
    printf("%s", msg);
	return readline();
}
int promptInteger(const char * msg) {
    string ret = prompt(msg);
    return Text::toInt(ret);
}
bool promptBoolean(const char * msg) {
    string ret = prompt(msg);
    if (ret.compare("yes") || ret.compare("y")) {
        return true;
    }
    return false;
}

#if !defined(DATA_PATH)
#define DATA_PATH ""
#endif

/**
 * @brief 
 */
int main(int argc, char * args[]) {

	Arguments arguments = ArgumentParser::parse(argc, args);

	bool deamon = false;
	System::getInstance()->ignoreSigpipe();
	
	LoggerFactory::instance().setProfile("*", "basic", "console");
	MimeTypes::load(DATA_PATH"/mimetypes");
	
	string configPath;
	if (arguments.texts().size() > 0) {
		arguments.texts()[0];
		configPath = args[1];
    } else {
        printf("Configuration file path(empty -> default): ");
		configPath = readline();
    }

	for (int i = 1; i < argc; i++) {
		if (arguments.is_set("D") == true) {
			logger->debug("DAEMON MODE");
			deamon = true;
			break;
		}
	}


	ServerConfig config;
	config["domain.host"] = "localhost";
	config["listen.port"] = "9000";
	config["static.base.path"] = "./";
	config["index.name"] = "index.html";
    config.setProperty("thread.count", 50);
	if (configPath.empty() == false) {
		config.loadFromFile(configPath);
	}

    AnotherHttpServer * server = NULL;
    if (config.isSecure() == false) {
		server = new AnotherHttpServer(config);
	} else {
#if !defined(USE_OPENSSL)
		throw Exception("SSL not supported");
#else
		logger->debug(" ** OpenSSL version: " + SecureContext::getOpenSSLVersion());
        class SecureServerSocketMaker : public ServerSocketMaker {
        private:
            string certPath;
            string keyPath;
        public:
            SecureServerSocketMaker(string certPath, string keyPath) :
				certPath(certPath), keyPath(keyPath) {}
            virtual ~SecureServerSocketMaker() {}
            virtual AutoRef<ServerSocket> makeServerSocket(int port) {
                AutoRef<ServerSocket> ret(new SecureServerSocket(port));
                ((SecureServerSocket*)&ret)->loadCert(certPath, keyPath);
                return ret;
            }
        };
		AutoRef<ServerSocketMaker> maker(new SecureServerSocketMaker(config.getCertPath(), config.getKeyPath()));
        server = new AnotherHttpServer(config, maker);
#endif
    }

	AutoRef<HttpRequestHandler> staticHandler(new StaticHttpRequestHandler(config, "/static"));
    server->registerRequestHandler("/static*", staticHandler);

	AutoRef<HttpRequestHandler> davStaticHandler(new StaticHttpRequestHandler(true, config, "/dav"));
    server->registerRequestHandler("/dav*", davStaticHandler);

	AutoRef<BasicAuth> auth(new BasicAuth(config["auth.username"], config["auth.password"]));
	AutoRef<HttpRequestHandler> authHandler(new AuthHttpRequestHandler(auth));
	server->registerRequestHandler("/auth*", authHandler);
	
	AutoRef<HttpRequestHandler> proxyHandler(new ProxyHandler(config));
	server->registerRequestHandler("/proxy", proxyHandler);

	server->startAsync();

	printf("[Listening... / port: %d]\n", config.getPort());
	
	while (1) {
		if (deamon) {
			idle(10);
			continue;
		}
		string line = readline();
		if (line.empty() == false) {
			if (line == "q" || line == "quit") {
				break;
			} else if (line == "s") {
				printf("Listen port: %d\n", config.getPort());
				printf(" * Connections: %zu\n", server->connections());
                vector<AutoRef<Connection> > conns = server->connectionManager().getConnectionList();
                for (vector<AutoRef<Connection> >::iterator iter = conns.begin(); iter != conns.end(); iter++) {
                    printf("  - [%s] Recv: %10s Bytes / Send: %10s Bytes (%ld)\n",
						   (*iter)->socket()->getRemoteInetAddress().toString().c_str(),
						   Text::toCommaNumber(Text::toString((*iter)->recvCount())).c_str(),
						   Text::toCommaNumber(Text::toString((*iter)->sendCount())).c_str(),
						   (*iter)->sendTryCount());
                }
			} else if (line == "load") {
				// TODO: implement
			}
		}
	}

	printf("[Stopping...]\n");
	server->stop();
    delete server;
	printf("[Done]\n");
    
    return 0;
}

