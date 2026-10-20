#include "Placer.hpp"
#include <unordered_map>
#include <iostream>

/************************************************************************************
 *  @brief  Функция для построения sequence_pair 
 ***********************************************************************************/
SequencePairData buildSequencePair(const FileData& data) {
	const int n = data.blocks.size();

	const QHash<QString, int> posA = [&] {
		QHash<QString, int> m;
		m.reserve(data.sequencePairData.A.size() * 2);
		for (int i = 0; i < data.sequencePairData.A.size(); ++i) {
			m[data.sequencePairData.A[i]] = i;

		}
		return m;
		}();
	const QHash<QString, int> posB = [&] {
		QHash<QString, int> m;
		m.reserve(data.sequencePairData.B.size() * 2);
		for (int i = 0; i < data.sequencePairData.B.size(); ++i) {
			m[data.sequencePairData.B[i]] = i;
		}
		return m;
		}();

	QVector<int> posAIdx(n), posBIdx(n);
	for (int i = 0; i < n; ++i) {
		posAIdx[i] = posA.value(data.blocks[i].id);
		posBIdx[i] = posB.value(data.blocks[i].id);
	}

	SequencePairData spd;
	spd.H.resize(n);
	spd.V.resize(n);

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (i == j) continue;
			const bool aBefore = posAIdx[i] < posAIdx[j];
			const bool bBefore = posBIdx[i] < posBIdx[j];
			if (aBefore && bBefore) {
				spd.H[i].push_back(j);
			}
			else if (aBefore && !bBefore) {
				spd.V[i].push_back(j);
			}
		}
	}
	return spd;
}

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
	ctx.seq_pair = buildSequencePair(data);
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
				total += std::sqrt(dx * dx + dy * dy);
			}
		}
	}
	return total;
}

/************************************************************************************
 *  @brief  Функция минимизации плотности упаковки
 ***********************************************************************************/
PlacementImprovements improve(const PlacementContext& ctx, QVector<int>& w, QVector<int>& h) {
	const int n = ctx.n;

	PlacementImprovements imp;
	imp.x = longestPath(n, ctx.seq_pair.H, w);
	imp.y = longestPath(n, ctx.seq_pair.V, h);

	int maxX = 0, maxY = 0;
	for (int i = 0; i < n; ++i) {
		maxX = std::max(maxX, imp.x[i] + w[i]);
		maxY = std::max(maxY, imp.y[i] + h[i]);
	}
	imp.totalWidth = maxX;
	imp.totalHeight = maxY;
	imp.area = 1LL * maxX * maxY;
	imp.hpwl = scoreHPWL(ctx, imp.x, imp.y, w, h);
	return imp;
}

/************************************************************************************
 *  @brief  Функция размещения блоков внутри рабочего поля
 ***********************************************************************************/
PlacementResult place(const FileData& data) {
	const auto& blocks = data.blocks;
	const int n = blocks.size();

	if (n == 0) {
		PlacementResult result;
		result.minX = result.minY = result.maxX = result.maxY = 0;
		result.totalWidth = result.totalHeight = 0;
		result.area = 0;
		result.hpwl = 0.0;
		return result;
	}

	const PlacementContext ctx = buildContext(data);

	QVector<int> w(n), h(n);
	QVector<bool> rotate(n, false);

	for (int i = 0; i < n; ++i) {
		w[i] = blocks[i].width;
		h[i] = blocks[i].height;
	}

	PlacementImprovements best = improve(ctx, w, h);

	if (data.decoderParamsData.allowRotationDuringDecode == true) {
		if (data.decoderParamsData.placementStrategy == "bottom_left") {
			bool improved = true;
			while (improved) {
				improved = false;

				for (int i = 0; i < n; ++i) {
					if (!blocks[i].rotationAllowed) continue;

					std::swap(w[i], h[i]);
					PlacementImprovements candidate = improve(ctx, w, h);

					if (candidate.area < best.area) {
						best = candidate;
						rotate[i] = !rotate[i];
						improved = true;
					}
					else {
						std::swap(w[i], h[i]);
					}
				}
			}
		}
		else if (data.decoderParamsData.placementStrategy == "min_hpwl") {
			bool improved = true;
			while (improved) {
				improved = false;

				for (int i = 0; i < n; ++i) {
					if (!blocks[i].rotationAllowed) continue;

					std::swap(w[i], h[i]);
					PlacementImprovements candidate = improve(ctx, w, h);

					if (candidate.hpwl < best.hpwl) {
						best = candidate;
						rotate[i] = !rotate[i];
						improved = true;

					}
					else {
						std::swap(w[i], h[i]);
					}
				}
			}
		}
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

	return result;
}