#include "Reader.hpp"
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

/************************************************************************************
 *  @brief  Функция чтения входного файла
 ***********************************************************************************/
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
                //Logger::instance().log("design:");
            }
            else if (t.contains("\"blocks\"")) {
                if (designCount == 0) {
                    Logger::instance().log("Error: No data on the chip is provided");
                    return false;
                }
                section = Section::Blocks;
                //Logger::instance().log("blocks:");
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
                //Logger::instance().log("sequence_pair:");
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
                //Logger::instance().log("nets:");
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
                //Logger::instance().log("decoder_params:");
            }
            continue;
        }

        //Считывание параметров группы design
        if (section == Section::Design) {
            if (t == "}," || t == "}") {
                if (data.designData.chipWidth == 0 || data.designData.chipHeight == 0 ||data.designData.gridSize == 0) {
                    Logger::instance().log("Error: One of the chip parameters is not specified. " "All data must be provided");
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
                //Logger::instance().log(QString("   chip_width = %1").arg(data.designData.chipWidth));
            }
            else if (key == "chip_height") {
                data.designData.chipHeight = value.toInt();
                //Logger::instance().log(QString("   chip_height = %1").arg(data.designData.chipHeight));
            }
            else if (key == "grid_size") {
                data.designData.gridSize = value.toInt();
                //Logger::instance().log(QString("   grid_size = %1").arg(data.designData.gridSize));
            }
            continue;
        }

        //Считывание параметров группы blocks
        if (section == Section::Blocks) {
            if (t == "]," || t == "]") {
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
                if (blockData.id.isEmpty() || blockData.width == 0 || blockData.height == 0)
                {
                    Logger::instance().log("Error: One of the blocks parameters is not specified. " "All data must be provided");
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
                //Logger::instance().log(QString("   id = %1").arg(blockData.id));
            }
            else if (key == "width") {
                blockData.width = value.toInt();
                //Logger::instance().log(QString("   width = %1").arg(blockData.width));
            }
            else if (key == "height") {
                blockData.height = value.toInt();
                //Logger::instance().log(QString("   height = %1").arg(blockData.height));
            }
            else if (key == "rotation_allowed") {
                blockData.rotationAllowed = (value == "true");
                //Logger::instance().log(QString("   rotation_allowed = %1").arg(blockData.rotationAllowed ? "true" : "false"));
            }
            continue;
        }

        //Считывание параметров группы sequence_pair
        if (section == Section::SequencePair) {
            if (t == "}" || t == "},") {
                for (const auto& b : data.blocks) {
                    if (!data.sequencePairData.A.contains(b.id)) {
                        Logger::instance().log(QString("Error: block \"%1\" is missing in A").arg(b.id));
                        return false;
                    }
                    if (!data.sequencePairData.B.contains(b.id)) {
                        Logger::instance().log(QString("Error: block \"%1\" is missing in B").arg(b.id));
                        return false;
                    }
                }
                if (data.sequencePairData.A.size() != data.blocks.size()) {
                    Logger::instance().log(QString("Error: A has %1 elements, but there are %2 blocks").arg(data.sequencePairData.A.size())
                        .arg(data.blocks.size()));
                    return false;
                }
                if (data.sequencePairData.B.size() != data.blocks.size()) {
                    Logger::instance().log(QString("Error: B has %1 elements, but there are %2 blocks").arg(data.sequencePairData.B.size())
                        .arg(data.blocks.size()));
                    return false;
                }

                section = Section::Undefined;
                continue;
            }

            if (collectingA) {
                bufferA += " " + t;
                if (t.contains(']')) {
                    collectingA = false;
                    if (!parseSequenceList(bufferA, "A", data)) return false;
                }
                continue;
            }

            if (collectingB) {
                bufferB += " " + t;
                if (t.contains(']')) {
                    collectingB = false;
                    if (!parseSequenceList(bufferB, "B", data)) return false;
                }
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
                if (netsData.net_id.isEmpty() || netsData.block_ids.isEmpty()) {
                    Logger::instance().log("Error: One of the nets parameters is not specified. " "All data must be provided");
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
                ///Logger::instance().log(QString("   net_id = %1").arg(netsData.net_id));
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
                const QString& ps = data.decoderParamsData.placementStrategy;
                if (ps != "bottom_left" && ps != "min_hpwl") {
                    Logger::instance().log("Error: One of the decoder parameters is not specified. " "All data must be provided");
                    return false;
                }
                section = Section::Undefined;
                continue;
            }

            QString key, value;
            if (!parseKeyValue(t, key, value)) continue;

            if (key == "use_b_tree") {
                data.decoderParamsData.useBTree = (value == "true");
                //Logger::instance().log(QString("   use_b_tree = %1").arg(data.decoderParamsData.useBTree ? "true" : "false"));
            }
            else if (key == "allow_rotation_during_decode") {
                data.decoderParamsData.allowRotationDuringDecode = (value == "true");
                /*Logger::instance().log(QString("   allow_rotation_during_decode = %1")
                    .arg(data.decoderParamsData.allowRotationDuringDecode ? "true" : "false"));*/
            }
            else if (key == "placement_strategy") {
                QString parsePlacementStrategy;
                if (value.size() > 2 && value.startsWith('"') && value.endsWith('"')) {
                    parsePlacementStrategy = value.mid(1, value.size() - 2);
                }
                data.decoderParamsData.placementStrategy = parsePlacementStrategy;
                //Logger::instance().log(QString("   placement_strategy = %1").arg(data.decoderParamsData.placementStrategy));
            }
            continue;
        }
    }

    return true;
}