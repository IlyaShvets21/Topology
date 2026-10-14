/************************************************************************************
 *  @file   Reader.hpp
 *  @brief  Файл содержит необходимые данные, для считывание информации из
 *			файла входных данных
 ************************************************************************************/
#include <string>
#include <vector>
#include <QMetaObject>
#include <QString>
#include <QVector>

#pragma once

enum class Section : int {
	Undefined = 0,
	Design,
	Blocks,
	SequencePair,
	Nets,
	DecoderParams
};

/************************************************************************************
 *  @brief  Структура данных для блока design
 ***********************************************************************************/
struct DesignData {
	int chipWidth = 0;
	int chipHeight = 0;
	int gridSize = 0;
};

/************************************************************************************
 *  @brief  Структура данных для блока blocks
 ***********************************************************************************/
struct BlockData {
	QString id;
	int width = 0;
	int height = 0;
	bool rotationAllowed = false;
};

/************************************************************************************
 *  @brief  Структура данных для блока sequence_pair
 ***********************************************************************************/
struct SequencePair {
	QVector<QString> A;
	QVector<QString> B;
};

/************************************************************************************
 *  @brief  Структура данных для блока nets
 ***********************************************************************************/
struct NetsData {
	QString net_id;
	QVector<QString> block_ids;
};

/************************************************************************************
 *  @brief  Структура данных для блока decoder_params
 ***********************************************************************************/
struct DecoderParams {
	bool useBTree = false;
	bool allowRotationDuringDecode = false;
	QString placementStrategy;
};

/************************************************************************************
 *  @brief  Структура данных для входного файла
 ***********************************************************************************/
struct FileData {
	DesignData designData;
	QVector<BlockData> blocks;
	SequencePair sequencePairData;
	QVector<NetsData> nets;
	DecoderParams decoderParamsData;
};

bool read(const QString& filename, FileData& data);

