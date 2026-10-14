/************************************************************************************
 *  @file   Writer.hpp
 *  @brief  Файл содержит необходимые данные, для создание выходного файла и
 *			записи в него результатов размещения
 ************************************************************************************/
#include "Reader.hpp"
#include "Placer.hpp"
#include "Logger.hpp"
#include <chrono>

bool write(const QString& filename, const PlacementResult& result, const FileData& data, std::chrono::duration<double> elasped);