#include "Reader.hpp"
#include "Validator.hpp"
#include "Logger.hpp"

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QVector>
#include <algorithm>


/************************************************************************************
 *  @brief  Функция парсинга строчки для нахождения ключевых имен параметров
 ***********************************************************************************/
bool parseKeyValue(const QString& line, QString& key, QString& value) {
    const int colon = line.indexOf(':');
    if (colon == -1) return false;

    QString left = line.left(colon).trimmed();
    if (left.size() >= 2 && left.startsWith('"') && left.endsWith('"')) {
        key = left.mid(1, left.size() - 2);
    }
    else {
        key = left;
    }

    QString right = line.mid(colon + 1).trimmed();
    if (!right.isEmpty() && right.endsWith(',')) {
        right.chop(1);
    }
    value = right.trimmed();

    return !key.isEmpty() && !value.isEmpty();
}

/************************************************************************************
 *  @brief  Функция парсинга списков блоков, задающих парную последовательность 
 ***********************************************************************************/
bool parseSequenceList(const QString& line, const QString& name, FileData& data) {
    const int lb = line.indexOf('[');
    const int rb = line.lastIndexOf(']');
    if (lb == -1 || rb == -1 || rb <= lb) return false;

    const QString inside = line.mid(lb + 1, rb - lb - 1);

    QVector<QString>& target = (name == "A") ? data.sequencePairData.A : data.sequencePairData.B;

    const QStringList tokens = inside.split(',', Qt::SkipEmptyParts);

    for (const QString& rawToken : tokens) {
        const QString token = rawToken.trimmed();

        QString parseToken;
        if (token.size() > 2 && token.startsWith('"') && token.endsWith('"')) {
            parseToken = token.mid(1, token.size() - 2);
        }

        if (parseToken.isEmpty()) continue;

        const bool exists = std::any_of( data.blocks.begin(), data.blocks.end(),
            [&](const BlockData& b) { return b.id == parseToken; });

        if (!exists) {
            Logger::instance().log(QString("%1 in %2 does not match any block id").arg(parseToken).arg(name));
            return false;
        }

        target.push_back(parseToken);
    }

    QString out = QString("   %1 = [").arg(name);
    for (int i = 0; i < target.size(); ++i) {
        out += target[i];
        if (i + 1 < target.size()) out += ", ";
    }
    out += "]";
    Logger::instance().log(out);

    return true;
}

/************************************************************************************
 *  @brief  Функция парсинга списков блоков, задающих соединения
 ***********************************************************************************/
bool parseNetsList(const QString& line, const QString& name, NetsData& netsData, FileData& data) {
    const int lb = line.indexOf('[');
    const int rb = line.lastIndexOf(']');
    if (lb == -1 || rb == -1 || rb <= lb) return false;

    const QString inside = line.mid(lb + 1, rb - lb - 1);

    QVector<QString>& target = netsData.block_ids;

    const QStringList tokens = inside.split(',', Qt::SkipEmptyParts);

    for (const QString& rawToken : tokens) {
        const QString token = rawToken.trimmed();

        QString parseToken;
        if (token.size() > 2 && token.startsWith('"') && token.endsWith('"')) {
            parseToken = token.mid(1, token.size() - 2);
        }

        if (parseToken.isEmpty()) continue;

        const bool exists = std::any_of(data.blocks.begin(), data.blocks.end(),
            [&](const BlockData& b) { return b.id == parseToken; });

        if (!exists) {
            Logger::instance().log(QString("%1 in %2 does not match any block id").arg(parseToken).arg(name));
            return false;
        }

        target.push_back(parseToken);
    }

    QString out = QString("   %1 = [").arg(name);
    for (int i = 0; i < target.size(); ++i) {
        out += target[i];
        if (i + 1 < target.size()) out += ", ";
    }
    out += "]";
    Logger::instance().log(out);

    return true;
}

bool read(const QString& filename, FileData& data) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::instance().log(QString("The file: %1 could not be opened.").arg(filename));
        return false;
    }

    if (file.size() == 0) {
        Logger::instance().log("File is empty");
        return false;
    }

    QTextStream in(&file);

    Section section = Section::Undefined;

    bool fileStart = false;
    int  designCount = 0;
    int  blocksCount = 0;
    int  netsCount   = 0;

    bool collectingA = false;
    bool collectingB = false;
    bool collectingBlocksIds = false;
    QString bufferA, bufferB, bufferBlocksIds;

    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QString t = line.trimmed();
        if (t.isEmpty()) continue;

        if (!fileStart) {
            if (!line.contains('{')) {
                Logger::instance().log("Error: No initial data declaration was found");
                return false;
            }
            fileStart = true;
            continue;
        }

        //Определение ключевого слова, определяющего группу параметров
        if (section == Section::Undefined) {
            if (t.contains("\"design\"")) {
                if (designCount == 1) {
                    Logger::instance().log("Error: Only a single crystal data entry is possible");
                    return false;
                }
                section = Section::Design;
            }
            else if (t.contains("\"blocks\"")) {
                if (designCount == 0) {
                    Logger::instance().log("Error: No data on the chip is provided");
                    return false;
                }
                section = Section::Blocks;
            }
            else if (t.contains("\"sequence_pair\"")) {
                if (designCount == 0) {
                    Logger::instance().log("Error: No data on the chip is provided");
                    return false;
                }
                if (blocksCount == 0) {
                    Logger::instance().log("Error: No data on the blocks is provided");
                    return false;
                }
                section = Section::SequencePair;
            }
            else if (t.contains("\"nets\"")) {
                if (designCount == 0) {
                    Logger::instance().log("Error: No data on the chip is provided");
                    return false;
                }
                if (blocksCount == 0) {
                    Logger::instance().log("Error: No data on the blocks is provided");
                    return false;
                }
                section = Section::Nets;
            }
            else if (t.contains("\"decoder_params\"")) {
                if (designCount == 0) {
                    Logger::instance().log("Error: No data on the chip is provided");
                    return false;
                }
                if (blocksCount == 0) {
                    Logger::instance().log("Error: No data on the blocks is provided");
                    return false;
                }
                section = Section::DecoderParams;
            }
            continue;
        }

        //Считывание параметров группы design
        if (section == Section::Design) {
            if (t == "}," || t == "}") {
                QString error;
                if (!Validator::isComplete(data.designData, error)) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                ++designCount;
                section = Section::Undefined;
                continue;
            }

            QString key, value;
            if (!parseKeyValue(t, key, value)) continue;

            if (key == "chip_width") {
                data.designData.chipWidth = value.toInt();
            }
            else if (key == "chip_height") {
                data.designData.chipHeight = value.toInt();
            }
            else if (key == "grid_size") {
                data.designData.gridSize = value.toInt();
            }
            else if (key == "line_size") {
                data.designData.lineSize = value.toInt();
            }
            else if (key == "channel_size") {
                data.designData.channelSize = value.toInt();
            }
            continue;
        }

        //Считывание параметров группы blocks
        if (section == Section::Blocks) {
            if (t == "]," || t == "]") {
                QString error;
                if (!Validator::checkUniqueBlockIds(data.blocks, error)) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                section = Section::Undefined;
                continue;
            }

            if (t == "{") {
                data.blocks.append(BlockData{});
                continue;
            }

            if (data.blocks.isEmpty()) {
                continue;
            }

            BlockData& blockData = data.blocks.last();

            if (t == "}," || t == "}") {
                QString error;
                if (!Validator::isComplete(blockData, error)) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                ++blocksCount;
                continue;
            }

            QString key, value;
            if (!parseKeyValue(t, key, value)) continue;

            if (key == "id") {
                if (value.size() < 2 || !value.startsWith('"') || !value.endsWith('"')) return false;

                const QString inner = value.mid(1, value.size() - 2);

                if (inner.trimmed().isEmpty()) {
                    Logger::instance().log("Error: The block name must not be empty.");
                    return false;
                }

                blockData.id = inner;
            }
            else if (key == "width") {
                blockData.width = value.toInt();
            }
            else if (key == "height") {
                blockData.height = value.toInt();
            }
            else if (key == "rotation_allowed") {
                QString error;
                bool parsed;
                if (!Validator::parseBool(value, parsed, error, "allow_rotation_during_decode")) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                blockData.rotationAllowed = parsed;
                blockData.rotationAllowedSet = true;
            }
            continue;
        }

        //Считывание параметров группы sequence_pair
        if (section == Section::SequencePair) {
            if (t == "}" || t == "},") {
                QString error;
                if (!Validator::checkSequencePair(data, error)) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                section = Section::Undefined;
                continue;
            }

            auto handleCollecting = [&](bool& collecting, QString& buffer, const QString& name) -> bool {
                if (!collecting) return true; 
                buffer += " " + t;
                if (t.contains(']')) {
                    collecting = false;
                    if (!parseSequenceList(buffer, name, data)) return false;
                }
                return true;  
                };

            if (collectingA) {
                if (!handleCollecting(collectingA, bufferA, "A")) return false;
                continue;
            }
            if (collectingB) {
                if (!handleCollecting(collectingB, bufferB, "B")) return false;
                continue;
            }

            QString key, value;
            if (!parseKeyValue(t, key, value)) continue;

            if (key == "A" || key == "B") {
                if (t.contains(']')) {
                    if (!parseSequenceList(t, key, data)) return false;
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

        //Считывание параметров группы nets
        if (section == Section::Nets) {
            if (t == "]," || t == "]") {
                section = Section::Undefined;
                continue;
            }

            if (t == "{") {
                data.nets.append(NetsData{});
                collectingBlocksIds = false;
                bufferBlocksIds.clear();
                continue;
            }

            if (data.nets.isEmpty()) {
                continue;
            }

            NetsData& netsData = data.nets.last();

            if (t == "}," || t == "}") {
                QString error;
                if (!Validator::isComplete(netsData, error)) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                ++netsCount;
                continue;
            }

            if (collectingBlocksIds) {
                bufferBlocksIds += " " + t;
                if (t.contains(']')) {
                    collectingBlocksIds = false;
                    if (!parseNetsList(bufferBlocksIds, netsData.net_id, netsData, data)) return false;
                }
                continue;
            }

            QString key, value;
            if (!parseKeyValue(t, key, value)) continue;

            if (key == "net_id") {
                if (value.size() < 2 || !value.startsWith('"') || !value.endsWith('"')) return false;

                const QString inner = value.mid(1, value.size() - 2);

                if (inner.trimmed().isEmpty()) {
                    Logger::instance().log("Error: The net name must not be empty.");
                    return false;
                }

                netsData.net_id = inner;
            }

            else if (key == "block_ids") {
                if (t.contains(']')) {
                    if (!parseNetsList(t, key,netsData, data)) return false;
                }
                else {
                    collectingBlocksIds = true;
                    bufferBlocksIds = t;
                }
            }
            continue;
        }

        //Считывание параметров группы decoder_params
        if (section == Section::DecoderParams) {
            if (t == "}" || t == "},") {
                QString error;
                if (!Validator::checkDecoderParams(data.decoderParamsData, error)) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                section = Section::Undefined;
                continue;
            }

            QString key, value;
            if (!parseKeyValue(t, key, value)) continue;

            if (key == "use_b_tree") {
                QString error;
                bool parsed;
                if (!Validator::parseBool(value, parsed, error, "use_b_tree")) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                data.decoderParamsData.useBTree = parsed;
                data.decoderParamsData.useBTreeSet = true;   
            }
            else if (key == "allow_rotation_during_decode") {
                QString error;
                bool parsed;
                if (!Validator::parseBool(value, parsed, error, "allow_rotation_during_decode")) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                data.decoderParamsData.allowRotationDuringDecode = parsed;
                data.decoderParamsData.allowRotationSet = true;  
            }
            else if (key == "placement_strategy") {
                QString parsePlacementStrategy;
                if (value.size() > 2 && value.startsWith('"') && value.endsWith('"')) {
                    parsePlacementStrategy = value.mid(1, value.size() - 2);
                }
                data.decoderParamsData.placementStrategy = parsePlacementStrategy;
                data.decoderParamsData.placementStrategySet = true;  
            }
            else if (key == "use_tracing") {
                QString error;
                bool parsed;
                if (!Validator::parseBool(value, parsed, error, "use_tracing")) {
                    Logger::instance().log("Error: " + error);
                    return false;
                }
                else if (Validator::parseBool(value, parsed, error, "use_tracing") 
                    && (data.designData.lineSize <= 0 || data.designData.channelSize <= 0)) {
                    Logger::instance().log("Error: line_size fnd channel_size must be positive when use_tracing: true");
                    return false;
                }
                data.decoderParamsData.useTracing = parsed;
                data.decoderParamsData.useTracingSet = true;
            }
            continue;
        }
    }

    QString error;
    if (!Validator::validateAll(data, error)) {
        Logger::instance().log("Error: " + error);
        return false;
    }
    return true; 
}