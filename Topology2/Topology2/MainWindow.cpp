#include "MainWindow.hpp"

#include <QHBoxLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>
#include <QSplitter>
#include <QCheckBox>

#include <chrono>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	auto* central = new QWidget(this);
	setCentralWidget(central);

	runButton = new QPushButton(tr("Run"), central);
	clearButton = new QPushButton(tr("Clear"), central);
	browseButton = new QPushButton(tr("Browse"), central);
	viewButton = new QPushButton(tr("View"), central);

	QCheckBox* showNetsCheck = new QCheckBox(tr("Show nets"), central);
	showNetsCheck->setChecked(true);

	runButton->setEnabled(false);
	viewButton->setEnabled(false);

	auto* buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(runButton);
	buttonLayout->addWidget(clearButton);
	buttonLayout->addWidget(browseButton);
	buttonLayout->addWidget(viewButton);
	buttonLayout->addSpacing(20);
	buttonLayout->addWidget(showNetsCheck);
	buttonLayout->addStretch();
	
	fileLabel = new QLabel(tr("File: <none>"), central);
	statusLabel = new QLabel(tr("Ready"), central);

	logView = new QPlainTextEdit(central);
	logView->setReadOnly(true);
	logView->setPlaceholderText(tr("Log output will appear here..."));
	logView->setMaximumBlockCount(10000);

	QFont mono("consolas");
	mono.setStyleHint(QFont::Monospace);
	mono.setPointSize(10);
	logView->setFont(mono);

	viewer = new Viewer(central);

	auto* topLayout = new QVBoxLayout;
	topLayout->addLayout(buttonLayout);
	topLayout->addWidget(fileLabel);
	topLayout->addWidget(viewer, 1);

	auto* bottomLayout = new QVBoxLayout;
	bottomLayout->addWidget(new QLabel(tr("Log:"), central));
	bottomLayout->addWidget(logView, 1);

	auto* splitter = new QSplitter(Qt::Vertical, central);
	auto* topWidget = new QWidget;
	topWidget->setLayout(topLayout);
	auto* bottomWidget = new QWidget;
	bottomWidget->setLayout(bottomLayout);

	splitter->addWidget(topWidget);
	splitter->addWidget(bottomWidget);
	splitter->setStretchFactor(0, 3);
	splitter->setStretchFactor(1, 1);

	auto* mainLayout = new QVBoxLayout(central);
	mainLayout->addWidget(splitter, 1);
	mainLayout->addWidget(statusLabel);

	connect(showNetsCheck, &QCheckBox::toggled, viewer, &Viewer::setShowNets);
	connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);
	connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
	connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
	connect(viewButton, &QPushButton::clicked, this, &MainWindow::onViewClicked);

	connect(&Logger::instance(), &Logger::messageLogged, this,
		&MainWindow::appendLog, Qt::QueuedConnection);

	resize(700, 700);
	setWindowTitle(tr("Sequence Pair Interface"));

	appendLog(tr("Application started"));
}

void MainWindow::onRunClicked() {
	if (currentPath.isEmpty()) {
		Logger::instance().log(tr("ERROR: no file selected"));
		return;
	}

	const QString defaultName = QFileInfo(currentPath).absolutePath() + "/output.txt";

	const QString outputPath = QFileDialog::getSaveFileName(this, tr("Save result as"), defaultName,
		tr("Text files (*.txt)"));

	if (outputPath.isEmpty()) {
		Logger::instance().log(tr("Save cancelles by user"));
		return;
	}

	runButton->setEnabled(false);
	viewButton->setEnabled(false);
	statusLabel->setText(tr("Running..."));

	FileData data;
	if (!read(currentPath, data)) {
		Logger::instance().log(tr("ERROR: failed to read %1").arg(currentPath));
		runButton->setEnabled(true);
		statusLabel->setText(tr("Error"));
		return;
	}

	auto start = std::chrono::steady_clock::now();
	const PlacementResult result = place(data);
	auto end = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration<double>(end - start);

	if (!write(outputPath, result, data, elapsed)) {
		Logger::instance().log(tr("ERROR: failed to write result"));
		runButton->setEnabled(true);
		statusLabel->setText(tr("Error"));
		return;
	}

	lastData = data;
	lastResult = result;
	hasResult = true;
	viewButton->setEnabled(true);

	statusLabel->setText(tr("Done"));
	runButton->setEnabled(true);
	Logger::instance().log(tr("Placement ready. Press View to draw."));
}

void MainWindow::onClearClicked() {
	logView->clear();
	Logger::instance().log(tr("Log cleared"));
}

void MainWindow::onBrowseClicked() {
	const QString path = QFileDialog::getOpenFileName(
		this, tr("Open input file"), QString(), tr("Text files (*.txt)"));

	if (path.isEmpty()) return;

	currentPath = path;
	fileLabel->setText(tr("File: %1").arg(QFileInfo(path).fileName()));
	runButton->setEnabled(true);
	viewButton->setEnabled(false);
	hasResult = false;
	viewer->clear();
	statusLabel->setText(tr("File selected"));

	Logger::instance().log(tr("Selected file: %1").arg(path));
}

void MainWindow::onViewClicked() {
	if (!hasResult) {
		Logger::instance().log(tr("No Placement avilable. Run first."));
		return;
	}

	viewer->setPlacement(lastResult, lastData.designData, lastData.nets);
	Logger::instance().log(tr("View: %1 blocks, chip %2 x %3").arg(lastResult.blocks.size())
		.arg(lastData.designData.chipWidth).arg(lastData.designData.chipHeight));
}

void MainWindow::appendLog(const QString& message) {
	logView->appendPlainText(message);

	auto* bar = logView->verticalScrollBar();
	bar->setValue(bar->maximum());
}