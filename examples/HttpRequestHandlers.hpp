#ifndef __HTTP_REQUEST_HANDLERS_HPP__
#define __HTTP_REQUEST_HANDLERS_HPP__

#include <string>
#include "HttpProtocol.hpp"

namespace HTTP {
	
	/**
	 * @brief file redirect handler
	 */
	class FileRedirectHandler : public HttpRequestHandler {
	private:
		std::string basePath;
	public:
		FileRedirectHandler(std::string basePath);
		virtual ~FileRedirectHandler();

		virtual void onRequest(HttpRequest & request, HttpResponse & response);

		int getFileSize(std::string path);
		std::string getContentType(std::string path);
		bool onFile(std::string path, HttpResponse & response);
		bool onDirectory(std::string path, HttpResponse & response);
		std::string getFullPath(const std::string & path);
	};

	/**
	 * @brief REST handler
	 */
	class RESThandler : public HttpRequestHandler {
	public:
		RESThandler();
		virtual ~RESThandler();

		virtual void onRequest(HttpRequest & request, HttpResponse & response);
		
		virtual void onPost(HttpRequest & request, HttpResponse & response);
		virtual void onGet(HttpRequest & request, HttpResponse & response);
		virtual void onPut(HttpRequest & request, HttpResponse & response);
		virtual void onDelete(HttpRequest & request, HttpResponse & response);
		
	};

}

#endif