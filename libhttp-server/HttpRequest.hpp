#ifndef __HTTP_REQUEST_HPP__
#define __HTTP_REQUEST_HPP__

#include <vector>
#include <string>
#include <map>

#include <liboslayer/os.hpp>
#include <liboslayer/AutoRef.hpp>
#include "HttpHeader.hpp"
#include "HttpHeaderDelegator.hpp"
#include "ChunkedReader.hpp"
#include "Packet.hpp"
#include "DataTransfer.hpp"
#include "Cookie.hpp"
#include "HttpRange.hpp"

namespace http {

	/**
	 * @brief http request
	 */
	class HttpRequest : public HttpHeaderDelegator {
	private:
        HttpRequestHeader _header;
        osl::AutoRef<DataTransfer> transfer;
        osl::InetAddress remoteAddress;
		osl::InetAddress localAddress;

	public:
        HttpRequest();
		virtual ~HttpRequest();
		
        virtual void clear();
        void setHeader(const HttpHeader & header);
		std::string getMethod() const;
		std::string getProtocol() const;
		void setHost(const std::string & host);
		std::string getPath() const;
		void setPath(const std::string & path);
		std::string getDirectory() const;
		std::string getRawPath() const;
        std::vector<std::string> getParameterNames();
		bool hasParameter(const std::string & name);
		std::string getParameter(const std::string & name);
		std::string getParameter(const char * name);
		std::vector<std::string> getParameters(const std::string & name);
		HttpRequestHeader & header();
		bool isWwwFormUrlEncoded();
		void parseWwwFormUrlencoded();
        osl::AutoRef<DataTransfer> getTransfer();
        void setTransfer(osl::AutoRef<DataTransfer> transfer);
        void clearTransfer();
        void setRemoteAddress(const osl::InetAddress & remoteAddress);
        osl::InetAddress & getRemoteAddress();
		void setLocalAddress(const osl::InetAddress & localAddress);
		osl::InetAddress & getLocalAddress();
		std::vector<Cookie> getCookies();
		std::string getCookie(const std::string & key);
		bool containsRange() const;
		HttpRange getRange() const;
		HttpRange parseRange(const std::string & range) const;
	};
}

#endif
