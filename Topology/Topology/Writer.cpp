#include "Writer.hpp"
#include "Reader.hpp"
#include <iostream>
#include <fstream>

bool write(const PlacementResult& result, const FileData& data, std::chrono::duration<double> elasped) {
	if (result.totalWidth > data.designData.chipWidth || result.totalHeight > data.designData.chipHeight) {
		std::cout << "The blocks extend beyond the boundaries of the crystal\n";
		return false;
	}

	std::ofstream out("output.txt");
	if (!out.is_open()) {
		std::cout << "The file could not be created\n";
		return false;
	}
	out << "\nPlacement:\n";
	for (const auto& b : result.blocks) {
		out << "   " << b.id << " at (" << b.x << ", " << b.y << ")"
			<< " size " << b.width << "x" << b.height << "\n";
	}

	out << "\nQuality metrics:\n";
	out << "   total = " << result.totalWidth << "x" << result.totalHeight << "\n";
	out << "   decoding time = " << elasped.count() << "s\n";

	std::cout << "The output file has been successfully created\n";
	out.close();
	return true;
}