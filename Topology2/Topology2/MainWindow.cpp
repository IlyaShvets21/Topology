#include "MainWindow.hpp"
#include "Logger.hpp"
#include "Reader.hpp"
#include "Placer.hpp"
#include "Writer.hpp"

#include <QHBoxLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>

#include <chrono>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	auto* central = new QWidget(this);
	setCentralWidget(central);

	runButton_ = new QPushButton(tr("Run"), central);
	clearButton_ = new QPushButton(tr("Clear"), central);
	browseButton_ = new QPushButton(tr("Browse"), central);

	runButton_->setEnabled(false);

	auto* buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(runButton_);
	buttonLayout->addWidget(clearButton_);
	buttonLayout->addWidget(browseButton_);
	buttonLayout->addStretch();
	
	fileLabel_ = new QLabel(tr("File: <none>"), central);
	statusLabel_ = new QLabel(tr("Ready"), central);

	logView_ = new QPlainTextEdit(central);
	logView_->setReadOnly(true);
	logView_->setPlaceholderText(tr("Log output will appear here..."));
	logView_->setMaximumBlockCount(10000);

	QFont mono("consolas");
	mono.setStyleHint(QFont::Monospace);
	mono.setPointSize(10);
	logView_->setFont(mono);

	auto* mainLayout = new QVBoxLayout(central);
	mainLayout->addLayout(buttonLayout);
	mainLayout->addWidget(fileLabel_);
	mainLayout->addWidget(logView_, 1);
	mainLayout->addWidget(statusLabel_);

	connect(runButton_, &QPushButton::clicked, this, &MainWindow::onRunClicked);
	connect(clearButton_, &QPushButton::clicked, this, &MainWindow::onClearClicked);
	connect(browseButton_, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);

	connect(&Logger::instance(), &Logger::messageLogged, this,
		&MainWindow::appendLog, Qt::QueuedConnection);

	resize(700, 450);
	setWindowTitle(tr("Sequence Pair Interface"));

	appendLog(tr("Application started"));
}

void MainWindow::onRunClicked() {
	if (currentPath_.isEmpty()) {
		Logger::instance().log(tr("ERROR: no file selected"));
		return;
	}

	const QString defaultName = QFileInfo(currentPath_).absolutePath() + "/output.txt";

	const QString outputPath = QFileDialog::getSaveFileName(this, tr("Save result as"), defaultName,
		tr("Text files (*.txt)"));

	if (outputPath.isEmpty()) {
		Logger::instance().log(tr("Save cancelles by user"));
		return;
	}

	runButton_->setEnabled(false);
	statusLabel_->setText(tr("Running..."));

	FileData data;
	if (!read(currentPath_, data)) {
		Logger::instance().log(tr("ERROR: failed to read %1").arg(currentPath_));
		runButton_->setEnabled(true);
		statusLabel_->setText(tr("Error"));
		return;
	}

	auto start = std::chrono::steady_clock::now();
	const PlacementResult result = place(data);
	auto end = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration<double>(end - start);

	Logger::instance().log(tr("Placement done in %1 s").arg(QString::number(elapsed.count(), 'f', 3)));

	if (!write(outputPath, result, data, elapsed)) {
		Logger::instance().log(tr("ERROR: failed to write result"));
		runButton_->setEnabled(true);
		statusLabel_->setText(tr("Error"));
		return;
	}

	statusLabel_->setText(tr("Done"));
	runButton_->setEnabled(true);
}

void MainWindow::onClearClicked() {
	logView_->clear();
	Logger::instance().log(tr("Log cleared"));
}

void MainWindow::onBrowseClicked() {
	const QString path = QFileDialog::getOpenFileName(
		this, tr("Open input file"), QString(), tr("Text files (*.txt)"));

	if (path.isEmpty()) return;

	currentPath_ = path;
	fileLabel_->setText(tr("File: %1").arg(QFileInfo(path).fileName()));
	runButton_->setEnabled(true);
	statusLabel_->setText(tr("File selected"));

	Logger::instance().log(tr("Selected file: %1").arg(path));
}

void MainWindow::appendLog(const QString& message) {
	logView_->appendPlainText(message);

	auto* bar = logView_->verticalScrollBar();
	bar->setValue(bar->maximum());
}