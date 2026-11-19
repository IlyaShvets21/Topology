/************************************************************************************
 *  @file   Placer.hpp
 *  @brief  Файл содержит необходимые данные, для размещения блоков по информации,
 *			прочитанной из файла входных данных
 ************************************************************************************/
#pragma once

#include "Reader.hpp"
#include "SequencePairDecoder.hpp"
#include "BTreeDecoder.hpp"
#include "Logger.hpp"

#include <QVector>
#include <QString>
#include <QHash>

/************************************************************************************
 *  @brief  Структура данных для индексов соединений
 ***********************************************************************************/
struct IndexedNet {
	QVector<int>	      block_ids;
};

/************************************************************************************
 *  @brief  Структура данных для первоначального размещения
 ***********************************************************************************/
struct PlacementContext {
	SequencePairData      seq_pair;
	QHash<QString, int>   blockIndex;
	QVector<IndexedNet>   nets;
	int                   n				= 0;
};

/************************************************************************************
 *  @brief  Структура данных для конечного размещения
 ***********************************************************************************/
struct PlacementImprovements {
	QVector<int>		  x, y;
	int					  totalWidth	= 0,
						  totalHeight	= 0;
	long long			  area			= 0;
	double				  hpwl			= 0.0;

	bool			      fitsChip		= true;
};

/************************************************************************************
 *  @brief  Структура данных для результата размещения блоков на кристалле
 ***********************************************************************************/
struct PlacementResult {
	struct PlacedBlock {
		QString			  id;
		int				  x				= 0, 
						  y				= 0;
		int				  width			= 0, 
						  height		= 0;
		bool			  rotate		= false;
	};

	QVector<PlacedBlock>  blocks;

	int					  minX			= 0, 
						  minY			= 0;
	int					  maxX			= 0, 
						  maxY			= 0;
	int					  totalWidth    = 0, 
						  totalHeight   = 0;
	long long			  area			= 0;
	double				  hpwl	        = 0.0;

	BTreeData			  bTree;
};

/************************************************************************************
 *  @brief  Функция размещения блоков внутри рабочего поля
 *  @param  data - данные входного файла
 *	@return результаты размещения 
 ***********************************************************************************/
PlacementResult place(const FileData& data);

/************************************************************************************
 *  @brief  Функция минимизации плотности упаковки
 *  @param  ctx		 - текущее размещение
 *			w		 - список ширин блоков
 *			h		 - список высот блоков
 *			useBTree - флаг использования B-Tree
 *			tree	 - данные B-Tree
 * @return  размещение с минимизированной площадью/HPWL
 ***********************************************************************************/
PlacementImprovements improve(const PlacementContext& ctx,
	QVector<int>& w, QVector<int>& h,
	bool useBTree, const BTreeData& tree,
	int chipW, int chipH,
	const FileData& data,
	const QVector<bool>& rotated);

