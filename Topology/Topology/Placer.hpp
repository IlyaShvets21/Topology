/************************************************************************************
 *  @file   Placer.hpp
 *  @brief  Файл содержит необходимые данные, для размещения блоков по информации,
 *			прочитанной из файла входных данных
 ************************************************************************************/
#include "Reader.hpp"
#include <vector>

#pragma once

struct PlacementImprovements {
	std::vector<int> x, y;
	int totalWidth = 0;
	int totalHeight = 0;
	long long area = 0;
};

/************************************************************************************
 *  @brief  Структура данных для результата размещения блоков на кристалле
 ***********************************************************************************/
struct PlacementResult {
	struct PlacedBlock {
		std::string id;
		int x = 0, y = 0;
		int width = 0, height = 0;
		bool rotate = false;
	};

	std::vector<PlacedBlock> blocks;

	int minX = 0, minY = 0;
	int maxX = 0, maxY = 0;
	int totalWidth = 0, totalHeight = 0;
	long long area = 0;
};

PlacementResult place(const FileData& data);
PlacementImprovements improve(const FileData& data, std::vector<int>& w, std::vector<int>& h);

