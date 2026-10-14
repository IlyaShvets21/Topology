#include "Placer.hpp"
#include <unordered_map>
#include <vector>
#include <iostream>

static std::unordered_map<std::string, int> buildIndex(const std::vector<BlockData>& blocks) {
	std::unordered_map<std::string, int> idx;
	for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
		idx[blocks[i].id] = i;
	}
	return idx;
}

static std::unordered_map<std::string, int>  buildOrder(const std::vector<std::string>& order) {
	std::unordered_map<std::string, int> pos;
	for (int i = 0; i < static_cast<int>(order.size()); ++i) {
		pos[order[i]] = i;
	}
	return pos;
}

std::vector<int> longestPath(int n, const std::vector<std::vector<int>>& edges, const std::vector<int>& weight) {
	std::vector<int> indeg(n, 0);
	for (int u = 0; u < n; ++u) {
		for (int v : edges[u]) ++indeg[v];
	}

	std::vector<int> coord(n, 0);
	std::vector<int> queue;
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

PlacementImprovements improve(const FileData& data, std::vector<int>& w, std::vector<int>& h) {
	const auto& blocks = data.blocks;
	const auto& A = data.sequencePairData.A;
	const auto& B = data.sequencePairData.B;
	const int n = static_cast<int>(blocks.size());

	const auto idx = buildIndex(blocks);
	const auto posA = buildOrder(A);
	const auto posB = buildOrder(B);

	std::vector<std::vector<int>> H(n), V(n);

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (i == j) continue;
			const std::string& X = blocks[i].id;
			const std::string& Y = blocks[j].id;

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
	return imp;
}

PlacementResult place(const FileData & data) {
	const auto& blocks = data.blocks;
	const int n = static_cast<int>(blocks.size());

	std::vector<int> w(n), h(n);
	std::vector<bool> rotate(n, false);

	for (int i = 0; i < n; ++i) {
		w[i] = blocks[i].width;
		h[i] = blocks[i].height;
	}

	PlacementImprovements best = improve(data, w, h);

	if (data.decoderParamsData.allowRotationDuringDecode == true && data.designData.rotationAllowed == true) {
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

	PlacementResult result;
	result.blocks.reserve(n);

	int minX = std::numeric_limits<int>::max();
	int minY = std::numeric_limits<int>::max();
	int maxX = std::numeric_limits<int>::min();
	int maxY = std::numeric_limits<int>::min();

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

	result.area = 1LL*result.totalWidth*result.totalHeight;

	return result;
}