#include "Logger.hpp"

#include <iostream>

namespace HTTP {

	using namespace std;

	class VerboseLogger : public Logger {
	public:
		virtual void logd(const char * msg) {
			cout << msg;
		}
		virtual void logd(const string & msg) {
			cout << msg;
		}
		virtual void loge(const char * msg) {
			cout << msg;
		}
		virtual void loge(const string & msg) {
			cout << msg;
		}
	};

	VerboseLogger verboseLogger;

	Logger::Logger() {
	}

	Logger::~Logger() {
	}

	Logger & Logger::getLogger() {
		return verboseLogger;
	}	
}
