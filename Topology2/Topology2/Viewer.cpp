#include "Viewer.hpp"

#include <QPainter>

Viewer::Viewer(QWidget* parent) : QWidget(parent) {
	setMinimumSize(300, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setAutoFillBackground(true);

	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(30, 30, 30));
	setPalette(pal);
}

void Viewer::setShowNets(bool on) {
	showNets = on;
	update();
}

void Viewer::setPlacement(const PlacementResult& result, const DesignData& design, const QVector<NetsData>& nets) {
	placement = result;
	designView = design;
	netsView = nets;
	hasData = true;

	idToIndex.clear();
	idToIndex.reserve(placement.blocks.size() * 2);
	for (int i = 0; i < placement.blocks.size(); ++i) {
		idToIndex[placement.blocks[i].id] = i;
	}
	update();
}

void Viewer::clear() {
	hasData = false;
	update();
}

QPointF Viewer::worldToScreen(double x, double y) const {
	return QPointF(offset.x() + x * scale, offset.y() + (designView.chipHeight - y) * scale);
}

void Viewer::paintEvent(QPaintEvent*) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	p.fillRect(rect(), QColor(30, 30, 30));

	if (!hasData) {
		p.setPen(QColor(150, 150, 150));
		p.drawText(rect(), Qt::AlignCenter, tr("No placement yet. \nRun and press View."));
		return;
	}

	const int margin = 20;
	viewport = rect().adjusted(margin, margin, -margin, -margin);

	const double chipW = std::max(1, designView.chipWidth);
	const double chipH = std::max(1, designView.chipHeight);

	const double sx = viewport.width() / chipW;
	const double sy = viewport.height() / chipH;

	scale = std::min(sx, sy);

	const double drawW = chipW * scale;
	const double drawH = chipH * scale;
	offset = QPointF(viewport.left() + (viewport.width() - drawW) * 0.5,
					 viewport.top() + (viewport.height() - drawH) * 0.5);

	const int gridSize = designView.gridSize > 0 ? designView.gridSize : 10;
	p.setPen(QPen(QColor(60, 60, 60), 1));

	for (int gx = 0; gx <= designView.chipWidth; gx += gridSize) {
		const double sxPos = worldToScreen(gx, 0).x();
		p.drawLine(QPointF(sxPos, offset.y()), QPointF(sxPos, offset.y() + drawH));
	}

	for (int gy = 0; gy <= designView.chipHeight; gy += gridSize) {
		const double syPos = worldToScreen(0, gy).y();
		p.drawLine(QPointF(offset.x(), syPos), QPointF(offset.x() + drawW, syPos));
	}

	p.setPen(QPen(QColor(200, 200, 200), 2));
	p.drawRect(QRectF(offset, QSizeF(drawW, drawH)));

	QFont small = font();
	small.setPointSize(8);
	p.setFont(small);

	for (const auto& b : placement.blocks) {
		const QPointF tl = worldToScreen(b.x, b.y + b.height);
		const QRectF r(tl.x(), tl.y(), b.width * scale, b.height * scale);

		QColor fill(0, 0, 255);
		if (b.rotate) fill = QColor(0, 255, 0);
		p.setBrush(fill);
		p.setPen(QPen(QColor(20, 20, 20), 1));
		p.drawRect(r);

		if (r.width() > 20 && r.height() > 12) {
			p.setPen(Qt::white);
			p.drawText(r, Qt::AlignCenter, b.id);
		}
	}

	p.setPen(QColor(255, 0, 0));
	p.setBrush(Qt::NoBrush);

	if (showNets) {
		for (const NetsData& net : netsView) {
			const int k = net.block_ids.size();
			if (k < 2) continue;

			QVector<QPointF> centers;
			centers.reserve(k);

			for (const QString& id : net.block_ids) {
				const auto it = idToIndex.find(id);
				if (it == idToIndex.end()) continue;

				const auto& b = placement.blocks[it.value()];
				const double cx = b.x + b.width * 0.5;
				const double cy = b.y + b.height * 0.5;

				centers.push_back(worldToScreen(cx, cy));
			}

			if (centers.size() < 2) continue;

			for (int a = 0; a < centers.size(); ++a) {
				for (int b = a + 1; b < centers.size(); ++b) {
					p.drawLine(centers[a], centers[b]);
				}
			}
		}
	}

	p.setPen(QColor(180, 180, 180));
	p.setFont(font());
	const QString info = QString("Chip: %1 x %2 | Area: %3 | HPWL: %4").arg(designView.chipWidth).arg(designView.chipHeight)
		.arg(placement.area).arg(QString::number(placement.hpwl, 'f', 2));
	p.drawText(QRectF(0, 0, width(), margin), Qt::AlignLeft | Qt::AlignVCenter, info);
}