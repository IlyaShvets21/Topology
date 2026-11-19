#include "Writer.hpp"
#include "Reader.hpp"
#include <iostream>
#include <QTextStream>
#include <QFile>

#include <windows.h>
#include <psapi.h>

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

	/*if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		out
	}*/

	QTextStream out(&file);

	out << "\nPlacement:\n";
	for (const auto& b : result.blocks) {
		out << "   " << b.id << " at (" << b.x << ", " << b.y << ")"
			<< " size " << b.width << "x" << b.height << " rotate " << (b.rotate ? "yes" : "no") << "\n";
	}

	PROCESS_MEMORY_COUNTERS pmc = {};

	out << "\nQuality metrics:\n";
	out << "   total area = " << result.totalWidth << "x" << result.totalHeight << "\n";
	out << "   total HPWL = " << result.hpwl << "\n";
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		out << "   consumed memory = " << pmc.WorkingSetSize / 1024 << " KiB\n";
	}
	else {
		out << "   consumed memory = unknown\n";
	}
	out << "   decoding time = " << elasped.count() << "s\n";

	Logger::instance().log("Placement:");
	for (const auto& b : result.blocks) {
		Logger::instance().log(QString("   %1 at (%2, %3), size: %4x%5, rotate: %6")
			.arg(b.id).arg(b.x).arg(b.y).arg(b.width).arg(b.height).arg(b.rotate ? "yes" : "no"));
	}

	Logger::instance().log("Quality metrics:");
	Logger::instance().log(QString("   total area = %1x%2").arg(result.totalWidth).arg(result.totalHeight));
	Logger::instance().log(QString("   total HPWL = %1").arg(result.hpwl));
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		Logger::instance().log(QString("   consumed memory = %1 KiB").arg(pmc.WorkingSetSize / 1024));
	}
	else {
		Logger::instance().log("   consumed memory = unknown");
	}
	Logger::instance().log(QString("   decoding time = %1s").arg(elasped.count()));

	Logger::instance().log("The output file has been successfully created");
	return true;
}