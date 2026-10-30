#include "BTreeDecoder.hpp"

/************************************************************************************
 *  @brief  Функция построения B-tree из sequence_pair
 ***********************************************************************************/
BTreeData BTreeDecoder::buildBTree(const FileData& data, const SequencePairData& sequence_pair, const QHash<QString, int>& blockIndex) {
	const int n = data.blocks.size();
	BTreeData tree;
	tree.nodes.resize(n);
	tree.root = -1;

	if (n == 0) return tree;

	for (int i = 0; i < n; ++i) {
		BTreeNode& node = tree.nodes[i];
		node.id = data.blocks[i].id;
		node.width = data.blocks[i].width;
		node.height = data.blocks[i].height;
		node.left = -1;
		node.right = -1;
	}

	QVector<bool> hasPred(n, false);
	for (int u = 0; u < n; ++u) {
		for (int v : sequence_pair.H[u]) hasPred[v] = true;
		for (int v : sequence_pair.V[u]) hasPred[v] = true;
	}

	for (int i = 0; i < n; ++i) {
		if (!hasPred[i]) {
			tree.root = i;
			break;
		}
	}
	if (tree.root < 0) tree.root = 0;

	QVector<bool> visited(n, false);
	QVector<int> queue;
	queue.push_back(tree.root);
	visited[tree.root] = true;

	int head = 0;
	while (head < queue.size()) {
		int u = queue[head++];

		for (int v : sequence_pair.H[u]) {
			if (!visited[v]) {
				tree.nodes[u].left = v;
				visited[v] = true;
				queue.push_back(v);
				break;
			}
		}
		for (int v : sequence_pair.V[u]) {
			if (!visited[v]) {
				tree.nodes[u].right = v;
				visited[v] = true;
				queue.push_back(v);
				break;
			}
		}
	}

	for (int i = 0; i < n; ++i) {
		if (!visited[i]) {
			int cur = tree.root;
			while (tree.nodes[cur].right >= 0) {
				cur = tree.nodes[cur].right;
			}
			tree.nodes[cur].right = i;
			visited[i] = true;
		}
	}
	return tree;
}

/************************************************************************************
 *  @brief  Функция декодирования B-Tree в координаты
 ***********************************************************************************/
void BTreeDecoder::decodeBTree(const BTreeData& tree, const QVector<int>& w, const QVector<int>& h,
				 QVector<int>& x, QVector<int>& y) {
	const int n = tree.nodes.size();
	x.fill(0, n);
	y.fill(0, n);

	std::function<void(int, int, int)> dfs = [&](int u, int px, int py) {
		if (u < 0 || u >= x.size()) return;
		x[u] = px;
		y[u] = py;

		const int wu = tree.nodes[u].rotate ? h[u] : w[u];
		const int hu = tree.nodes[u].rotate ? w[u] : h[u];

		dfs(tree.nodes[u].left, px + wu, py);
		dfs(tree.nodes[u].right, px, py + hu);
		};
	dfs(tree.root, 0, 0);
}

/************************************************************************************
 *  @brief  Функция компактификации после размещения, для избавления от наложений
 ***********************************************************************************/
void BTreeDecoder::resolveOverlaps(QVector<int>& x, QVector<int>& y, const QVector<int>& w, const QVector<int>& h) {
	const int n = x.size();
	bool changed = true;
	int iter = 0;

	while (changed && iter++ < 180) {
		changed = false;
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				if (i == j) continue;
				if (x[i] < x[j] + w[j] && x[j] < x[i] + w[i] && y[i] < y[j] + h[j] && y[j] < y[i] + h[i]) {
					int dx = (x[j] + w[j]) - x[i];
					int dy = (y[j] + h[j]) - y[i];
					if (dx < dy) {
						x[i] += dx;
					}
					else {
						y[i] += dy;
					}
					changed = true;
				}
			}
		}
	}
}

