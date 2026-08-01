#pragma once

#include "mxh/game/item_types.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace mxh::server {

inline constexpr std::uint16_t UB_DBIDX = 1;
inline constexpr std::uint16_t UB_ICONIDX = 2;
inline constexpr std::uint16_t UB_ABSPOS = 4;
inline constexpr std::uint16_t UB_QABSPOS = 8;
inline constexpr std::uint16_t UB_DURA = 16;
inline constexpr std::uint16_t UB_RARE = 32;
inline constexpr std::uint16_t UB_ALL = 63;

inline constexpr std::uint16_t SS_NONE = 0;
inline constexpr std::uint16_t SS_PREINSERT = 1;
inline constexpr std::uint16_t SS_LOCKOMIT = 2;
inline constexpr std::uint16_t SS_CHKDBIDX = 4;

inline constexpr std::uint32_t MAX_YOUNGYAKITEM_DUPNUM = 20;

#pragma pack(push, 1)
struct SlotInfo {
    std::int32_t bLock = 0;
    std::uint16_t wPassword = 0;
    std::uint16_t wState = 0;
};
#pragma pack(pop)

static_assert(sizeof(SlotInfo) == 8);
static_assert(offsetof(SlotInfo, bLock) == 0);
static_assert(offsetof(SlotInfo, wPassword) == 4);
static_assert(offsetof(SlotInfo, wState) == 6);

enum class ItemError : std::uint8_t {
    Success = 0,
    OutOfPosition = 1,
    DataMismatch = 2,
    AlreadyExists = 3,
    NotFound = 4,
    Locked = 5,
    Password = 6,
    NotEnoughMoney = 7,
    NoSpace = 8,
    MaxMoney = 9,
};

class ItemSlot {
public:
    using DuplicateItemPredicate = std::function<bool(std::uint16_t)>;

    bool init(std::uint16_t start_abs_position,
              std::uint16_t slot_count,
              std::span<mxh::game::ItemBase> items,
              std::span<SlotInfo> slot_info,
              DuplicateItemPredicate is_duplicate_item = {}) noexcept;

    const mxh::game::ItemBase* get_item_info_abs(std::uint16_t abs_position) const noexcept;
    bool get_item_info_all(std::span<mxh::game::ItemBase> output) const noexcept;
    bool set_item_info_all(std::span<const mxh::game::ItemBase> input) noexcept;

    ItemError insert_item_abs(std::uint16_t abs_position,
                              mxh::game::ItemBase& item,
                              std::uint16_t state = SS_NONE) noexcept;
    ItemError update_item_abs(std::uint16_t abs_position,
                              std::uint32_t db_idx,
                              std::uint16_t item_idx,
                              std::uint16_t position,
                              std::uint16_t quick_position,
                              std::uint32_t durability,
                              std::uint16_t flags = UB_ALL,
                              std::uint16_t state = SS_NONE,
                              std::uint32_t rare_db_idx = 0) noexcept;
    ItemError delete_item_abs(std::uint16_t abs_position,
                              mxh::game::ItemBase* pItemOut = nullptr,
                              std::uint16_t state = SS_NONE) noexcept;

    std::uint16_t start_position() const noexcept { return m_start_abs_position; }
    std::uint16_t slot_count() const noexcept { return m_slot_count; }
    bool is_empty(std::uint16_t abs_position) const noexcept;
    bool set_lock(std::uint16_t abs_position, bool value) noexcept;
    bool is_lock(std::uint16_t abs_position) const noexcept;
    bool is_password(std::uint16_t abs_position) const noexcept;
    std::uint16_t item_count() const noexcept;

protected:
    bool is_position_inside(std::uint16_t abs_position) const noexcept;
    bool is_empty_inner(std::uint16_t abs_position) const noexcept;
    bool set_slot_count(std::uint16_t slot_count) noexcept;

    std::span<mxh::game::ItemBase> m_items;
    std::span<SlotInfo> m_slot_info;
    std::uint16_t m_start_abs_position = 0;
    std::uint16_t m_slot_count = 0;
    DuplicateItemPredicate m_is_duplicate_item;
};

class InventoryItemSlot final : public ItemSlot {
public:
    std::uint16_t get_empty_cell(std::uint16_t* pEmptyCellPositions,
                                 std::uint16_t need_count = 1) const noexcept;
    bool check_quick_position_for_item(std::uint16_t item_idx) const noexcept;
    bool check_item_lock_for_item(std::uint16_t item_idx) const noexcept;
    bool set_extra_slot_count(std::uint32_t count) noexcept;
    std::uint32_t extra_slot_count() const noexcept { return m_extra_slot_count; }

private:
    std::uint32_t m_extra_slot_count = 0;
};

}  // namespace mxh::server
