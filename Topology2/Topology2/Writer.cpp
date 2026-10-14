#include "Writer.hpp"
#include "Reader.hpp"
#include <iostream>
#include <QTextStream>
#include <QFile>

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
	out << "   total hpwl = " << result.hpwl << "\n";
	out << "   decoding time = " << elasped.count() << "s\n";

	Logger::instance().log("The output file has been successfully created");
	return true;
}