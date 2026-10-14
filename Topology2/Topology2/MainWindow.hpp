#pragma once

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override = default;

private slots:
	void onRunClicked();
	void onClearClicked();
	void onBrowseClicked();
	void appendLog(const QString& message);

private:
	QPushButton* runButton_ = nullptr;
	QPushButton* clearButton_ = nullptr;
	QPushButton* browseButton_ = nullptr;
	QPlainTextEdit* logView_ = nullptr;
	QLabel* statusLabel_ = nullptr;
	QLabel* fileLabel_ = nullptr;

	QString currentPath_;
};