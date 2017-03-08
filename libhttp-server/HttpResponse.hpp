#ifndef __HTTP_RESPONSE_HPP__
#define __HTTP_RESPONSE_HPP__

#include <vector>
#include <string>
#include <map>

#include <liboslayer/os.hpp>
#include <liboslayer/AutoRef.hpp>
#include "HttpHeader.hpp"
#include "ChunkedReader.hpp"
#include "DataTransfer.hpp"

namespace HTTP {

	/**
	 * @brief http response
	 */
	class HttpResponse {
	private:
		HttpResponseHeader _header;
        UTIL::AutoRef<DataTransfer> transfer;
		bool _redirectRequested;
		std::string _redirectLocation;
		bool _forwardRequested;
		std::string _forwardLocation;
		std::map<std::string, std::string> props;
		
	public:
		HttpResponse();
		virtual ~HttpResponse();
        
        void clear();
		void setStatus(int statusCode);
		void setStatus(int statusCode, const std::string & statusString);
		int getStatusCode();
		void setParts(std::vector<std::string> &parts);
		void setContentLength(unsigned long long length);
		void setContentType(const std::string & type);
        bool completeContentTransfer();
		std::string getHeaderField(const std::string & name) const;
		std::string getHeaderFieldIgnoreCase(const std::string & name) const;
		HttpResponseHeader & header();
        void setTransfer(UTIL::AutoRef<DataTransfer> transfer);
        UTIL::AutoRef<DataTransfer> getTransfer();
		void clearTransfer();
		void setRedirect(const std::string & location);
		void cancelRedirect();
		void setForward(const std::string & location);
		void cancelForward();
		std::string getRedirectLocation();
		std::string getForwardLocation();
		bool redirectRequested();
		bool forwardRequested();
		std::string & operator[] (const std::string & name);
	};
}

#endif
