#include "HttpConnection.hpp"
#include "HttpStatusCodes.hpp"
#include "Text.hpp"

namespace HTTP {

	using namespace std;
	using namespace UTIL;

	/**
	 * @brief http request
	 */
	HttpRequest::HttpRequest(HttpHeader & header, OS::Socket & socket)
		: header(header), contentSize(0), socket(socket) {
	}
	HttpRequest::~HttpRequest() {
	}
	
	string HttpRequest::getMethod() {
		return header.getMethod();
	}
	string HttpRequest::getPath() {
		return header.getPath();
	}
	string HttpRequest::getHeaderField(string & name) {
		return header.getHeaderField(name);
	}
	string HttpRequest::getParameter(string & name) {
		return header.getParameter(name);
	}
	vector<string> HttpRequest::getParameters(string & name) {
		return header.getParameters(name);
	}
	HttpRequestHeader & HttpRequest::getHeader() {
		return header;
	}
	void HttpRequest::appendContent(char * buffer, size_t size) {
		content += string(buffer, size);
	}
	void HttpRequest::compactContent() {
		content = "";
	}
	string & HttpRequest::getContent() {
		return content;
	}
	int HttpRequest::getContentLength() {
		return Text::toInt(header.getParameter("Content-Length"));
	}
	string HttpRequest::getContentType() {
		return header.getParameter("Content-Type");
	}
	bool HttpRequest::remaining() {
		return getContentLength() > contentSize;
	}


	/**
	 * @brief http response constructor
	 */
	HttpResponse::HttpResponse(OS::Socket & socket) : socket(socket), complete(false) {
	}
	HttpResponse::~HttpResponse() {
	}
	void HttpResponse::setStatusCode(int code) {
		header.setStatusCode(code);
		header.setMessage(HttpStatusCodes::getMessage(code));
	}
	void HttpResponse::setStatusCode(int code, string message) {
		header.setStatusCode(code);
		header.setMessage(message);
	}
	void HttpResponse::setParts(vector<string> & parts) {
		header.setParts(parts);
	}
	void HttpResponse::setContentLength(int length) {
		header.setHeaderField("Content-Length", Text::toString(length));
	}
	void HttpResponse::setContentType(string type) {
		header.setHeaderField("Content-Type", type);
	}
	
	int HttpResponse::write(string content) {
		this->content += content;
	}
	int HttpResponse::write(char * buf, int size) {
		this->content += string(buf, size);
	}
	void HttpResponse::sendContent() {
		string header_string = header.toString();
		setContentLength(content.length());
		socket.send((char*)header_string.c_str(), header_string.length());
		socket.send((char*)content.c_str(), content.length());
	}
	void HttpResponse::setComplete() {
		if (!complete) {
			sendContent();
			complete = true;
		}
	}
	bool HttpResponse::hasComplete() {
		return complete;
	}
	
	
	
	/**
	 * @brief http connection constructor
	 */
	HttpConnection::HttpConnection(HttpRequestHandler * handler)
		: HttpRequestHandlerDecorator(handler), request(NULL), response(NULL) {
	}
	HttpConnection::~HttpConnection() {
		releaseRequest();
		releaseResponse();
	}

	void HttpConnection::onConnect(MultiConn & server, OS::Socket & client) {
	}

	void HttpConnection::onReceive(MultiConn & server, OS::Socket & client, Packet & packet) {

		if (!headerReader.complete()) {
			int offset = headerReader.read(packet.getBuffer(), packet.size());
			gatherContent(packet.getBuffer() + offset, packet.length() - offset);
		} else {
			gatherContent(packet.getBuffer(), packet.length());
		}

		if (headerReader.complete()) {
			if (!request) {
				request = new HttpRequest(headerReader.getHeader(), client);
			}
			if (!response) {
				vector<string> parts;
				response = new HttpResponse(client);
				response->setStatusCode(200);
			}
			
			onRequest(*request, *response);

			if (response->hasComplete()) {
				server.disconnect(client);
			}
		}
	}

	void HttpConnection::onDisconnect(MultiConn & server, OS::Socket & client) {
	}
	void HttpConnection::gatherContent(char * buffer, size_t size) {
	}
	void HttpConnection::onRequest(HttpRequest & request, HttpResponse & response) {
		// protocol.onRequest(request, response);
		if (getHandler()) {
			getHandler()->onRequest(request, response);
		}
	}
	bool HttpConnection::needMoreHeader() {
		return !headerReader.complete();
	}
	void HttpConnection::readHeader(Packet & packet) {
	}
	bool HttpConnection::needMoreContent() {
	}
	void HttpConnection::readContent(Packet & packet) {
	}
	void HttpConnection::releaseRequest() {
		if (request) {
			delete request;
			request = NULL;
		}
	}
	void HttpConnection::releaseResponse() {
		if (response) {
			delete response;
			response = NULL;
		}
	}
	
}
