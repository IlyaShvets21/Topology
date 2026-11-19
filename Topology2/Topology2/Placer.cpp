#include "Placer.hpp"
#include "SequencePairDecoder.hpp"
#include <unordered_map>
#include <iostream>

/************************************************************************************
 *  @brief  Функция для построения хэша для блоков
 ***********************************************************************************/
QHash<QString, int> buildBlockIndex(const QVector<BlockData>& blocks) {
	QHash<QString, int> index;
	index.reserve(blocks.size() * 2);
	for (int i = 0; i < blocks.size(); ++i) {
		index[blocks[i].id] = i;
	}
	return index;
}

/************************************************************************************
 *  @brief  Функция для построения вектора для соединений
 ***********************************************************************************/
QVector<IndexedNet> buildIndexNets(const FileData& data, const QHash<QString, int>& index) {
	QVector<IndexedNet> result;
	result.reserve(data.nets.size());
	for (const NetsData& net : data.nets) {
		IndexedNet ni;
		ni.block_ids.reserve(net.block_ids.size());
		for (const QString& id : net.block_ids) {
			const auto it = index.find(id);
			if (it != index.end()) {
				ni.block_ids.push_back(it.value());
			}		
		}
		result.push_back(ni);
	}
	return result;
}

/************************************************************************************
 *  @brief  Функция для построения начального размещения
 ***********************************************************************************/
PlacementContext buildContext(const FileData& data) {
	PlacementContext ctx;
	ctx.n = data.blocks.size();
	ctx.seq_pair = SequencePairDecoder::buildSequencePair(data);
	ctx.blockIndex = buildBlockIndex(data.blocks);
	ctx.nets = buildIndexNets(data, ctx.blockIndex);
	return ctx;
}

/************************************************************************************
 *  @brief  Функция для построения самого длинного пути из блоком. Предназначен для 
 *			того, чтобы при размещении блоки не налезли друг на друга
 ***********************************************************************************/
QVector<int> longestPath(int n, const QVector<QVector<int>>& edges, const QVector<int>& weight) {
	QVector<int> indeg(n, 0);
	for (int u = 0; u < n; ++u) {
		for (int v : edges[u]) ++indeg[v];
	}

	QVector<int> coord(n, 0);
	QVector<int> queue;
	queue.reserve(n);
	for (int i = 0; i < n; ++i) {
		if (indeg[i] == 0) queue.push_back(i);
	}

	int head = 0;
	while (head < queue.size()) {
		const int u = queue[head++];
		for (int v : edges[u]) {
			coord[v] = std::max(coord[v], coord[u] + weight[u]);
			if (--indeg[v] == 0) queue.push_back(v);
		}
	}
	return coord;
}

/************************************************************************************
 *  @brief  Функция минимизации суммарной HPWL
 ***********************************************************************************/
static double scoreHPWL(const PlacementContext& ctx, const QVector<int>& x, const QVector<int>& y,
	const QVector<int>& w, const QVector<int>& h) {
	double total = 0.0;

	for (const IndexedNet& net : ctx.nets) {
		const int k = net.block_ids.size();
		if (k < 2) continue;

		QVector<double> cx(k), cy(k);
		for (int i = 0; i < k; ++i) {
			const int j = net.block_ids[i];
			cx[i] = x[j] + w[j] * 0.5;
			cy[i] = y[j] + h[j] * 0.5;
		}

		for (int a = 0; a < k; ++a) {
			for (int b = a + 1; b < k; ++b) {
				const double dx = cx[a] - cx[b];
				const double dy = cy[a] - cy[b];
				total += abs(dx) + abs(dy);
			}
		}
	}
	return total;
}

PlacementImprovements improve(const PlacementContext& ctx, QVector<int>& w, QVector<int>& h,
	bool useBTree, const BTreeData& tree, int chipW, int chipH, const FileData& data, const QVector<bool>& rotated) {
	const int n = ctx.n;
	PlacementImprovements imp;

	if (useBTree) {
		QVector<int> spX = longestPath(n, ctx.seq_pair.H, w);
		QVector<int> spY = longestPath(n, ctx.seq_pair.V, h);

		BTreeData layoutTree = BTreeDecoder::buildBTreeFromLayout(data, spX, spY, w, h, rotated);

		BTreeDecoder::decodeBTree(layoutTree, w, h, imp.x, imp.y);

		for (int i = 0; i < n; ++i) {
			if (imp.x[i] != spX[i] || imp.y[i] != spY[i]) {
				Logger::instance().log(
					QString("Warning: B-tree decode mismatch at block %1: "
						"expected (%2, %3), got (%4, %5)")
					.arg(i).arg(spX[i]).arg(spY[i]).arg(imp.x[i]).arg(imp.y[i]));
				imp.x = spX;
				imp.y = spY;
				break;
			}
		}
	}
	else {
		imp.x = longestPath(n, ctx.seq_pair.H, w);
		imp.y = longestPath(n, ctx.seq_pair.V, h);
	}

	int maxX = 0, maxY = 0;
	for (int i = 0; i < n; ++i) {
		maxX = std::max(maxX, imp.x[i] + w[i]);
		maxY = std::max(maxY, imp.y[i] + h[i]);
	}
	imp.totalWidth = maxX;
	imp.totalHeight = maxY;
	imp.area = 1LL * maxX * maxY;
	imp.hpwl = scoreHPWL(ctx, imp.x, imp.y, w, h);

	imp.fitsChip = true;
	for (int i = 0; i < n; ++i) {
		if (imp.x[i] < 0 || imp.y[i] < 0 ||
			imp.x[i] + w[i] > chipW ||
			imp.y[i] + h[i] > chipH) {
			imp.fitsChip = false;
			break;
		}
	}

	return imp;
}

PlacementResult place(const FileData& data) {
	const auto& blocks = data.blocks;
	const int n = blocks.size();
	const int chipW = data.designData.chipWidth;
	const int chipH = data.designData.chipHeight;

	if (n == 0) {
		PlacementResult result;
		result.minX = result.minY = result.maxX = result.maxY = 0;
		result.totalWidth = result.totalHeight = 0;
		result.area = 0;
		result.hpwl = 0.0;
		return result;
	}

	const PlacementContext ctx = buildContext(data);
	const bool useBTree = data.decoderParamsData.useBTree;

	BTreeData tree;
	if (useBTree) {
		tree = BTreeDecoder::buildBTree(data, ctx.seq_pair);
	}

	QVector<int> w(n), h(n);
	QVector<bool> rotate(n, false);

	for (int i = 0; i < n; ++i) {
		w[i] = blocks[i].width;
		h[i] = blocks[i].height;
	}

	PlacementImprovements best = improve(ctx, w, h, useBTree, tree, chipW, chipH, data, rotate);

	bool haveValidPlacement = best.fitsChip;

	if (data.decoderParamsData.allowRotationDuringDecode == true) {
		const bool byHpwl =	(data.decoderParamsData.placementStrategy == "min_hpwl");
		bool improved = true;
		while (improved) {
			improved = false;

			for (int i = 0; i < n; ++i) {
				if (!blocks[i].rotationAllowed) continue;

				std::swap(w[i], h[i]);
				PlacementImprovements candidate = improve(ctx, w, h, useBTree, tree, chipW, chipH, data, rotate);


				bool accept = false;

				if (candidate.fitsChip) {
					if (!haveValidPlacement) {
						accept = true;
					}
					else if (byHpwl) {
						accept = (candidate.hpwl < best.hpwl);
					}
					else {
						accept = (candidate.area < best.area);
					}
				}
				else {
					if (!haveValidPlacement) {
						if (byHpwl) {
							accept = (candidate.hpwl < best.hpwl);
						}
						else {
							accept = (candidate.area < best.area);
						}
					}
					else {
						accept = false;
					}
				}

				if (accept) {
					best = candidate;
					rotate[i] = !rotate[i];
					improved = true;

					if (candidate.fitsChip) {
						haveValidPlacement = true;
					}
				}
				else {
					std::swap(w[i], h[i]); 
				}
			}
		}
	}

	if (!best.fitsChip) {
		Logger::instance().log("Error: No valid placement found that fits the chip. "
			"All configurations exceed the chip boundaries.");
	}

	PlacementResult result;
	result.blocks.reserve(n);

	int minX = INT_MAX, minY = INT_MAX;
	int maxX = INT_MIN, maxY = INT_MIN;

	for (int i = 0; i < n; ++i) {
		PlacementResult::PlacedBlock p;
		p.id = blocks[i].id;
		p.x = best.x[i];
		p.y = best.y[i];
		p.width = w[i];
		p.height = h[i];
		p.rotate = rotate[i];
		result.blocks.push_back(p);

		minX = std::min(minX, p.x);
		minY = std::min(minY, p.y);
		maxX = std::max(maxX, p.x + p.width);
		maxY = std::max(maxY, p.y + p.height);
	}

	if (n == 0) { minX = minY = maxX = maxY = 0; }

	result.minX = minX;
	result.minY = minY;
	result.maxX = maxX;
	result.maxY = maxY;

	result.totalWidth = maxX - minX;
	result.totalHeight = maxY - minY;

	result.area = 1LL * result.totalWidth * result.totalHeight;
	result.hpwl = best.hpwl;

	if (useBTree) {
		result.bTree = tree;
	}

	return result;
}