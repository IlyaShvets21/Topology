/************************************************************************************
 *  @file   MainWindow.hpp
 *  @brief  Файл содержит необходимые данные для наполнения функционалом диалогового 
 *			окна программы
 ************************************************************************************/
#pragma once

#include "Logger.hpp"
#include "Reader.hpp"
#include "Placer.hpp"
#include "Writer.hpp"
#include "ViewPlacement.hpp"
#include "ViewBTree.hpp"

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
	void onRunClicked		();
	void onClearClicked		();
	void onBrowseClicked	();
	void onViewClicked		();
	void onShowBTree		();
	void appendLog			(const QString& message);

private:
	QPushButton		*runButton		= nullptr;
	QPushButton		*clearButton	= nullptr,
					*browseButton	= nullptr,
					*bTreeButton	= nullptr,
					*viewButton 	= nullptr;
	QPlainTextEdit  *logView		= nullptr;
	QLabel			*statusLabel	= nullptr,
					*fileLabel		= nullptr;

	ViewPlacement	*viewer			= nullptr;
	QString			currentPath;

	bool			hasResult		= false;
	PlacementResult lastResult;
	FileData		lastData;
};