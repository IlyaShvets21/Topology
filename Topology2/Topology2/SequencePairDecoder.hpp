/************************************************************************************
 *  @file   SequencePairDecoder.hpp
 *  @brief  Файл содержит необходимые данные, для формирования информации о 
 *			расположении блоков по парной последовательности
 ************************************************************************************/
#pragma once

#include "Reader.hpp"

#include <QVector>
#include <QHash>

/************************************************************************************
 *  @brief  Структура данных для графов, построенных из sequence_pair
 ***********************************************************************************/
struct SequencePairData {
	QVector<QVector<int>> H;
	QVector<QVector<int>> V;
};

class SequencePairDecoder {
public:
	static SequencePairData buildSequencePair(const FileData& data);
};
