// shop_item_update_sql.hpp
//
// 1:1 port of legacy [Server]Map/MapDBMsgParser.cpp SQL builders for the
// shop-item update stored procedures. The legacy code uses sprintf(txt,
// "EXEC %s %d, %d, %d", STORED_SHOPITEM_*, ...) and then dispatches the
// string via g_DB.Query(eQueryType_FreeQuery, ...). The modern port splits
// the data plane (this file) from the dispatch (DbThread::execute_async)
// so the SQL format is testable in isolation and 1:1 with legacy.
//
// 1:1 invariants:
//   - Storage procedure names match the legacy MapDBMsgParser.h macros
//     verbatim: dbo.MP_SHOPITEM_Updatetime / dbo.MP_SHOPITEM_UpdateUseInfo
//     / dbo.MP_SHOPITEM_UpdateParam.
//   - Argument order matches the legacy sprintf call signature exactly
//     (CharacterIdx, dwItemIdx, RemainTime) for the updatetime builder,
//     (CharacterIdx, dwDBIdx, Param, RemainTime) for the use-info builder,
//     and (CharacterIdx, dwItemDBIdx, Param) for the param builder.
//   - The args are rendered as decimal digits (no hex prefix, no leading
//     zeros, no signed conversion). For all realistic DWORD values (< 2^31)
//     this is byte-equal to legacy sprintf %d output. For values >= 2^31
//     the legacy %d would render as negative; std::to_string renders them
//     positive. The legacy call sites never exercise the >= 2^31 branch
//     (item DB indices, character IDs, params, and remain-ms times are
//     all small positive integers), so the in-practice format is 1:1.
//
// Modern orchestrator: the data-plane string produced here is handed to
// mxh::db::DbThread::execute_async by the MapDBMsgParser dispatcher. The
// database layer is not coupled to this header.

#pragma once

#include <cstdint>
#include <string>

namespace mxh::server {

// 1:1 with legacy MapDBMsgParser.h STORED_SHOPITEM_* macros.
inline constexpr const char* STORED_SHOPITEM_UPDATETIME   = "dbo.MP_SHOPITEM_Updatetime";
inline constexpr const char* STORED_SHOPITEM_UPDATEUSEINFO = "dbo.MP_SHOPITEM_UpdateUseInfo";
inline constexpr const char* STORED_SHOPITEM_UPDATEPARAM  = "dbo.MP_SHOPITEM_UpdateParam";

// 1:1 with legacy MapDBMsgParser::ShopItemUpdatetimeToDB.
// Legacy: sprintf(txt, "EXEC %s %d, %d, %d", STORED_SHOPITEM_UPDATETIME,
//                       CharacterIdx, dwItemIdx, RemainTime);
inline std::string build_shop_item_update_time_sql(
    std::uint32_t character_idx,
    std::uint32_t item_idx,
    std::uint32_t remain_ms) noexcept {
    std::string out = "EXEC ";
    out += STORED_SHOPITEM_UPDATETIME;
    out += ' ';
    out += std::to_string(character_idx);
    out += ", ";
    out += std::to_string(item_idx);
    out += ", ";
    out += std::to_string(remain_ms);
    return out;
}

// 1:1 with legacy MapDBMsgParser::ShopItemUpdateUseInfoToDB.
// Legacy: sprintf(txt, "EXEC %s %d, %d, %d, %d", STORED_SHOPITEM_UPDATEUSEINFO,
//                       CharacterIdx, dwDBIdx, Param, RemainTime);
inline std::string build_shop_item_update_use_info_sql(
    std::uint32_t character_idx,
    std::uint32_t db_idx,
    std::uint32_t param,
    std::uint32_t remain_ms) noexcept {
    std::string out = "EXEC ";
    out += STORED_SHOPITEM_UPDATEUSEINFO;
    out += ' ';
    out += std::to_string(character_idx);
    out += ", ";
    out += std::to_string(db_idx);
    out += ", ";
    out += std::to_string(param);
    out += ", ";
    out += std::to_string(remain_ms);
    return out;
}

// 1:1 with legacy MapDBMsgParser::ShopItemParamUpdateToDB.
// Legacy: sprintf(txt, "EXEC %s %d, %d, %d", STORED_SHOPITEM_UPDATEPARAM,
//                       CharacterIdx, dwItemDBIdx, Param);
inline std::string build_shop_item_update_param_sql(
    std::uint32_t character_idx,
    std::uint32_t db_idx,
    std::uint32_t param) noexcept {
    std::string out = "EXEC ";
    out += STORED_SHOPITEM_UPDATEPARAM;
    out += ' ';
    out += std::to_string(character_idx);
    out += ", ";
    out += std::to_string(db_idx);
    out += ", ";
    out += std::to_string(param);
    return out;
}

}  // namespace mxh::server
