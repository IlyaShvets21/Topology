/************************************************************************************
 *  @file   ViewPlacement.hpp
 *  @brief  Файл содержит необходимые данные, для отбражения размещенных блоков,
 *			по информации прочитанной из файла входных данных
 ************************************************************************************/
#pragma once

#include "Placer.hpp"

#include <QWidget>
#include <QWheelEvent>
#include <QGraphicsView>

class ViewPlacement : public QWidget {
	Q_OBJECT
public:
	explicit ViewPlacement(QWidget* parent = nullptr);

	void setPlacement(const PlacementResult& result, const DesignData& design, const QVector<NetsData>& nets);

	void clear();

protected:
	void paintEvent(QPaintEvent* e) override;
	void wheelEvent(QWheelEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

private:
	PlacementResult		placement;
	DesignData			designView;
	QVector<NetsData>	netsView;
	bool				hasData		= false;

	bool				showNets	= true;
	QHash<QString, int> idToIndex;

	double				scale		= 1.0;
	QPointF				offset;
	QRectF				viewport;

	bool dragging = false;
	QPoint lastPos;

	QPointF worldToScreen(double x, double y) const;
	void fitToView();

public slots:
	void setShowNets(bool on); 
};
