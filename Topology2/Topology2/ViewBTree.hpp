/************************************************************************************
 *  @file   ViewBTree.hpp
 *  @brief  Файл содержит необходимые данные, для отбражения построенного,
 *			бинарного дерева
 ************************************************************************************/

#pragma once

#include "Placer.hpp"
#include "BTreeDecoder.hpp"

#include <QDialog>
#include <QGraphicsView>

class ViewBTree : public QGraphicsView {
	Q_OBJECT
public: 
	explicit ViewBTree(QWidget* parent = nullptr);

protected:
	void wheelEvent(QWheelEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

private:
	bool dragging = false;
	QPoint lastPos;
};

class ViewBTreeDialog : public QDialog {
	Q_OBJECT
public:
	explicit ViewBTreeDialog(const BTreeData& tree, QWidget* parent = nullptr);

private:
	ViewBTree* view = nullptr;
	void buildScene(const BTreeData& tree);
};