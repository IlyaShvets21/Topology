#include "Validator.hpp"
#include <QSet>

bool Validator::parseBool(const QString& value, bool& out, QString& error, const QString& fieldName) {
    QString v = value.trimmed();
    if (v.size() >= 2 && v.startsWith('"') && v.endsWith('"')) {
        v = v.mid(1, v.size() - 2);
    }
    if (v == "true") { out = true;  return true; }
    if (v == "false") { out = false; return true; }

    error = QString("Field '%1' must be 'true' or 'false', got '%2'.")
        .arg(fieldName).arg(v);
    return false;
}

bool Validator::isComplete(const DesignData& d, QString& error) {
    if (d.chipWidth <= 0) {
        error = "Field 'chip_width' must be positive.";
        return false;
    }
    if (d.chipHeight <= 0) {
        error = "Field 'chip_height' must be positive.";
        return false;
    }
    if (d.gridSize <= 0) {
        error = "Field 'grid_size' must be positive.";
        return false;
    }
    return true;
}

bool Validator::isComplete(const BlockData& b, QString& error) {
    if (b.id.trimmed().isEmpty()) {
        error = "Block 'id' must not be empty.";
        return false;
    }
    if (b.width <= 0) {
        error = QString("Block '%1': 'width' must be positive.").arg(b.id);
        return false;
    }
    if (b.height <= 0) {
        error = QString("Block '%1': 'height' must be positive.").arg(b.id);
        return false;
    }
    return true;
}

bool Validator::isComplete(const NetsData& n, QString& error) {
    if (n.net_id.trimmed().isEmpty()) {
        error = "Net 'net_id' must not be empty.";
        return false;
    }
    if (n.block_ids.size() < 2) {
        error = QString("Net '%1': must contain at least 2 blocks.").arg(n.net_id);
        return false;
    }
    return true;
}

bool Validator::checkUniqueBlockIds(const QVector<BlockData>& blocks, QString& error) {
    QSet<QString> seen;
    for (const BlockData& b : blocks) {
        if (!b.rotationAllowedSet) {
            error = QString("Field 'rotation_allowed' is missing in '%1'.").arg(b.id);
            return false;
        }
        if (seen.contains(b.id)) {
            error = QString("Duplicate block id '%1'. "
                "Each block must have a unique id.").arg(b.id);
            return false;
        }
        seen.insert(b.id);
    }
    return true;
}

bool Validator::checkSequencePair(const FileData& data, QString& error) {
    const auto& A = data.sequencePairData.A;
    const auto& B = data.sequencePairData.B;
    const auto& blocks = data.blocks;

    if (A.size() != blocks.size()) {
        error = QString("Sequence pair A has %1 elements, but there are %2 blocks.")
            .arg(A.size()).arg(blocks.size());
        return false;
    }
    if (B.size() != blocks.size()) {
        error = QString("Sequence pair B has %1 elements, but there are %2 blocks.")
            .arg(B.size()).arg(blocks.size());
        return false;
    }

    QSet<QString> blockIds;
    for (const auto& b : blocks) blockIds.insert(b.id);

    QSet<QString> setA, setB;
    for (const QString& id : A) {
        if (!blockIds.contains(id)) {
            error = QString("Block '%1' in A does not match any block id.").arg(id);
            return false;
        }
        if (setA.contains(id)) {
            error = QString("Duplicate '%1' in A.").arg(id);
            return false;
        }
        setA.insert(id);
    }
    for (const QString& id : B) {
        if (!blockIds.contains(id)) {
            error = QString("Block '%1' in B does not match any block id.").arg(id);
            return false;
        }
        if (setB.contains(id)) {
            error = QString("Duplicate '%1' in B.").arg(id);
            return false;
        }
        setB.insert(id);
    }

    for (const auto& b : blocks) {
        if (!setA.contains(b.id)) {
            error = QString("Block '%1' is missing in A.").arg(b.id);
            return false;
        }
        if (!setB.contains(b.id)) {
            error = QString("Block '%1' is missing in B.").arg(b.id);
            return false;
        }
    }

    return true;
}

bool Validator::checkDecoderParams(const DecoderParams& p, QString& error) {
    if (!p.useBTreeSet) {
        error = "Field 'use_b_tree' is missing in 'decoder_params'.";
        return false;
    }

    if (!p.allowRotationSet) {
        error = "Field 'allow_rotation_during_decode' is missing in 'decoder_params'.";
        return false;
    }

    if (!p.placementStrategySet) {
        error = "Field 'placement_strategy' is missing in 'decoder_params'.";
        return false;
    }

    if (p.placementStrategy != "bottom_left" && p.placementStrategy != "min_hpwl") {
        error = QString("Invalid 'placement_strategy' '%1'. "
            "Allowed: 'bottom_left', 'min_hpwl'.").arg(p.placementStrategy);
        return false;
    }

    return true;
}

bool Validator::checkRowsMode(const DecoderParams& p, const DesignData& d, 
    const QVector<BlockData>& blocks, QString& error) {
    if (!p.useTracing) return true;

    if (d.lineSize <= 0) {
        error = "Field 'line_size' must be positive when 'use_rows' is true.";
        return false;
    }
    for (const BlockData& b : blocks) {
        if (b.height > d.lineSize) {
            error = QString("Block '%1' has height %2, which exceeds "
                "line_size %3. A block cannot be taller than a row.")
                .arg(b.id).arg(b.height).arg(d.lineSize);
            return false;
        }
    }

    return true;
}

bool Validator::validateAll(const FileData& data, QString& error) {
    if (!isComplete(data.designData, error)) return false;

    if (data.blocks.isEmpty()) {
        error = "No blocks provided.";
        return false;
    }
    for (const BlockData& b : data.blocks) {
        if (!isComplete(b, error)) return false;
    }
    if (!checkUniqueBlockIds(data.blocks, error)) return false;

    for (const NetsData& n : data.nets) {
        if (!isComplete(n, error)) return false;
    }

    if (!checkSequencePair(data, error)) return false;
    if (!checkDecoderParams(data.decoderParamsData, error)) return false;
    if (!checkRowsMode(data.decoderParamsData, data.designData, data.blocks, error)) return false;

    return true;
}