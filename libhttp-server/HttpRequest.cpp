#include <liboslayer/Text.hpp>
#include "HttpRequest.hpp"
#include "StringDataSink.hpp"

namespace HTTP {

	using namespace std;
	using namespace OS;
	using namespace UTIL;
	
	/**
	 * @brief http request
	 */
    HttpRequest::HttpRequest() : HttpHeaderDelegator(_header) {
    }
	
	HttpRequest::~HttpRequest() {
	}
	
    void HttpRequest::clear() {
        _header.clear();
        clearTransfer();
    }
    void HttpRequest::setHeader(const HttpHeader & header) {
        _header.setHeader(header);
    }
	string HttpRequest::getMethod() const {
		return _header.getMethod();
	}
	string HttpRequest::getProtocol() const {
		return _header.getProtocol();
	}
	void HttpRequest::setHost(const std::string & host) {
		_header.setHost(host);
	}
	string HttpRequest::getPath() const {
		return _header.getPath();
	}
	void HttpRequest::setPath(const string & path) {
		_header.setPath(path);
	}
	string HttpRequest::getRawPath() const {
		return _header.getRawPath();
	}

    vector<string> HttpRequest::getParameterNames() {
        return _header.getParameterNames();
    }
	
	string HttpRequest::getParameter(const string & name) {
		return _header.getParameter(name);
	}
	
	string HttpRequest::getParameter(const char * name) {
		return _header.getParameter(name);
	}
	
	vector<string> HttpRequest::getParameters(string & name) {
		return _header.getParameters(name);
	}
	
	HttpRequestHeader & HttpRequest::header() {
		return _header;
	}

	bool HttpRequest::isWwwFormUrlEncoded() {
		return (_header.getContentType().empty()
				|| _header.getContentType() == "application/x-www-form-urlencoded");
	}
	void HttpRequest::parseWwwFormUrlencoded() {
		if (transfer.nil()) {
			return;
		}
		string data = ((StringDataSink*)&transfer->sink())->data();
		_header.parseQuery(data);
	}
	
    UTIL::AutoRef<DataTransfer> HttpRequest::getTransfer() {
        return transfer;
    }
	
    void HttpRequest::setTransfer(UTIL::AutoRef<DataTransfer> transfer) {
        this->transfer = transfer;
    }
	
    void HttpRequest::clearTransfer() {
        transfer = NULL;
    }
	
    void HttpRequest::setRemoteAddress(const InetAddress & remoteAddress) {
        this->remoteAddress = remoteAddress;
    }
	
    InetAddress & HttpRequest::getRemoteAddress() {
        return remoteAddress;
    }
	
	void HttpRequest::setLocalAddress(const InetAddress & localAddress) {
		this->localAddress = localAddress;
	}
	
	InetAddress & HttpRequest::getLocalAddress() {
		return localAddress;
	}
}
