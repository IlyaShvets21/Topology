/************************************************************************************
 *  @file   Viewer.hpp
 *  @brief  Файл содержит необходимые данные, для отбражения размещенных блоков,
 *			по информации прочитанной из файла входных данных
 ************************************************************************************/
#pragma once

#include "Placer.hpp"

#include <QWidget>

class Viewer : public QWidget {
	Q_OBJECT
public:
	explicit Viewer(QWidget* parent = nullptr);

	void setPlacement(const PlacementResult& result, const DesignData& design, const QVector<NetsData>& nets);

	void clear();

protected:
	void paintEvent(QPaintEvent* event) override;

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

	QPointF worldToScreen(double x, double y) const;

public slots:
	void setShowNets(bool on); 
};