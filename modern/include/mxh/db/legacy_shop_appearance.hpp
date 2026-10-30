#pragma once
#include "mxh/db/db_adapter.hpp"
#include <array>
#include <limits>
#include <optional>

namespace mxh::db {

struct LegacyUsedShopItem {
    std::uint16_t item_id = 0, position = 0;
    std::uint32_t database_id = 0, parameter = 0, begin_time = 0, remaining_time = 0;
};

struct LegacyShopAppearanceRows {
    std::vector<LegacyUsedShopItem> used_items;
    std::array<std::uint16_t, 5> skin{}; // Hat, Mask, Dress, Shoulder, Shoes
};

enum class LegacyShopLoadStatus { Loaded, InvalidCharacter, QueryFailed, InvalidData };
struct LegacyShopLoadResult {
    LegacyShopLoadStatus status = LegacyShopLoadStatus::InvalidCharacter;
    DbError db_error = DbError::Ok;
    std::optional<LegacyShopAppearanceRows> rows;
};

// Reads the verified MP_SHOPITEM_UseInfo / MP_CHARACTER_SkinInfo source tables.
// Caller must establish the legacy character identity and own any transaction
// needed to combine this read with other admission data. No schema creation,
// modern-id translation, expiry calculation, or gameplay publication occurs here.
inline LegacyShopLoadResult load_legacy_shop_appearance(IDbAdapter& db, std::uint32_t legacy_character_id) {
    if (legacy_character_id == 0 || legacy_character_id > 0x7fffffffu) return {};
    ResultSet used, skins;
    const auto used_result = db.query(
        "SELECT A.ITEM_IDX,A.ITEM_DBIDX,A.ITEM_PARAM,A.BEGIN_TIME,A.REMAIN_TIME,"
        "COALESCE(B.item_position,0) FROM TB_SHOPITEMUSEINFO A "
        "LEFT JOIN TB_ITEM B ON A.ITEM_DBIDX=B.ITEM_DBIDX WHERE A.CHARACTER_IDX=?",
        {bind(static_cast<std::int64_t>(legacy_character_id))}, used);
    if (!used_result.ok()) return {LegacyShopLoadStatus::QueryFailed, used_result.error, std::nullopt};
    const auto skin_result = db.query(
        "SELECT Hat,Mask,Dress,Shoulder,Shoes FROM TB_SKININFO WHERE CharacterIdx=?",
        {bind(static_cast<std::int64_t>(legacy_character_id))}, skins);
    if (!skin_result.ok()) return {LegacyShopLoadStatus::QueryFailed, skin_result.error, std::nullopt};

    const auto invalid = [] { return LegacyShopLoadResult{LegacyShopLoadStatus::InvalidData, DbError::Ok, std::nullopt}; };
    // Preserve DWORD bit patterns stored in SQL Server's signed INT columns.
    const auto dword = [](const Value& value, std::uint32_t& out) {
        const auto* number = std::get_if<std::int64_t>(&value);
        if (!number || *number < std::numeric_limits<std::int32_t>::min() ||
            *number > std::numeric_limits<std::uint32_t>::max()) return false;
        out = static_cast<std::uint32_t>(*number); return true;
    };
    const auto word = [](const Value& value, std::uint16_t& out) {
        const auto* number = std::get_if<std::int64_t>(&value);
        if (!number || *number < 0 || *number > 65535) return false;
        out = static_cast<std::uint16_t>(*number); return true;
    };
    LegacyShopAppearanceRows rows;
    for (const auto& row : used.rows) {
        LegacyUsedShopItem item;
        if (row.size() != 6 || !word(row[0], item.item_id) || !dword(row[1], item.database_id) ||
            !dword(row[2], item.parameter) || !dword(row[3], item.begin_time) ||
            !dword(row[4], item.remaining_time) || !word(row[5], item.position)) return invalid();
        rows.used_items.push_back(item);
    }
    if (skins.rows.size() > 1) return invalid();
    if (!skins.rows.empty()) {
        if (skins.rows[0].size() != rows.skin.size()) return invalid();
        for (std::size_t i = 0; i < rows.skin.size(); ++i)
            if (!word(skins.rows[0][i], rows.skin[i])) return invalid();
    }
    // Verified stored procedure returns zero skin fields when no row exists.
    // Only a successful query can reach this empty-record default.
    return {LegacyShopLoadStatus::Loaded, DbError::Ok, std::move(rows)};
}
}
