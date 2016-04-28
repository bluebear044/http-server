#include <liboslayer/os.hpp>
#include <liboslayer/SecureSocket.hpp>
#include <liboslayer/Utils.hpp>
#include <liboslayer/Properties.hpp>
#include <libhttp-server/AnotherHttpServer.hpp>
#include <libhttp-server/HttpSessionManager.hpp>
#include <libhttp-server/HttpSessionTool.hpp>
#include <libhttp-server/LispPage.hpp>
#include <libhttp-server/Url.hpp>
#include <libhttp-server/StringDataSink.hpp>
#include <liboslayer/Lisp.hpp>

using namespace std;
using namespace OS;
using namespace UTIL;
using namespace HTTP;

static AutoRef<Logger> logger = LoggerFactory::getInstance().getLogger(__FILE__);
HttpSessionManager sessionManager(30 * 60 * 1000);

class ServerConfig : public HttpServerConfig {
private:
    string host;
    int port;
    bool secure;
    string certPath;
    string keyPath;
    string defaultBrowsePath;
    string browseIndexPath;
    
public:
    
    ServerConfig() : port(0), secure(false), defaultBrowsePath(".") {}
    virtual ~ServerConfig() {}
    void load(const string & configPath) {
        loadFromFile(configPath);
        host = getProperty("domain.host");
        port = getIntegerProperty("listen.port");
        string secureString = getProperty("secure");
        secure = secureString.empty() == false && !secureString.compare("y");
        certPath = getProperty("cert.path");
        keyPath = getProperty("key.path");
        defaultBrowsePath = getProperty("default.browse.path");
        browseIndexPath = getProperty("browse.index");
    }
    string getHost() { return host; }
    int getPort() { return port; }
    bool isSecure() { return secure; }
    string getCertPath() { return certPath; }
    string getKeyPath() { return keyPath; }
    string getDefaultBrowsePath() { return defaultBrowsePath; }
    string getBrowseIndexPath() { return browseIndexPath; }
};

ServerConfig config;

static void redirect(ServerConfig & config, HttpRequest & request, HttpResponse & response, HttpSession & session, const string & uri) {
    
    HttpResponseHeader & header = response.getHeader();
    
    string host = config.getHost();
    string port = Text::toString(request.getLocalAddress().getPort());
    if (!request.getHeaderField("Host").empty()) {
        Url url("http://" + request.getHeaderField("Host"));
        host = url.getHost();
        port = url.getPort();
    }
    
    response.setStatusCode(302);
    header.setHeaderField("Location", (config.isSecure() ? "https://" : "http://") +
                          host + ":" + port + "/" +
                          HttpSessionTool::urlMan(uri, session));
    response.setContentType("text/html");
    header.setConnection("close");
}


class FileBrowseHttpRequestHandler : public HttpRequestHandler {
private:
	string defaultPath;
    string browseIndexPath;
	string indexFile;
	map<string, string> mimeTypes;
	
public:
	FileBrowseHttpRequestHandler(const string & defaultPath, const string & browseIndexPath) : defaultPath(defaultPath), browseIndexPath(browseIndexPath) {
		mimeTypes["txt"] = "text/plain";
		mimeTypes["htm"] = "text/html";
		mimeTypes["html"] = "text/html";
		mimeTypes["css"] = "text/css";
		mimeTypes["js"] = "text/javascript";
		mimeTypes["xml"] = "text/xml";
		mimeTypes["lsp"] = "Application/x-lisp";
	}
	virtual ~FileBrowseHttpRequestHandler() {}

	virtual AutoRef<DataSink> getDataSink() {
		return AutoRef<DataSink>(new StringDataSink);
	}
    
    virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {
		try {
			doHandle(request, response);
		} catch (Exception & e) {
			cout << " ** error" << endl;
			response.setStatusCode(500);
			response.setContentType("text/html");
			setFixedTransfer(response, "Server Error/" + e.getMessage());
		}
	}

	virtual void doHandle(HttpRequest & request, HttpResponse & response) {

		if (request.isWwwFormUrlEncoded()) {
			request.parseWwwFormUrlencoded();
		}

		HttpSession & session = HttpSessionTool::getSession(sessionManager, request);
		session.updateLastAccessTime();
        
        HttpResponseHeader & responseHeader = response.getHeader();
        
        response.setStatusCode(200, "OK");
        response.setContentType("text/html");
		responseHeader.setConnection("close");

		string path = request.getParameter("path");
        if (path.empty()) {
			path = defaultPath;
		}
        
        logger->logd(Text::format("** Path: %s [%s:%d]", path.c_str(),
                                 request.getRemoteAddress().getHost().c_str(),
								 request.getRemoteAddress().getPort()));
        
        if (!Text::startsWith(path, defaultPath) || path.find("..") != string::npos) {
            response.setStatusCode(403);
            setFixedTransfer(response, "Not permitted");
            return;
        }

		string content;

		if (!browseIndexPath.empty()) {
			File file(browseIndexPath);
			FileReader reader(file);
			LispPage page;
			page.applyWeb();
			page.applySession(session);
			page.applyRequest(request);
			page.applyResponse(response);
			page.env()["*path*"] = LISP::text(path);
			content = page.parseLispPage(reader.dumpAsString());
		}

		if (response.needRedirect()) {
			redirect(config, request, response, session, response.getRedirectLocation());
			return;
		}

		if (response["set-file-transfer"].empty() == false) {

			logger->logd(Text::format("** File transfer / %s", response["set-file-transfer"].c_str()));
			
			File file(response["set-file-transfer"]);
			if (!file.exists() || !file.isFile()) {
				response.setStatusCode(404);
				setFixedTransfer(response, "Not Found");
				return;
			}

			map<string, string> types = mimeTypes;

			if (request.getParameter("mode") == "streaming") {
				types["mp4"] = "video/mp4";
			}
			
			if (types.find(file.getExtension()) != types.end()) {
				response.setContentType(types[file.getExtension()]);
			} else {
				response.setContentType("Application/octet-stream");
				response.getHeader().setHeaderField("Content-Disposition",
													"attachment; filename=\"" + file.getName() + "\"");
			}

			string range = request.getHeaderFieldIgnoreCase("Range");
			if (!range.empty()) {
				size_t f = range.find("=");
				if (f != string::npos) {
					range = range.substr(f + 1);
					f = range.find("-");
					if (f != string::npos) {
						string start = range.substr(0, f);
						string end = range.substr(f + 1);
                        try {
                            setPartialFileTransfer(response,
                                                   file,
                                                   (size_t)Text::toLong(start),
                                                   (size_t)Text::toLong(end));
                            return;
                        } catch (Exception e) {
                            logger->loge(e.getMessage());
                            response.setStatusCode(500);
                            setFixedTransfer(response, e.getMessage());
                            return;
                        }
					}
				}
			}

			setFileTransfer(response, file);
			return;
		}

        setFixedTransfer(response, content);
    }
};

class SinglePageHttpRequestHandler : public HttpRequestHandler {
private:
	string path;
public:
	SinglePageHttpRequestHandler(const string & path) : path(path) {}
	virtual ~SinglePageHttpRequestHandler() {}

	virtual AutoRef<DataSink> getDataSink() {
		return AutoRef<DataSink>(new StringDataSink);
	}

	virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {

		if (request.isWwwFormUrlEncoded()) {
			request.parseWwwFormUrlencoded();
		}
		
		HttpSession & session = HttpSessionTool::getSession(sessionManager, request);
		session.updateLastAccessTime();
		
		File file(path);
		FileReader reader(file);

		response.setStatusCode(200);
		response.setContentType("text/html");
            
		LispPage page;
		page.applyWeb();
		page.applySession(session);
		page.applyRequest(request);
		page.applyResponse(response);
		string content = page.parseLispPage(reader.dumpAsString());
		
		setFixedTransfer(response, content);
	}
};

class FaviconHttpRequestHandler : public HttpRequestHandler {
private:
    string path;
public:
    FaviconHttpRequestHandler(const string & path) : path(path) {}
    virtual ~FaviconHttpRequestHandler() {}
    
    virtual AutoRef<DataSink> getDataSink() {
        return AutoRef<DataSink>(new StringDataSink);
    }
    
    virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink,HttpResponse & response) {
        
        cout << " ** favico / path : " << request.getPath() << endl;
        
        File file(path);
        if (!file.exists() || !file.isFile()) {
            response.setStatusCode(404);
            return;
        }
        
        response.setStatusCode(200);
        setFileTransfer(response, file);
    }
};

class StaticHttpRequestHandler : public HttpRequestHandler {
private:
    string basePath;
	string prefix;
	string indexName;
	map<string, string> mimeTypes;
public:
	StaticHttpRequestHandler(const string & basePath, const string & prefix, const string & indexName) : basePath(basePath), prefix(prefix), indexName(indexName) {
		mimeTypes["txt"] = "text/plain";
		mimeTypes["htm"] = "text/html";
		mimeTypes["html"] = "text/html";
		mimeTypes["css"] = "text/css";
		mimeTypes["js"] = "text/javascript";
		mimeTypes["xml"] = "text/xml";
		mimeTypes["lsp"] = "Application/x-lisp";
		mimeTypes["woff"] = "application/x-font-woff";
	}
    virtual ~StaticHttpRequestHandler() {}
    
    virtual AutoRef<DataSink> getDataSink() {
        return AutoRef<DataSink>(new StringDataSink);
    }
    
    virtual void onHttpRequestContentCompleted(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {

		try {
			doHandle(request, sink, response);
		} catch (Exception & e) {
			cout << " ** error" << endl;
			response.setStatusCode(500);
			response.setContentType("text/html");
			setFixedTransfer(response, "Server Error/" + e.getMessage());
		}
	}

	void doHandle(HttpRequest & request, AutoRef<DataSink> sink, HttpResponse & response) {

		logger->logd(Text::format("** Path: %s [%s:%d]", request.getPath().c_str(),
                                 request.getRemoteAddress().getHost().c_str(),
								 request.getRemoteAddress().getPort()));

		if (request.isWwwFormUrlEncoded()) {
			request.parseWwwFormUrlencoded();
		}

		HttpSession & session = HttpSessionTool::getSession(sessionManager, request);
		session.updateLastAccessTime();

		string path = request.getPath();
		path = path.substr(prefix.size());
        
        cout << " ** static / path : " << path;

        File file(File::mergePaths(basePath, path));
        if (!file.exists() || !file.isFile()) {

			if (path == "/") {
				file = File(File::mergePaths(basePath, indexName));
				if (!file.exists()) {
					cout << " := 404 no index, " << File::mergePaths(basePath, indexName) << endl;
					response.setStatusCode(404);
					return;
				}
			} else {
				cout << " := 404 not found" << endl;
				response.setStatusCode(404);
				return;
			}
        }

		if (file.getExtension() == "lsp") {

			FileReader reader(file);
			
			LispPage page;
			page.applyWeb();
			page.applySession(session);
			page.applyRequest(request);
			page.applyResponse(response);
			string content = page.parseLispPage(reader.dumpAsString());

			if (response.needRedirect()) {
				cout << " := 302 redirect" << endl;
				redirect(config, request, response, session, response.getRedirectLocation());
				return;
			}


			if (response["set-file-transfer"].empty() == false) {

				File file(response["set-file-transfer"]);
				if (!file.exists() || !file.isFile()) {
					cout << " := 404" << endl;
					response.setStatusCode(404);
					setFixedTransfer(response, "Not Found");
					return;
				}

				setContentTypeWithFile(request, response, file);

				string range = request.getHeaderFieldIgnoreCase("Range");
				if (!range.empty()) {
					size_t f = range.find("=");
					if (f != string::npos) {
						range = range.substr(f + 1);
						f = range.find("-");
						if (f != string::npos) {
							string start = range.substr(0, f);
							string end = range.substr(f + 1);
							try {
								cout << " := 206 partial transfer" << endl;
								// TODO: implicitly handling the response code
								setPartialFileTransfer(response,
													   file,
													   (size_t)Text::toLong(start),
													   (size_t)Text::toLong(end));
								return;
							} catch (Exception e) {
								cout << " := 500 " << e.getMessage() << endl;
								response.setContentType("text/html");
								response.setStatusCode(500);
								setFixedTransfer(response, e.getMessage());
								return;
							}
						}
					}
				}

				cout << " := 200 fixed transfer" << endl;
				response.setStatusCode(200);
				setFileTransfer(response, file);
				return;
			}
			
			cout << " := 200 lisp page" << endl;
			response.setStatusCode(200);
			response.setContentType("text/html");
			setFixedTransfer(response, content);
			return;
		}

		cout << " := 200 static" << endl;
        response.setStatusCode(200);
		setContentTypeWithFile(request, response, file);
        setFileTransfer(response, file);
    }

	void setContentTypeWithFile(HttpRequest & request, HttpResponse & response, File & file) {
		map<string, string> types = mimeTypes;

		if (request.getParameter("mode") == "streaming") {
			types["mp4"] = "video/mp4";
		}
			
		if (types.find(file.getExtension()) != types.end()) {
			response.setContentType(types[file.getExtension()]);
		} else {
			response.setContentType("Application/octet-stream");
			response.getHeader().setHeaderField("Content-Disposition",
												"attachment; filename=\"" + file.getName() + "\"");
		}
	}
};

size_t readline(char * buffer, size_t max) {
	if (fgets(buffer, (int)max - 1, stdin)) {
		buffer[strlen(buffer) - 1] = 0;
		return strlen(buffer);
	}
	return 0;
}

string prompt(const char * msg) {
    char buffer[1024] = {0,};
    printf("%s", msg);
    if (readline(buffer, sizeof(buffer)) > 0) {
        return string(buffer);
    }
    return "";
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


int main(int argc, char * args[]) {

	LoggerFactory::getInstance().setLoggerDescriptorSimple("*", "basic", "console");

	string configPath;
	
	if (argc > 1) {
		configPath = args[1];
    } else {
        char buffer[1024] = {0,};
        printf("Configuration file path: ");
        readline(buffer, sizeof(buffer));
		configPath = buffer;
    }

	config.load(configPath);
    
	System::getInstance()->ignoreSigpipe();

    AnotherHttpServer * server = NULL;

	config.setProperty("thread.count", 50);
    if (config.isSecure()) {
        
#if defined(USE_OPENSSL)
        
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
#else
        throw Exception("SSL not supported");
#endif
    } else {
        server = new AnotherHttpServer(config);
    }

	AutoRef<HttpRequestHandler> browse(new FileBrowseHttpRequestHandler(config.getDefaultBrowsePath(),
																		config.getBrowseIndexPath()));
	server->registerRequestHandler("/browse", browse);
	
	AutoRef<HttpRequestHandler> single(new SinglePageHttpRequestHandler(config["default.page"]));
	server->registerRequestHandler("/", single);
	server->registerRequestHandler("/index.htm", single);
    
    AutoRef<HttpRequestHandler> favico(new FaviconHttpRequestHandler(config["favicon.path"]));
    server->registerRequestHandler("/favicon.ico", favico);

	AutoRef<HttpRequestHandler> staticHandler(new StaticHttpRequestHandler(config["static.base.path"], "/static", config["index.name"]));
    server->registerRequestHandler("/static/*", staticHandler);

	server->startAsync();

	printf("Listening... %d\n", config.getPort());
	
	while (1) {
		char buffer[1024] = {0,};
		if (readline(buffer, sizeof(buffer)) > 0) {
			if (!strcmp(buffer, "q")) {
				break;
			} else if (!strcmp(buffer, "s")) {
				printf("Listen port: %d\n", config.getPort());
			}
		}
	}

	printf("** stopping\n");
	server->stop();
    delete server;
	printf("** done\n");
    
    return 0;
}

