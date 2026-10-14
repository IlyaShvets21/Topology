#include "Placer.hpp"
#include <unordered_map>
#include <iostream>

#include <QHash>

static std::unordered_map<QString, int> buildIndex(const QVector<BlockData>& blocks) {
	std::unordered_map<QString, int> idx;
	for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
		idx[blocks[i].id] = i;
	}
	return idx;
}

static std::unordered_map<QString, int>  buildOrder(const QVector<QString>& order) {
	std::unordered_map<QString, int> pos;
	for (int i = 0; i < static_cast<int>(order.size()); ++i) {
		pos[order[i]] = i;
	}
	return pos;
}

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

	std::size_t head = 0;
	while (head < queue.size()) {
		const int u = queue[head++];
		for (int v : edges[u]) {
			coord[v] = std::max(coord[v], coord[u] + weight[u]);
			if (--indeg[v] == 0) queue.push_back(v);
		}
	}
	return coord;
}

static double computeHPWL(const FileData& data, const QVector<int>& x, const QVector<int>& y,
						  const QVector<int>& w, const QVector<int>& h) {
	QHash<QString, int> index;
	index.reserve(data.blocks.size() * 2);
	for (int i = 0; i < data.blocks.size(); ++i) {
		index[data.blocks[i].id] = i;
	}

	double total = 0.0;

	for (const NetsData& net : data.nets) {
		const int k = net.block_ids.size();
		if (k < 2) continue;

		QVector<double> cx(k), cy(k);
		int valid = 0;
		for (int j = 0; j < k; ++j) {
			const auto it = index.find(net.block_ids[j]);
			if (it == index.end()) continue;
			const int i = it.value();
			cx[valid] = x[i] + w[i] * 0.5;
			cy[valid] = y[i] + h[i] * 0.5;
			++valid;
		}

		for (int a = 0; a < valid; ++a) {
			for (int b = a + 1; b < valid; ++b) {
				const double dx = cx[a] - cx[b];
				const double dy = cy[a] - cy[b];
				total += std::sqrt(dx * dx + dy * dy);
			}
		}
	}

	return total;
}

PlacementImprovements improve(const FileData& data, QVector<int>& w, QVector<int>& h) {
	const auto& blocks = data.blocks;
	const int n = static_cast<int>(blocks.size());

	const auto idx = buildIndex(blocks);
	const auto posA = buildOrder(data.sequencePairData.A);
	const auto posB = buildOrder(data.sequencePairData.B);

	QVector<QVector<int>> H(n), V(n);

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (i == j) continue;
			const QString& X = blocks[i].id;
			const QString& Y = blocks[j].id;

			const bool aBefore = posA.at(X) < posA.at(Y);
			const bool bBefore = posB.at(X) < posB.at(Y);

			if (aBefore && bBefore) {
				H[i].push_back(j);
			}
			else if (aBefore && !bBefore) {
				V[i].push_back(j);
			}
		}
	}

	PlacementImprovements imp;
	imp.x = longestPath(n, H, w);
	imp.y = longestPath(n, V, h);

	int maxX = 0, maxY = 0;
	for (int i = 0; i < n; ++i) {
		maxX = std::max(maxX, imp.x[i] + w[i]);
		maxY = std::max(maxY, imp.y[i] + h[i]);
	}
	imp.totalWidth = maxX;
	imp.totalHeight = maxY;
	imp.area = 1LL * maxX * maxY;
	imp.hpwl = computeHPWL(data, imp.x, imp.y, w, h);
	return imp;
}

PlacementResult place(const FileData& data) {
	const auto& blocks = data.blocks;
	const int n = static_cast<int>(blocks.size());

	QVector<int> w(n), h(n);
	QVector<bool> rotate(n, false);

	for (int i = 0; i < n; ++i) {
		w[i] = blocks[i].width;
		h[i] = blocks[i].height;
	}

	PlacementImprovements best = improve(data, w, h);

	if (data.decoderParamsData.allowRotationDuringDecode == true) {
		if (data.decoderParamsData.placementStrategy == "bottom_left") {
			bool improved = true;
			while (improved) {
				improved = false;

				for (int i = 0; i < n; ++i) {
					if (!blocks[i].rotationAllowed) continue;

					std::swap(w[i], h[i]);
					PlacementImprovements candidate = improve(data, w, h);

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
					PlacementImprovements candidate = improve(data, w, h);

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