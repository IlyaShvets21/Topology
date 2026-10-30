#include "SequencePairDecoder.hpp"

/************************************************************************************
 *  @brief  Функция для построения sequence_pair
 ***********************************************************************************/
SequencePairData SequencePairDecoder::buildSequencePair(const FileData& data) {
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
