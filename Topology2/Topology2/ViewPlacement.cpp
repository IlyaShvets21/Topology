#include "ViewPlacement.hpp"

#include <QPainter>
#include <QVBoxLayout>
#include <QGraphicsView>

#include <algorithm>

ViewPlacement::ViewPlacement(QWidget* parent) : QWidget(parent) {
	setMinimumSize(300, 200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setAutoFillBackground(true);
	setMouseTracking(true);

	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(30, 30, 30));
	setPalette(pal);
}

void ViewPlacement::setShowNets(bool on) {
	showNets = on;
	update();
}

void ViewPlacement::fitToView() {
	if (!hasData) return;

	QRectF bbox(0, 0, designView.chipWidth, designView.chipHeight);

	const int margin = 20;
	const double availW = width() - 2 * margin;
	const double availH = height() - 2 * margin;

	if (availW <= 0 || availH <= 0) return;

	const double sx = availW / bbox.width();
	const double sy = availH / bbox.height();
	scale = std::min(sx, sy);

	offset = QPointF(margin + (availW - bbox.width() * scale) / 2.0 - bbox.x() * scale,
					 margin + (availH - bbox.height() * scale) / 2.0 - bbox.y() * scale
	);
}

int ViewPlacement::blockAt(const QPoint& screenPos) const {
	if (!hasData) return -1;

	const double worldX = (screenPos.x() - offset.x()) / scale;
	const double worldY = designView.chipHeight - (screenPos.y() - offset.y()) / scale;

	for (int i = 0; i < placement.blocks.size(); ++i) {
		const auto& b = placement.blocks[i];
		if (worldX >= b.x && worldX <= b.x + b.width &&
			worldY >= b.y && worldY <= b.y + b.height) {
			return i;
		}
	}
	return -1;
}

void ViewPlacement::updateConnectedBlocks() {
	connectedBlockIds.clear();
	if (selectedBlockId.isEmpty()) return;

	for (const NetsData& net : netsView) {
		if (net.block_ids.contains(selectedBlockId)) {
			for (const QString& id : net.block_ids) {
				if (id != selectedBlockId) {
					connectedBlockIds.insert(id);
				}
			}
		}
	}
}

QColor ViewPlacement::blockColor(const QString& id, bool rotated) const {
	if (selectedBlockId.isEmpty()) {
		return rotated ? QColor(0, 255, 0) : QColor(0, 0, 255);
	}

	if (id == selectedBlockId) {
		return QColor(255, 255, 0);
	}

	if (connectedBlockIds.contains(id)) {
		return QColor(255, 165, 0);
	}

	return QColor(80, 80, 80);
}

QColor ViewPlacement::netColor(const NetsData& net) const {
	if (selectedBlockId.isEmpty()) {
		return QColor(255, 0, 0);
	}
	if (isNetSelected(net)) {
		return QColor(255, 100, 0);
	}
	return QColor(80, 80, 80);
}

bool ViewPlacement::isNetSelected(const NetsData& net) const {
	if (selectedBlockId.isEmpty()) return false;
	return net.block_ids.contains(selectedBlockId);
}

void ViewPlacement::clearSelection() {
	selectedBlockId.clear();
	connectedBlockIds.clear();
	update();
}

void ViewPlacement::setPlacement(const PlacementResult& result, const DesignData& design, 
	const QVector<NetsData>& nets, const DecoderParams& params) {
	placement = result;
	designView = design;
	paramsData = params;
	netsView = nets;
	hasData = true;

	idToIndex.clear();
	idToIndex.reserve(placement.blocks.size() * 2);
	for (int i = 0; i < placement.blocks.size(); ++i) {
		idToIndex[placement.blocks[i].id] = i;
	}

	fitToView();
	update();
}

void ViewPlacement::clear() {
	hasData = false;
	update();
}

QPointF ViewPlacement::worldToScreen(double x, double y) const {
	return QPointF(offset.x() + x * scale, offset.y() + y * scale);
}

void ViewPlacement::paintEvent(QPaintEvent*) {
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

	const double drawW = chipW * scale;
	const double drawH = chipH * scale;

	const int gridSize = designView.gridSize > 0 ? designView.gridSize : 10;
	p.setPen(QPen(QColor(60, 60, 60), 1));

	for (int gx = 0; gx <= designView.chipWidth; gx += gridSize) {
		const double sxPos = worldToScreen(gx, 0).x();
		p.drawLine(QPointF(sxPos, offset.y()), QPointF(sxPos, offset.y() + drawH));
	}

	for (int gy = 0; gy <= designView.chipHeight; gy += gridSize) {
		const double syPos = worldToScreen(0, designView.chipHeight - gy).y();
		p.drawLine(QPointF(offset.x(), syPos), QPointF(offset.x() + drawW, syPos));
	}

	if (paramsData.useTracing) {
		const int step = designView.lineSize + designView.channelSize;

		p.setPen(QPen(QColor(255, 0, 0), 1));
		for (int y = 0; y <= designView.chipHeight; y += step) {
			const double syPos = worldToScreen(0, designView.chipHeight - y).y();
			p.drawLine(QPointF(offset.x(), syPos),
				QPointF(offset.x() + drawW, syPos));
		}

		if (designView.channelSize > 0) {
			for (int y = designView.lineSize; y <= designView.chipHeight; y += step) {
				const double syPos = worldToScreen(0, designView.chipHeight - y).y();
				p.drawLine(QPointF(offset.x(), syPos),
					QPointF(offset.x() + drawW, syPos));
			}
		}
	}

	p.setPen(QPen(QColor(200, 200, 200), 2));
	p.drawRect(QRectF(offset, QSizeF(drawW, drawH)));

	QFont small = font();
	small.setPointSize(8);
	p.setFont(small);

	for (const auto& b : placement.blocks) {
		const QPointF tl = worldToScreen(b.x, designView.chipHeight - b.y - b.height);
		const QRectF r(tl.x(), tl.y(), b.width * scale, b.height * scale);

		QColor fill = blockColor(b.id, b.rotate);
		p.setBrush(fill);
		p.setPen(QPen(QColor(20, 20, 20), 1));
		p.drawRect(r);

		if (r.width() > 20 && r.height() > 12) {
			p.setPen((b.id == selectedBlockId || connectedBlockIds.contains(b.id))
				? Qt::black : Qt::white);
			p.drawText(r, Qt::AlignCenter, b.id);
		}
	}

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

				centers.push_back(worldToScreen(cx, designView.chipHeight - cy));
			}

			if (centers.size() < 2) continue;

			QColor color = netColor(net);
			bool selected = isNetSelected(net);
			p.setPen(QPen(color, selected ? 2.0 : 1.0));

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

void ViewPlacement::wheelEvent(QWheelEvent* e) {
	if (!hasData) {
		e->ignore();
		return;
	}
	const QPointF cursor = e->posF();

	const QPointF worldBefore((cursor.x() - offset.x()) / scale, (cursor.y() - offset.y()) / scale);

	const double factor = (e->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
	scale = std::max(0.05,std::min(scale * factor , 50.0));

	offset = QPointF(cursor.x() - worldBefore.x() * scale, cursor.y() - worldBefore.y() * scale);

	update();
	e->accept();
}

void ViewPlacement::mousePressEvent(QMouseEvent* e) {
	if (e->button() == Qt::MiddleButton ||
		(e->button() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier))) {
		dragging = true;
		lastPos = e->pos();
		setCursor(Qt::ClosedHandCursor);
		e->accept();
		return;
	}

	if (e->button() == Qt::LeftButton && hasData) {
		const int idx = blockAt(e->pos());
		if (idx >= 0) {
			const QString& id = placement.blocks[idx].id;
			if (id == selectedBlockId) {
				selectedBlockId.clear();
				connectedBlockIds.clear();
			}
			else {
				selectedBlockId = id;
				updateConnectedBlocks();
			}
			update();
			e->accept();
			return;
		}
		else {
			if (!selectedBlockId.isEmpty()) {
				selectedBlockId.clear();
				connectedBlockIds.clear();
				update();
			}
		}
	}

	QWidget::mousePressEvent(e);
}

void ViewPlacement::mouseMoveEvent(QMouseEvent* e) {
	if (dragging) {
		const QPoint delta = e->pos() - lastPos;
		lastPos = e->pos();
		offset += delta;
		update();
		e->accept();
		return;
	}
	QWidget::mouseMoveEvent(e);
}

void ViewPlacement::mouseReleaseEvent(QMouseEvent* e) {
	if (dragging) {
		dragging = false;
		setCursor(Qt::ArrowCursor);
		e->accept();
		return;
	}
	QWidget::mouseReleaseEvent(e);
}
