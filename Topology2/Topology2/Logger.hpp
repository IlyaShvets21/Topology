#pragma once

#include <QObject>
#include <QString>

class Logger : public QObject {
	Q_OBJECT
public:
	static Logger& instance();

	void log(const QString& message);

signals:
	void messageLogged(const QString& message);

private:
	explicit Logger(QObject* parent = nullptr) : QObject(parent) {}
	Logger(const Logger&) = delete;
	Logger& operator = (const Logger&) = delete;
};