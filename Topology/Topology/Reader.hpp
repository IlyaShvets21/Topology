/************************************************************************************
 *  @file   Reader.hpp
 *  @brief  Файл содержит необходимые данные, для считывание информации из
 *			файла входных данных  
 ************************************************************************************/
#include <string>
#include <vector>

#pragma once

enum class Section : int {
	Undefined = 0,
	Design,
	Blocks,
	SequencePair,
	DecoderParams
};

/************************************************************************************
 *  @brief  Структура данных для блока design
 ***********************************************************************************/
struct DesignData {
	int chipWidth			= 0;
	int chipHeight			= 0;
	int gridSize			= 0;
	bool rotationAllowed	= false;
};

/************************************************************************************
 *  @brief  Структура данных для блока blocks
 ***********************************************************************************/
struct BlockData {
	std::string id			= "";
	int width				= 0;
	int height				= 0;
	bool rotationAllowed	= false;
};

/************************************************************************************
 *  @brief  Структура данных для блока sequence_pair
 ***********************************************************************************/
struct SequencePair {
	std::vector<std::string> A;
	std::vector<std::string> B;
};

/************************************************************************************
 *  @brief  Структура данных для блока decoder_params
 ***********************************************************************************/
struct DecoderParams {
	bool useBTree					= false;
	bool allowRotationDuringDecode	= false;
	std::string placementStrategy	= "";
};

/************************************************************************************
 *  @brief  Структура данных для входного файла
 ***********************************************************************************/
struct FileData {
	DesignData designData;
	std::vector<BlockData> blocks;
	SequencePair sequencePairData;
	DecoderParams decoderParamsData;
};

bool read(const std::string& filename, FileData& data);

