#include "Logger.hpp"
#include <QMetaObject>

Logger& Logger::instance() {
	static Logger logger;
	return logger;
}

void Logger::log(const QString& message) {
	QMetaObject::invokeMethod(this, [this, message] {
		emit messageLogged(message);
		}, Qt::QueuedConnection);
}