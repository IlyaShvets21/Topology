#include "ViewBTree.hpp"

#include <QWheelEvent>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsRectItem>

ViewBTree::ViewBTree(QWidget* parent) : QGraphicsView(parent) {
	setRenderHint(QPainter::Antialiasing);
	setDragMode(QGraphicsView::NoDrag);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

void ViewBTree::wheelEvent(QWheelEvent* e) {
	const double factor = (e->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
	scale(factor, factor);
	e->accept();
}

void ViewBTree::mousePressEvent(QMouseEvent* e) {
	if (e->button() == Qt::MiddleButton ||
		(e->button() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier))) {
		dragging = true;
		lastPos = e->pos();
		setCursor(Qt::ClosedHandCursor);
		e->accept();
		return;
	}
	QGraphicsView::mousePressEvent(e);
}

void ViewBTree::mouseMoveEvent(QMouseEvent* e) {
	if (dragging) {
		const QPoint delta = e->pos() - lastPos;
		lastPos = e->pos();
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
		verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
		e->accept();
		return;
	}
	QGraphicsView::mouseMoveEvent(e);
}

void ViewBTree::mouseReleaseEvent(QMouseEvent* e) {
	if (dragging) {
		dragging = false;
		setCursor(Qt::ArrowCursor);
		e->accept();
		return;
	}
	QGraphicsView::mouseReleaseEvent(e);
}

ViewBTreeDialog::ViewBTreeDialog(const BTreeData& tree, QWidget* parent) : QDialog(parent) {
	setWindowTitle("B-Tree");
	resize(600, 400);

	view = new ViewBTree(this);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(view);

	buildScene(tree);
}

void ViewBTreeDialog::buildScene(const BTreeData& tree) {
	auto* scene = new QGraphicsScene(this);
	view->setScene(scene);

	if (tree.root < 0 || tree.nodes.isEmpty()) {
		scene->addText("B-Tree is Empty");
		return;
	}

	QVector<int> depth(tree.nodes.size(), 0);

	std::function<void(int, int)> assignDepth = [&](int u, int d) {
		if (u < 0) return;
		depth[u] = d;
		assignDepth(tree.nodes[u].left, d + 1);
		assignDepth(tree.nodes[u].right, d + 1);
		};
	assignDepth(tree.root, 0);

	QVector<double> xPos(tree.nodes.size(), 0.0);
	double nextX = 0.0;

	std::function<void(int)> inorder = [&](int u) {
		if (u < 0) return;
		inorder(tree.nodes[u].left);
		xPos[u] = nextX;
		nextX += 120.0;
		inorder(tree.nodes[u].right);
		};
	inorder(tree.root);

	const double levelHeight = 90.0;
	const double nodeW = 70.0;
	const double nodeH = 40.0;

	auto nodePos = [&](int u) {
		return QPointF(xPos[u] + nodeW / 2.0, depth[u] * levelHeight + nodeH / 2.0);
		};

	for (int u = 0; u < tree.nodes.size(); ++u) {
		if (u == tree.root) continue;
		for (int p = 0; p < tree.nodes.size(); ++p) {
			if (tree.nodes[p].left == u || tree.nodes[p].right == u) {
				QPointF a = nodePos(p);
				QPointF b = nodePos(u);
				scene->addLine(QLineF(a, b), QPen(Qt::darkGray, 1.5));
				break;
			}
		}
	}

	for (int u = 0; u < tree.nodes.size(); ++u) {
		const BTreeNode& node = tree.nodes[u];
		QPointF pos = nodePos(u);

		auto* rect = new QGraphicsRectItem(pos.x() - nodeW / 2.0, pos.y() - nodeH / 2.0, nodeW, nodeH);
		QColor fill = (u == tree.root) ? QColor(255, 220, 120) : QColor(200, 230, 255);

		if (node.rotate) fill = QColor(255, 180, 180);
		rect->setBrush(fill);
		rect->setPen(QPen(Qt::black, 1.2));
		scene->addItem(rect);

		auto* text = new QGraphicsTextItem(node.id);
		QFont f = text->font();
		f.setBold(true);
		text->setFont(f);
		QRectF tb = text->boundingRect();
		text->setPos(pos.x() - tb.width() / 2.0,
			pos.y() - tb.height() / 2.0);
		scene->addItem(text);

		auto* dims = new QGraphicsTextItem(QString("%1x%2").arg(node.width).arg(node.height));
		QFont f2 = dims->font();
		f2.setPointSize(7);
		dims->setFont(f2);
		dims->setDefaultTextColor(Qt::darkGray);
		QRectF db = dims->boundingRect();
		dims->setPos(pos.x() - db.width() / 2.0, pos.y() + nodeH / 2.0);
		scene->addItem(dims);
	}

	QRectF bounds = scene->itemsBoundingRect();   
	view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	view->scale(10, 10);
}