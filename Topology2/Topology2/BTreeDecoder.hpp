/************************************************************************************
 *  @file   BTreeDecoder.hpp
 *  @brief  Файл содержит необходимые данные, для формирования информации о 
 *			расположении блоков по бинарному дереву
 ************************************************************************************/
#pragma once

#include "Reader.hpp"
#include "SequencePairDecoder.hpp"

#include <QVector>
#include <QString>

/************************************************************************************
 *  @brief  Структура данных для узлов B-tree
 ***********************************************************************************/
struct BTreeNode {
	QString				  id;
	int				      width		= 0,
					      height	= 0;
	bool				  rotate	= false;
	int					  x			= 0,
						  y			= 0;
	int					  left		= -1,
						  right		= -1;
};

/************************************************************************************
 *  @brief  Структура данных для построения B-Tree
 ***********************************************************************************/
struct BTreeData {
	QVector<BTreeNode>	  nodes;
	int					  root = -1;
};

class BTreeDecoder {
public:
	static BTreeData buildBTree(const FileData& data, const SequencePairData& sequence_pair, const QHash<QString, int>& blockIndex);
	static void decodeBTree(const BTreeData& tree, const QVector<int>& w, const QVector<int>& h,
		QVector<int>& x, QVector<int>& y);
	static void resolveOverlaps(QVector<int>& x, QVector<int>& y, const QVector<int>& w, const QVector<int>& h);
};

