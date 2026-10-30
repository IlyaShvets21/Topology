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

void ViewPlacement::setPlacement(const PlacementResult& result, const DesignData& design, const QVector<NetsData>& nets) {
	placement = result;
	designView = design;
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

	p.setPen(QPen(QColor(200, 200, 200), 2));
	p.drawRect(QRectF(offset, QSizeF(drawW, drawH)));

	QFont small = font();
	small.setPointSize(8);
	p.setFont(small);

	for (const auto& b : placement.blocks) {
		const QPointF tl = worldToScreen(b.x, designView.chipHeight - b.y - b.height);
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

				centers.push_back(worldToScreen(cx, designView.chipHeight - cy));
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
