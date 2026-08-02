// shop_item_manager.cpp - Phase D4 ShopItemManager data-plane anchor.
//
// All data-plane logic lives in the inline header so it can be unit-tested
// without linking a compiled .cpp. This translation unit exists so the
// manager participates in mxh_server link units and CMake target_sources.

#include "mxh/server/shop_item_manager.hpp"

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

bool ShopItemManager::add_move_point(const MovePointEntry& entry) noexcept {
    if (entry.DBIdx == 0) return false;  // legacy refused DBIdx == 0
    if (m_movePoints.find(entry.DBIdx) != m_movePoints.end()) return false;
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

[[maybe_unused]] constexpr int shop_item_manager_translation_unit_anchor = 0;

} // namespace mxh::server
