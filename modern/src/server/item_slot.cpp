#include "mxh/server/item_slot.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace mxh::server {
namespace {

bool has_flag(std::uint16_t value, std::uint16_t flag) noexcept {
    return (value & flag) != 0;
}

void clear_item_base(mxh::game::ItemBase& item) noexcept {
    item.dwDBIdx = 0;
    item.wIconIdx = 0;
    item.Position = 0;
    item.QuickPosition = 0;
    item.Durability = 0;
    item.RareIdx = 0;
}

void clear_slot_info(SlotInfo& info) noexcept {
    info.bLock = 0;
    info.wPassword = 0;
    info.wState = 0;
}

}  // namespace

bool ItemSlot::init(std::uint16_t start_abs_position,
                    std::uint16_t slot_count,
                    std::span<mxh::game::ItemBase> items,
                    std::span<SlotInfo> slot_info,
                    DuplicateItemPredicate is_duplicate_item) noexcept {
    const auto end = static_cast<std::size_t>(start_abs_position) + slot_count;
    if (end > items.size() || end > slot_info.size()) return false;

    m_start_abs_position = start_abs_position;
    m_slot_count = slot_count;
    m_items = items;
    m_slot_info = slot_info;
    m_is_duplicate_item = std::move(is_duplicate_item);
    return true;
}

const mxh::game::ItemBase* ItemSlot::get_item_info_abs(
    std::uint16_t abs_position) const noexcept {
    if (!is_position_inside(abs_position)) return nullptr;
    return &m_items[abs_position];
}

bool ItemSlot::get_item_info_all(
    std::span<mxh::game::ItemBase> output) const noexcept {
    if (output.size() < m_slot_count) return false;
    std::copy_n(m_items.begin() + m_start_abs_position, m_slot_count,
                output.begin());
    return true;
}

bool ItemSlot::set_item_info_all(
    std::span<const mxh::game::ItemBase> input) noexcept {
    if (input.size() < m_slot_count) return false;
    std::copy_n(input.begin(), m_slot_count,
                m_items.begin() + m_start_abs_position);
    return true;
}

ItemError ItemSlot::insert_item_abs(std::uint16_t abs_position,
                                    mxh::game::ItemBase& item,
                                    std::uint16_t state) noexcept {
    if (!is_position_inside(abs_position)) return ItemError::OutOfPosition;
    if (item.Position != abs_position) item.Position = abs_position;
    if (!is_empty_inner(abs_position)) return ItemError::AlreadyExists;
    if (!has_flag(state, SS_LOCKOMIT) && is_lock(abs_position)) {
        return ItemError::Locked;
    }
    if (is_password(abs_position)) return ItemError::Password;

    if (!has_flag(state, SS_PREINSERT) && m_is_duplicate_item &&
        m_is_duplicate_item(m_items[abs_position].wIconIdx) &&
        m_items[abs_position].Durability > MAX_YOUNGYAKITEM_DUPNUM) {
        return ItemError::DataMismatch;
    }

    m_items[abs_position] = item;
    m_slot_info[abs_position].wState =
        static_cast<std::uint16_t>(state & ~SS_LOCKOMIT);
    m_slot_info[abs_position].bLock = 0;
    return ItemError::Success;
}

ItemError ItemSlot::update_item_abs(std::uint16_t abs_position,
                                    std::uint32_t db_idx,
                                    std::uint16_t item_idx,
                                    std::uint16_t position,
                                    std::uint16_t quick_position,
                                    std::uint32_t durability,
                                    std::uint16_t flags,
                                    std::uint16_t state,
                                    std::uint32_t rare_db_idx) noexcept {
    if (!is_position_inside(abs_position)) return ItemError::OutOfPosition;
    if (has_flag(flags, UB_ABSPOS) && position != abs_position) {
        position = abs_position;
    }
    if (!has_flag(state, SS_LOCKOMIT) && is_lock(abs_position)) {
        return ItemError::Locked;
    }
    if (has_flag(state, SS_CHKDBIDX) &&
        db_idx != m_items[abs_position].dwDBIdx) {
        return ItemError::DataMismatch;
    }
    if (is_password(abs_position)) return ItemError::Password;

    if (has_flag(flags, UB_ICONIDX)) {
        m_items[abs_position].wIconIdx = item_idx;
    }
    if (has_flag(flags, UB_QABSPOS)) {
        m_items[abs_position].QuickPosition = quick_position;
    }
    if (has_flag(flags, UB_ABSPOS)) {
        m_items[abs_position].Position = position;
    }

    if (m_is_duplicate_item &&
        m_is_duplicate_item(m_items[abs_position].wIconIdx) &&
        m_items[abs_position].Durability > MAX_YOUNGYAKITEM_DUPNUM) {
        return ItemError::DataMismatch;
    }

    if (has_flag(flags, UB_DURA)) {
        m_items[abs_position].Durability = durability;
    }
    if (has_flag(flags, UB_RARE)) {
        m_items[abs_position].RareIdx = rare_db_idx;
    }

    m_slot_info[abs_position].wState = static_cast<std::uint16_t>(
        state & ~(SS_LOCKOMIT | SS_CHKDBIDX));
    m_slot_info[abs_position].bLock = 0;
    return ItemError::Success;
}

ItemError ItemSlot::delete_item_abs(std::uint16_t abs_position,
                                    mxh::game::ItemBase* pItemOut,
                                    std::uint16_t state) noexcept {
    if (!is_position_inside(abs_position)) return ItemError::OutOfPosition;
    if (is_empty_inner(abs_position)) return ItemError::NotFound;

    const bool shop_position =
        (abs_position >= mxh::game::TP_SHOPITEM_START &&
         abs_position < mxh::game::TP_SHOPITEM_END) ||
        (abs_position >= mxh::game::TP_SHOPINVEN_START &&
         abs_position < mxh::game::TP_SHOPINVEN_END);
    if (!shop_position && !has_flag(state, SS_LOCKOMIT) &&
        is_lock(abs_position)) {
        return ItemError::Locked;
    }
    if (is_password(abs_position)) return ItemError::Password;

    if (pItemOut) *pItemOut = m_items[abs_position];
    clear_item_base(m_items[abs_position]);
    clear_slot_info(m_slot_info[abs_position]);
    return ItemError::Success;
}

bool ItemSlot::is_empty(std::uint16_t abs_position) const noexcept {
    if (!is_position_inside(abs_position)) return false;
    return !m_slot_info[abs_position].bLock &&
           m_slot_info[abs_position].wState == SS_NONE &&
           m_items[abs_position].dwDBIdx == 0;
}

bool ItemSlot::set_lock(std::uint16_t abs_position, bool value) noexcept {
    if (!is_position_inside(abs_position)) return false;
    m_slot_info[abs_position].bLock = value ? 1 : 0;
    return true;
}

bool ItemSlot::is_lock(std::uint16_t abs_position) const noexcept {
    if (!is_position_inside(abs_position)) return false;
    return m_slot_info[abs_position].bLock != 0;
}

bool ItemSlot::is_password(std::uint16_t abs_position) const noexcept {
    if (!is_position_inside(abs_position)) return false;
    return m_slot_info[abs_position].wPassword != 0;
}

std::uint16_t ItemSlot::item_count() const noexcept {
    std::uint16_t count = 0;
    const auto end = static_cast<std::uint16_t>(
        m_start_abs_position + m_slot_count);
    for (std::uint16_t position = m_start_abs_position;
         position < end; ++position) {
        if (!is_empty(position)) ++count;
    }
    return count;
}

bool ItemSlot::is_position_inside(std::uint16_t abs_position) const noexcept {
    const auto end = static_cast<std::size_t>(m_start_abs_position) +
                     m_slot_count;
    return abs_position >= m_start_abs_position && abs_position < end;
}

bool ItemSlot::is_empty_inner(std::uint16_t abs_position) const noexcept {
    return m_slot_info[abs_position].wState != SS_NONE ||
           m_items[abs_position].dwDBIdx == 0;
}

bool ItemSlot::set_slot_count(std::uint16_t slot_count) noexcept {
    const auto end = static_cast<std::size_t>(m_start_abs_position) +
                     slot_count;
    if (end > m_items.size() || end > m_slot_info.size()) return false;
    m_slot_count = slot_count;
    return true;
}

std::uint16_t InventoryItemSlot::get_empty_cell(
    std::uint16_t* pEmptyCellPositions,
    std::uint16_t need_count) const noexcept {
    if (need_count == 0) return 0;

    std::uint16_t empty_count = 0;
    const auto end = static_cast<std::uint16_t>(
        m_start_abs_position + m_slot_count);
    for (std::uint16_t position = m_start_abs_position;
         position < end; ++position) {
        if (!is_empty(position)) continue;
        if (pEmptyCellPositions) {
            pEmptyCellPositions[empty_count++] = position;
        }
        if (empty_count == need_count) break;
    }
    return empty_count;
}

bool InventoryItemSlot::check_quick_position_for_item(
    std::uint16_t item_idx) const noexcept {
    const auto end = static_cast<std::uint16_t>(
        m_start_abs_position + m_slot_count);
    for (std::uint16_t position = m_start_abs_position;
         position < end; ++position) {
        const auto* pItem = get_item_info_abs(position);
        if (pItem->wIconIdx == item_idx && pItem->QuickPosition != 0) {
            return false;
        }
    }
    return true;
}

bool InventoryItemSlot::check_item_lock_for_item(
    std::uint16_t item_idx) const noexcept {
    const auto end = static_cast<std::uint16_t>(
        m_start_abs_position + m_slot_count);
    for (std::uint16_t position = m_start_abs_position;
         position < end; ++position) {
        const auto* pItem = get_item_info_abs(position);
        if (pItem->wIconIdx == item_idx && is_lock(position)) {
            return false;
        }
    }
    return true;
}

bool InventoryItemSlot::set_extra_slot_count(std::uint32_t count) noexcept {
    constexpr std::uint32_t kTabCellCount = 20;
    constexpr std::uint32_t kGivenInventoryTabs = 2;
    const auto requested = kTabCellCount * (kGivenInventoryTabs + count);
    if (requested > std::numeric_limits<std::uint16_t>::max()) return false;
    if (!set_slot_count(static_cast<std::uint16_t>(requested))) return false;
    m_extra_slot_count = count;
    return true;
}

}  // namespace mxh::server
