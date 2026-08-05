// ItemManager implementation.  Mirrors the SkillManager pattern
// (modern/src/game/skill_manager.cpp) but over ItemInfo instead of
// SkillInfo.  Phase D6.x adds init_from_bin() via load_item_list.

#include "mxh/game/item_manager.hpp"
#include "mxh/game/item_list_parser.hpp"

#include <stdexcept>
#include <utility>

namespace mxh::game {

void ItemManager::init_from_bin(const std::string& path,
                                 std::uint32_t* out_errors) {
    auto result = load_item_list(path);
    if (!result.error_message.empty()
        && result.items.empty()) {
        // I/O or header failure: no rows loaded, propagate.
        throw std::runtime_error(
            "ItemManager::init_from_bin: " + result.error_message);
    }
    clear();
    for (auto& it : result.items) {
        // add() throws on duplicate item_idx; for the legacy bin
        // (which has unique item indices by construction) this
        // signals a malformed file, not normal flow.
        add(std::move(it));
    }
    if (out_errors) *out_errors = result.parse_errors;
}

void ItemManager::add(const ItemInfo& it) {
    if (m_idx.find(it.ItemIdx) != m_idx.end()) {
        throw std::invalid_argument(
            "ItemManager::add: duplicate item_idx " +
            std::to_string(it.ItemIdx));
    }
    m_idx.emplace(it.ItemIdx, m_items.size());
    m_items.push_back(it);
}

void ItemManager::add(ItemInfo&& it) {
    if (m_idx.find(it.ItemIdx) != m_idx.end()) {
        throw std::invalid_argument(
            "ItemManager::add: duplicate item_idx " +
            std::to_string(it.ItemIdx));
    }
    m_idx.emplace(it.ItemIdx, m_items.size());
    m_items.push_back(std::move(it));
}

const ItemInfo& ItemManager::get(std::uint32_t item_idx) const {
    auto it = m_idx.find(item_idx);
    if (it == m_idx.end()) {
        throw ItemNotFound(item_idx);
    }
    return m_items[it->second];
}

bool ItemManager::try_get(std::uint32_t item_idx,
                           ItemInfo& out) const noexcept {
    auto it = m_idx.find(item_idx);
    if (it == m_idx.end()) return false;
    out = m_items[it->second];
    return true;
}

bool ItemManager::exists(std::uint32_t item_idx) const noexcept {
    return m_idx.find(item_idx) != m_idx.end();
}

void ItemManager::clear() noexcept {
    m_items.clear();
    m_idx.clear();
}

}  // namespace mxh::game
