// shop_item_manager.cpp - Phase D4 ShopItemManager data-plane anchor.
//
// All data-plane logic lives in the inline header so it can be unit-tested
// without linking a compiled .cpp. This translation unit exists so the
// manager participates in mxh_server link units and CMake target_sources.

#include "mxh/server/shop_item_manager.hpp"
#include <cstring>

namespace mxh::server {

void ShopItemManager::init(void* p_player) noexcept {
    m_pPlayer       = p_player;
    m_Checktime     = 0;
    m_Updatetime    = 0;
    m_DupIncantation = 0;
    m_DupCharm      = 0;
    m_DupHerb       = 0;
    m_DupSundries   = 0;
    m_DupPetEquip   = 0;
    m_ProtectItemIdx = 0;
    m_usingItems.clear();
    m_movePoints.clear();
}

void ShopItemManager::release() noexcept {
    m_pPlayer        = nullptr;
    m_DupIncantation = 0;
    m_DupCharm       = 0;
    m_DupHerb        = 0;
    m_DupSundries    = 0;
    m_DupPetEquip    = 0;
    m_ProtectItemIdx = 0;
    m_Checktime      = 0;
    m_Updatetime     = 0;
    m_usingItems.clear();
    m_movePoints.clear();
}

std::uint32_t ShopItemManager::bump_dup_incantation() noexcept { return ++m_DupIncantation; }
std::uint32_t ShopItemManager::bump_dup_charm()       noexcept { return ++m_DupCharm; }
std::uint32_t ShopItemManager::bump_dup_herb()        noexcept { return ++m_DupHerb; }
std::uint32_t ShopItemManager::bump_dup_sundries()    noexcept { return ++m_DupSundries; }
std::uint32_t ShopItemManager::bump_dup_pet_equip()   noexcept { return ++m_DupPetEquip; }

void ShopItemManager::clear_dup_counters() noexcept {
    m_DupIncantation = 0;
    m_DupCharm       = 0;
    m_DupHerb        = 0;
    m_DupSundries    = 0;
    m_DupPetEquip    = 0;
}

bool ShopItemManager::add_using_item(const UsingShopItemEntry& entry) noexcept {
    if (entry.ItemIdx == 0) return false;  // legacy refused dwItemIdx == 0
    if (m_usingItems.find(entry.ItemIdx) != m_usingItems.end()) return false;
    m_usingItems.emplace(entry.ItemIdx, entry);
    return true;
}

bool ShopItemManager::has_using_item(std::uint64_t item_idx) const noexcept {
    return m_usingItems.find(item_idx) != m_usingItems.end();
}

const UsingShopItemEntry* ShopItemManager::find_using_item(std::uint64_t item_idx) const noexcept {
    auto it = m_usingItems.find(item_idx);
    if (it == m_usingItems.end()) return nullptr;
    return &it->second;
}

bool ShopItemManager::delete_using_item(std::uint64_t item_idx) noexcept {
    return m_usingItems.erase(item_idx) > 0;
}

bool ShopItemManager::used_shop_item(const game::ItemBase& item_base,
                                     std::uint32_t param,
                                     game::PackedTime begin_time,
                                     std::uint32_t remain_ms,
                                     std::uint32_t now_ms) noexcept {
    if (item_base.wIconIdx == 0) return false;  // invalid shop-item slot
    const std::uint64_t key = item_base.wIconIdx;  // legacy key = wIconIdx
    if (m_usingItems.find(key) != m_usingItems.end()) return false;  // already in use
    UsingShopItemEntry e{};
    e.ItemIdx = key;
    e.Data.ShopItem.ItemBase = item_base;
    e.Data.ShopItem.Param = param;
    e.Data.ShopItem.BeginTime = begin_time;
    e.Data.ShopItem.Remaintime = remain_ms;
    e.Data.LastCheckTime = now_ms;  // legacy uses gCurTime
    m_usingItems.emplace(key, e);
    return true;
}

bool ShopItemManager::has_using_item_by_icon_idx(std::uint16_t icon_idx) const noexcept {
    return m_usingItems.find(static_cast<std::uint64_t>(icon_idx)) != m_usingItems.end();
    (void)0;  // suppress unused warning on no-op return
}

const UsingShopItemEntry* ShopItemManager::find_using_item_by_icon_idx(
    std::uint16_t icon_idx) const noexcept {
    auto it = m_usingItems.find(static_cast<std::uint64_t>(icon_idx));
    if (it == m_usingItems.end()) return nullptr;
    return &it->second;
}

bool ShopItemManager::is_using_item_active(std::uint16_t icon_idx,
                                           std::uint32_t now_ms) const noexcept {
    const auto* e = find_using_item_by_icon_idx(icon_idx);
    if (e == nullptr) return false;
    const std::uint64_t deadline =
        static_cast<std::uint64_t>(e->Data.LastCheckTime)
        + static_cast<std::uint64_t>(e->Data.ShopItem.Remaintime);
    return deadline > now_ms;  // strict > matches legacy 'still active'
}

bool ShopItemManager::is_memory_move_extend_icon(std::uint16_t icon_idx) noexcept {
    return icon_idx == INCANTATION_MEMORY_MOVE_EXTEND
        || icon_idx == INCANTATION_MEMORY_MOVE_EXTEND7
        || icon_idx == INCANTATION_MEMORY_MOVE2
        || icon_idx == INCANTATION_MEMORY_MOVE_EXTEND30;
}

std::size_t ShopItemManager::move_point_capacity() const noexcept {
    // Legacy: capacity is MAX_MOVEDATA_PER_PAGE (10) unless any of the 4
    // MemoryMove incantation items is in the using-item table; then it
    // doubles to MAX_MOVEDATA_PER_PAGE*MAX_MOVEPOINT_PAGE (20).
    for (const auto& kv : m_usingItems) {
        const auto icon = static_cast<std::uint16_t>(kv.first & 0xFFFFu);
        if (is_memory_move_extend_icon(icon)) {
            return MAX_MOVEDATA_PER_PAGE_MODERN * game::MAX_MOVEPOINT_PAGE_MODERN;
        }
    }
    return MAX_MOVEDATA_PER_PAGE_MODERN;
}

bool ShopItemManager::add_move_point(const MovePointEntry& entry) noexcept {
    if (entry.DBIdx == 0) return false;  // legacy refused DBIdx == 0
    if (m_movePoints.find(entry.DBIdx) != m_movePoints.end()) return false;
    // Legacy capacity gate: rejecting on `>=` so a full table refuses the
    // next insert without overshooting.
    if (m_movePoints.size() >= move_point_capacity()) return false;
    m_movePoints.emplace(entry.DBIdx, entry);
    return true;
}

const MovePointEntry* ShopItemManager::find_move_point(std::uint32_t db_idx) const noexcept {
    auto it = m_movePoints.find(db_idx);
    if (it == m_movePoints.end()) return nullptr;
    return &it->second;
}

bool ShopItemManager::delete_move_point(std::uint32_t db_idx) noexcept {
    return m_movePoints.erase(db_idx) > 0;
}

bool ShopItemManager::tick(std::uint32_t delta_ms) noexcept {
    if (delta_ms == 0) return false;
    // Use DWORD arithmetic to match legacy (no overflow checks). The legacy
    // code uses gTickTime which is bounded (~10s max), so practical overflow
    // never happens, but the data plane preserves the legacy shape.
    m_Updatetime += delta_ms;
    if (m_Updatetime > SHOP_ITEM_UPDATE_INTERVAL_MS) {
        m_Updatetime = 0;
        return true;  // rollover - legacy flushes ShopItemAllUseInfo to DB
    }
    m_Checktime += delta_ms;
    return false;
}

void ShopItemManager::collect_expired(std::uint32_t now_ms,
                                     std::vector<std::uint64_t>& out) const {
    out.clear();
    for (const auto& kv : m_usingItems) {
        const auto& entry = kv.second;
        const std::uint64_t deadline =
            static_cast<std::uint64_t>(entry.Data.LastCheckTime)
            + static_cast<std::uint64_t>(entry.Data.ShopItem.Remaintime);
        if (deadline <= now_ms) out.push_back(kv.first);
    }
}

std::size_t ShopItemManager::tick_and_collect_expired(std::uint32_t delta_ms,
                                                     std::uint32_t now_ms,
                                                     std::vector<std::uint64_t>& out) {
    const bool rollover = tick(delta_ms);
    (void)rollover;
    if (!check_due()) {
        out.clear();
        return 0;
    }
    const std::size_t before = out.size();
    collect_expired(now_ms, out);
    return out.size() - before;
}

std::vector<std::uint8_t> ShopItemManager::serialize_using_items(
    std::uint8_t category, std::uint8_t protocol) const {
    if (m_usingItems.size() > 100u) return {};  // legacy max
    const std::uint16_t count = static_cast<std::uint16_t>(m_usingItems.size());
    const std::size_t total =
        sizeof(mxh::net::MsgHeader) + sizeof(std::uint16_t) +
        static_cast<std::size_t>(count) * sizeof(game::ShopItemBase);
    std::vector<std::uint8_t> out(total, 0);
    if (total < 8u) return out;
    out[2] = category;   // MSGBASE.Category offset = 2 (checksum, code, category)
    out[3] = protocol;   // MSGBASE.Protocol offset = 3
    std::memcpy(out.data() + 8, &count, sizeof(std::uint16_t));
    std::size_t offset = 8 + sizeof(std::uint16_t);
    for (const auto& kv : m_usingItems) {
        std::memcpy(out.data() + offset, &kv.second.Data.ShopItem,
                    sizeof(game::ShopItemBase));
        offset += sizeof(game::ShopItemBase);
    }
    return out;
}

std::vector<std::uint8_t> ShopItemManager::serialize_using_items_headerless() const {
    return serialize_using_items(0, 0);
}

std::vector<std::uint8_t> ShopItemManager::serialize_move_points(
    std::uint8_t category, std::uint8_t protocol, bool b_inited) const {
    if (m_movePoints.size() > game::MOVEPOINT_TOTAL_MODERN) return {};
    const std::uint16_t count = static_cast<std::uint16_t>(m_movePoints.size());
    const std::size_t total = sizeof(mxh::net::MsgHeader) + sizeof(std::uint8_t)
                              + sizeof(std::uint16_t)
                              + static_cast<std::size_t>(count) * sizeof(game::MoveData);
    std::vector<std::uint8_t> out(total, 0);
    out[2] = category;   // MSGBASE.Category
    out[3] = protocol;   // MSGBASE.Protocol
    out[sizeof(mxh::net::MsgHeader)] = b_inited ? 1 : 0;  // bInited
    std::memcpy(out.data() + sizeof(mxh::net::MsgHeader) + sizeof(std::uint8_t),
                &count, sizeof(std::uint16_t));
    std::size_t offset = sizeof(mxh::net::MsgHeader) + sizeof(std::uint8_t)
                          + sizeof(std::uint16_t);
    for (const auto& kv : m_movePoints) {
        std::memcpy(out.data() + offset, &kv.second.Data,
                    sizeof(game::MoveData));
        offset += sizeof(game::MoveData);
    }
    return out;
}

std::vector<std::uint8_t> ShopItemManager::serialize_move_points_headerless(
    bool b_inited) const {
    return serialize_move_points(0, 0, b_inited);
}

[[maybe_unused]] constexpr int shop_item_manager_translation_unit_anchor = 0;

} // namespace mxh::server
