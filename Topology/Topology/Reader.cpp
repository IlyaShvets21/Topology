#include "Reader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

std::string trim(const std::string& s) {
	const auto begin = s.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos) return "";
	const auto end = s.find_last_not_of(" \t\r\n");
	return s.substr(begin, end - begin + 1);
}

bool parseKeyValue(const std::string& line, std::string& key, std::string& value) {
	const auto colon = line.find(':');
	if (colon == std::string::npos) return false;

	std::string left = trim(line.substr(0, colon));
	if (left.size() >= 2 && left.front() == '"' && left.back() == '"') {
		key = left.substr(1, left.size() - 2);
	}
	else {
		key = left;
	}

	std::string right = trim(line.substr(colon + 1));
	if (!right.empty() && right.back() == ',') {
		right.pop_back();
	}
	value = trim(right);

	return !key.empty() && !value.empty();
}

bool parceSequenceList(const std::string& line, const std::string& name, FileData& data) {
	const auto lb = line.find('[');
	const auto rb = line.rfind(']');
	if (lb == std::string::npos || rb == std::string::npos || rb <= lb) return false;

	const std::string inside = line.substr(lb + 1, rb - lb - 1);

	std::vector<std::string>& target = (name == "A") ? data.sequencePairData.A : data.sequencePairData.B;

	std::size_t start = 0;
	while (start < inside.size()) {
		const auto comma = inside.find(',', start);
		const std::string token = trim(inside.substr(start, comma == std::string::npos ? std::string::npos : comma - start));

		std::string parseToken;
		if (token.size() > 2) {
			parseToken = token.substr(1, token.size() - 2);
		}

		if (!parseToken.empty()) {
			const bool exists = std::any_of(
				data.blocks.begin(), data.blocks.end(),
				[&](const BlockData& b) { return b.id == parseToken; });

			if (!exists) {
				std::cout << parseToken << " in " << name << " does not match any block id\n";
				return false;
			}

			target.push_back(parseToken);
		}

		if (comma == std::string::npos) break;
		start = comma + 1;
	}
	std::cout << "   " << name << " = [";
	for (std::size_t i = 0; i < target.size(); i++) {
		std::cout << target[i];
		if (i + 1 < target.size()) std::cout << ", ";
	}
	std::cout << "]\n";
	return true;
}

bool read(const std::string& filename, FileData& data) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "The file: " << filename << "could not be opened.\n";
		return false;
	}

	file.clear();
	file.seekg(0, std::ios::end);
	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	if (size == std::streampos(-1)) {
		std::cout << "Size determination error\n";
		return false;
	}

	if (size == 0) {
		std::cout << "File is empty\n";
		return false;
	}

	Section section = Section::Undefined;

	std::string line;
	bool fileStart = false;
	int designCount = 0;
	int blocksCount = 0;

	bool collectingA = false;
	bool collectingB = false;
	std::string bufferA, bufferB;

	while (std::getline(file, line)) {
		const std::string t = trim(line);
		if (t.empty()) continue;

		if (!fileStart) {
			if (line.find('{') == std::string::npos) {
				std::cout << "No initial data declaration was found\n";
				return false;
			}
			fileStart = true;
			continue;
		}

		if (section == Section::Undefined) {
			if (t.find("\"design\"") != std::string::npos) {
				if (designCount == 1) {
					std::cout << "Only a single crystal data entry is possible\n";
					return false;
				}
				section = Section::Design;
				std::cout << "design:\n";
			}
			else if (t.find("\"blocks\"") != std::string::npos) {
				if (designCount == 0) {
					std::cout << "No data on the chip is provided\n";
					return false;
				}

				section = Section::Blocks;
				std::cout << "blocks:\n";
			}
			else if (t.find("\"sequence_pair\"") != std::string::npos) {
				if (designCount == 0) {
					std::cout << "No data on the chip is provided\n";
					return false;
				}
				if (blocksCount == 0) {
					std::cout << "No data on the blocks is provided\n";
					return false;
				}

				section = Section::SequencePair;
				std::cout << "sequence_pair:\n";
			}
			else if (t.find("\"decoder_params\"") != std::string::npos) {
				if (designCount == 0) {
					std::cout << "No data on the chip is provided\n";
					return false;
				}
				if (blocksCount == 0) {
					std::cout << "No data on the blocks is provided\n";
					return false;
				}

				section = Section::DecoderParams;
				std::cout << "decoder_params:\n";
			}
			continue;
		}

		if (section == Section::Design) {
			if (t == "},") { 
				if (data.designData.chipWidth == 0 || data.designData.chipHeight == 0 || data.designData.gridSize == 0 || 
					(data.designData.rotationAllowed != false && data.designData.rotationAllowed != true)) {
					std::cout << "One of the chip parameters is not specified. All data must be provided\n";
					return false;
				}
				designCount++;
				section = Section::Undefined;  
				continue;
			}
		
			std::string key, value;
			if (!parseKeyValue(t, key, value)) continue;

			if (key == "chip_width") {
				data.designData.chipWidth = std::stoi(value);
			    std::cout << "   chip_width = " << data.designData.chipWidth << "\n";
			}
			else if (key == "chip_height") {
				data.designData.chipHeight = std::stoi(value);
				std::cout << "   chip_height = " << data.designData.chipHeight << "\n";
			}
			else if (key == "grid_size") {
				data.designData.gridSize = std::stoi(value);
				std::cout << "   grid_size = " << data.designData.gridSize << "\n";
			}
			else if (key == "rotation_allowed") {
				data.designData.rotationAllowed = (value == "true");
				std::cout << "   rotation_allowed = " << (data.designData.rotationAllowed ? "true" : "false") << "\n";
			}

			continue;
		}

		if (section == Section::Blocks) {
			if (t == "]," || t == "]") {
				section = Section::Undefined;
				continue;
			}

			if (t == "{") {
				data.blocks.emplace_back();
				continue;
			}

			BlockData& blockData = data.blocks.back();

			if (t == "},") {
				if (blockData.id == "" || blockData.width == 0 ||
					blockData.height == 0 || (blockData.rotationAllowed != false && blockData.rotationAllowed != true)) {
					std::cout << "One of the blocks parameters is not specified. All data must be provided\n";
					return false;
				}
				blocksCount++;
				continue;
			}

			std::string key, value;
			if (!parseKeyValue(t, key, value)) continue;

			if (key == "id") {
				std::string parseValue;
				if (value.size() > 2) {
					parseValue = value.substr(1, value.size() - 2);
				}
				blockData.id = parseValue;
				std::cout << "   id = " << blockData.id << "\n";
			}
			else if (key == "width") {
				blockData.width = std::stoi(value);
				std::cout << "   width = " << blockData.width << "\n";

			}
			else if (key == "height") {
				blockData.height = std::stoi(value);
				std::cout << "   height = " << blockData.height << "\n";

			}
			else if (key == "rotation_allowed") {
				blockData.rotationAllowed = (value == "true");
				std::cout << "   rotation_allowed = " << (blockData.rotationAllowed ? "true" : "false") << "\n";
			}

			continue;
		}

		if (section == Section::SequencePair) {
			if (t == "}" || t == "},") {
				for (const auto& b : data.blocks) {
					const bool inA = std::find(data.sequencePairData.A.begin(),
						data.sequencePairData.A.end(), b.id)
						!= data.sequencePairData.A.end();
					if (!inA) {
						std::cerr << "Error: block \"" << b.id
							<< "\" is missing in A\n";
						return false;
					}

					const bool inB = std::find(data.sequencePairData.B.begin(),
						data.sequencePairData.B.end(), b.id)
						!= data.sequencePairData.B.end();
					if (!inB) {
						std::cerr << "Error: block \"" << b.id
							<< "\" is missing in B\n";
						return false;
					}
				}

				if (data.sequencePairData.A.size() != data.blocks.size()) {
					std::cerr << "Error: A has " << data.sequencePairData.A.size()
						<< " elements, but there are "
						<< data.blocks.size() << " blocks\n";
					return false;
				}
				if (data.sequencePairData.B.size() != data.blocks.size()) {
					std::cerr << "Error: B has " << data.sequencePairData.B.size()
						<< " elements, but there are "
						<< data.blocks.size() << " blocks\n";
					return false;
				}

				section = Section::Undefined;
				continue;
			}

			if (collectingA) {
				bufferA += " " + t;
				if (t.find(']') != std::string::npos) {
					collectingA = false;
					if (!parceSequenceList(bufferA, "A", data)) return false;
				}
				continue;
			}


			if (collectingB) {
				bufferB += " " + t;
				if (t.find(']') != std::string::npos) {
					collectingB = false;
					if (!parceSequenceList(bufferB, "B", data)) return false;
				}
				continue;
			}

			std::string key, value;
			if (!parseKeyValue(t, key, value)) continue;

			if (key == "A" || key == "B"){
				if (t.find(']') != std::string::npos) {
					if(!parceSequenceList(t, key, data)) return false;
				}
				else {
					if (key == "A") {
						collectingA = true;
						bufferA = t;
					}
					else {
						collectingB = true;
						bufferB = t;
					}
				}
			}

			continue;
		}

		if (section == Section::DecoderParams) {
			if (t == "}") {
				if ((data.decoderParamsData.useBTree != false && data.decoderParamsData.useBTree != true) || 
					(data.decoderParamsData.allowRotationDuringDecode != false && data.decoderParamsData.allowRotationDuringDecode != true) ||
					(data.decoderParamsData.placementStrategy != "bottom_left" && data.decoderParamsData.placementStrategy != "min_hpwl")) {
					std::cout << "One of the decoder parameters is not specified. All data must be provided\n";
					return false;
				}
				section = Section::Undefined;
				continue;
			}

			std::string key, value;
			if (!parseKeyValue(t, key, value)) continue;

			if (key == "use_b_tree") {
				data.decoderParamsData.useBTree = (value == "true");
				std::cout << "   use_b_tree = " << (data.decoderParamsData.useBTree ? "true" : "false") << "\n";
			}
			else if (key == "allow_rotation_during_decode") {
				data.decoderParamsData.allowRotationDuringDecode = (value == "true");
				std::cout << "   allow_rotation_during_decode = " << (data.decoderParamsData.allowRotationDuringDecode ? "true" : "false") << "\n";
			}
			else if (key == "placement_strategy") {
				std::string parsePlacementStrtegy;
				if (value.size() > 2) {
					parsePlacementStrtegy = value.substr(1, value.size() - 2);
				}
				data.decoderParamsData.placementStrategy = parsePlacementStrtegy;
				std::cout << "   placement_strategy = " << data.decoderParamsData.placementStrategy << "\n";
			}

			continue;
		}
	}

	return true;
}

