#include "Writer.hpp"
#include "Reader.hpp"
#include <iostream>
#include <QTextStream>
#include <QFile>

/************************************************************************************
 *  @brief  Функция предназначена для вывода информации о размещении в log и в  
 *			выходной файл
 ************************************************************************************/
bool write(const QString& filename, const PlacementResult& result, const FileData& data, std::chrono::duration<double> elasped) {
	if (result.totalWidth > data.designData.chipWidth || result.totalHeight > data.designData.chipHeight) {
		Logger::instance().log("The blocks extend beyond the boundaries of the crystal");
		return false;
	}

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		Logger::instance().log(QString("Connot open %1 for writing").arg(filename));
		return false;
	}

	QTextStream out(&file);

	out << "\nPlacement:\n";
	for (const auto& b : result.blocks) {
		out << "   " << b.id << " at (" << b.x << ", " << b.y << ")"
			<< " size " << b.width << "x" << b.height << "\n";
	}

	out << "\nQuality metrics:\n";
	out << "   total area = " << result.totalWidth << "x" << result.totalHeight << "\n";
	out << "   total HPWL = " << result.hpwl << "\n";
	out << "   decoding time = " << elasped.count() << "s\n";

	Logger::instance().log("Placement:");
	for (const auto& b : result.blocks) {
		Logger::instance().log(QString("   %1 at (%2, %3), size: %4x%5, rotate: %6")
			.arg(b.id).arg(b.x).arg(b.y).arg(b.width).arg(b.height).arg(b.rotate ? "yes" : "no"));
	}

	Logger::instance().log("Quality metrics:");
	Logger::instance().log(QString("   total area = %1x%2").arg(result.totalWidth).arg(result.totalHeight));
	Logger::instance().log(QString("   total HPWL = %1").arg(result.hpwl));
	Logger::instance().log(QString("   decoding time = %1s").arg(elasped.count()));

	Logger::instance().log("The output file has been successfully created");
	return true;
}