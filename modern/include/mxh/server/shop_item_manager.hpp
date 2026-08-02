// shop_item_manager.hpp
//
// 1:1 port of legacy [Server]Map/ShopItemManager.h CShopItemManager.
// Manages per-player shop-item accounting: in-flight using-items (time-bound
// shop-bought items that grant buffs), saved-move-points, and the legacy dup
// counters that prevent double-application of certain shop items.
//
// Locked invariants (1:1 with legacy):
//   - Pool sizing: UsingItemPool and MovePointPool are each 50 max with 10
//     increment (legacy CMemoryPoolTempl init values).
//   - Hash-table capacities: UsingItemTable 50, MovePointTable 30 (legacy
//     CYHHashTable Initialize values).
//   - Five dup counters (Incantation/Charm/Herb/Sundries/PetEquip) and a
//     single ProtectItemIdx, all reset to 0 by Init() and Release().
//   - Init sets m_pPlayer (opaque pointer); Release sets it back to nullptr.
//   - The state machine is intentionally not coupled to AbilityManager /
//     Battle / SiegeWar / WeatherManager / etc.; those hooks live in a
//     follow-up commit. This skeleton only locks the data-plane (init,
//     release, dup counters, table CRUD) so legacy invariants can be tested
//     in isolation.
//
// This header is header-only (no compiled .cpp needed for the data plane)
// so it stays trivially testable and matches the modern pattern of small
// inline-only manager classes that anchor the legacy ABI.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "mxh/game/shop_item_types.hpp"

namespace mxh::server {

// Legacy ShopItemManager pool sizing. Kept here as named constants so tests
// and the data plane cannot drift from the legacy values.
inline constexpr std::size_t SHOP_ITEM_MANAGER_USING_POOL_MAX = 50u;
inline constexpr std::size_t SHOP_ITEM_MANAGER_USING_POOL_INCREMENT = 10u;
inline constexpr std::size_t SHOP_ITEM_MANAGER_MOVE_POOL_MAX = 50u;
inline constexpr std::size_t SHOP_ITEM_MANAGER_MOVE_POOL_INCREMENT = 10u;
inline constexpr std::size_t SHOP_ITEM_MANAGER_USING_TABLE_CAPACITY = 50u;
inline constexpr std::size_t SHOP_ITEM_MANAGER_MOVE_TABLE_CAPACITY = 30u;

inline constexpr std::uint32_t SHOP_ITEM_DUP_NONE = 0u;

// Legacy timer thresholds from CShopItemManager::CheckEndTime.
inline constexpr std::uint32_t SHOP_ITEM_CHECK_INTERVAL_MS = 30000u;
inline constexpr std::uint32_t SHOP_ITEM_UPDATE_INTERVAL_MS = 600000u;

// Tracks one using-item row, mirror of legacy SHOPITEMWITHTIME plus the
// 64-bit object id used as the table key (legacy dwItemIdx).
struct UsingShopItemEntry {
    std::uint64_t ItemIdx = 0;
    game::ShopItemWithTime Data{};
};

// Tracks one move-point row, mirror of legacy MOVEDATA keyed by DBIdx.
struct MovePointEntry {
    std::uint32_t DBIdx = 0;
    game::MoveData Data{};
};

class ShopItemManager final {
public:
    ShopItemManager() = default;

    // init(): legacy CShopItemManager::Init(CPlayer*). Sets the opaque
    // player pointer and resets all counters, tables, and timers to 0.
    // After init, the manager holds exactly one weak reference to p_player.
    void init(void* p_player) noexcept;

    // release(): legacy CShopItemManager destructor + Release path. Clears
    // the player pointer and empties the tables. Counters are zeroed so a
    // subsequent init() call starts fresh.
    void release() noexcept;

    // Getters for the five legacy dup counters. All return 0 after init.
    std::uint32_t dup_incantation() const noexcept { return m_DupIncantation; }
    std::uint32_t dup_charm()       const noexcept { return m_DupCharm; }
    std::uint32_t dup_herb()        const noexcept { return m_DupHerb; }
    std::uint32_t dup_sundries()    const noexcept { return m_DupSundries; }
    std::uint32_t dup_pet_equip()   const noexcept { return m_DupPetEquip; }

    // Legacy setter: increments one dup counter by 1. Returns the new value.
    std::uint32_t bump_dup_incantation() noexcept;
    std::uint32_t bump_dup_charm()       noexcept;
    std::uint32_t bump_dup_herb()        noexcept;
    std::uint32_t bump_dup_sundries()    noexcept;
    std::uint32_t bump_dup_pet_equip()   noexcept;

    // Reset all five dup counters to 0. Legacy called this on logout/cleanup.
    void clear_dup_counters() noexcept;

    // ProtectItemIdx: legacy m_ProtectItemIdx. Stores the item index that
    // protected the player from a particular shop-item debuff (e.g. revival
    // scroll). Defaults to 0 (no protection).
    std::uint32_t protect_item_idx() const noexcept { return m_ProtectItemIdx; }
    void set_protect_item_idx(std::uint32_t idx) noexcept { m_ProtectItemIdx = idx; }

    // Timer fields exposed for the legacy CheckEndTime()/Update loop. Both
    // are DWORD milliseconds-since-init mirrors of m_Checktime/m_Updatetime.
    std::uint32_t check_time()    const noexcept { return m_Checktime; }
    std::uint32_t update_time()   const noexcept { return m_Updatetime; }
    void set_check_time(std::uint32_t v) noexcept { m_Checktime = v; }
    void set_update_time(std::uint32_t v) noexcept { m_Updatetime = v; }

    // Using-item table CRUD (legacy AddUsingShopItem + DeleteUsingShopItem).
    // add_using_item returns false if a row with the same item_idx already
    // exists (1:1 with legacy "don't re-add" semantics).
    bool add_using_item(const UsingShopItemEntry& entry) noexcept;
    bool has_using_item(std::uint64_t item_idx) const noexcept;
    std::size_t using_item_count() const noexcept { return m_usingItems.size(); }
    const UsingShopItemEntry* find_using_item(std::uint64_t item_idx) const noexcept;
    bool delete_using_item(std::uint64_t item_idx) noexcept;

    // Legacy CShopItemManager::UsedShopItem data plane.
    // Inserts a new using-item row keyed by item_base.wIconIdx (1:1 with
    // legacy m_UsingItemTable.Add(ShopItem, pItemBase->wIconIdx)). Returns
    // false if the icon index is already present (the legacy 'this item
    // is already in use' guard) or if wIconIdx is zero (invalid item).
    // LastCheckTime is set to now_ms (legacy uses gCurTime; the caller
    // passes the same clock value the runtime passes into tick()).
    //
    // The full legacy UsedShopItem path also adjusts ShopItemOption
    // (SkillPoint / StatePoint counters) for special incantation items
    // and writes the row to DB; those hooks live outside the data
    // plane and remain in a follow-up commit.
    bool used_shop_item(const game::ItemBase& item_base,
                        std::uint32_t param,
                        game::PackedTime begin_time,
                        std::uint32_t remain_ms,
                        std::uint32_t now_ms) noexcept;

    // True iff an entry keyed by wIconIdx already exists. Cheap O(1) hash
    // lookup that mirrors legacy m_UsingItemTable.GetData(wIconIdx).
    bool has_using_item_by_icon_idx(std::uint16_t icon_idx) const noexcept;

    // O(1) lookup by wIconIdx, returns nullptr if missing.
    const UsingShopItemEntry* find_using_item_by_icon_idx(
        std::uint16_t icon_idx) const noexcept;

    // True if this icon index has a remainder time still in the future at
    // now_ms (legacy 'active using-item' predicate before any DB flush).
    bool is_using_item_active(std::uint16_t icon_idx,
                              std::uint32_t now_ms) const noexcept;

    // Move-point table CRUD (legacy MovePointTable operations).
    bool add_move_point(const MovePointEntry& entry) noexcept;
    std::size_t move_point_count() const noexcept { return m_movePoints.size(); }
    const MovePointEntry* find_move_point(std::uint32_t db_idx) const noexcept;
    bool delete_move_point(std::uint32_t db_idx) noexcept;

    // Opaque player pointer accessors. After release() this returns nullptr.
    void* player() const noexcept { return m_pPlayer; }

    // Build the SEND_SHOPITEM_USEDINFO wire bytes for this manager,
    // 1:1 with legacy CShopItemManager::SendShopItemUseInfo(). The header
    // is zeroed except for category/protocol (caller supplies them via
    // `category` and `protocol`). The bytes are MSGBASE + WORD ItemCount
    // + ItemCount * ShopItemBase, trimmed exactly as the legacy
    // SEND_SHOPITEM_USEDINFO::GetSize() does. Returns an empty vector
    // if the table exceeds the legacy 100-entry maximum (which cannot
    // happen at the data plane since the pool capacity is 50).
    std::vector<std::uint8_t> serialize_using_items(
        std::uint8_t category, std::uint8_t protocol) const;

    // Same as serialize_using_items() but leaves the header category/
    // protocol bytes at 0. Useful for tests that only care about the
    // ItemCount + Item[N] layout.
    std::vector<std::uint8_t> serialize_using_items_headerless() const;

    // Const read-only access to the underlying tables for serialization
    // (legacy SendShopItemUseInfo sends the entire table on demand).
    const std::unordered_map<std::uint64_t, UsingShopItemEntry>& using_items() const noexcept {
        return m_usingItems;
    }
    // Tick the timer fields (m_Checktime, m_Updatetime) by delta_ms.
    // Returns true if m_Updatetime wrapped past the 10-minute mark
    // (legacy uses this to flush ShopItemAllUseInfo to DB).
    bool tick(uint32_t delta_ms) noexcept;

    // True once m_Checktime has reached 30000 ms (legacy throttle: the
    // full CheckEndTime sweep only runs every ~30s).
    bool check_due() const noexcept { return m_Checktime >= 30000u; }

    // Scan using-items and append the indices of items whose remaining
    // time at LastCheckTime would have elapsed by now_ms. Pure data-plane:
    // no DB writes, no ShopItemOption updates, no ItemManager calls.
    void collect_expired(uint32_t now_ms, std::vector<std::uint64_t>& out) const;

    // Convenience wrapper: tick by delta_ms and, if check_due(), append
    // expired item indices. Returns the number of expired entries.
    std::size_t tick_and_collect_expired(uint32_t delta_ms, uint32_t now_ms,
                                          std::vector<std::uint64_t>& out);

    // Reset m_Checktime after a check sweep so the next 30-second window
    // begins. Legacy resets to 0 unconditionally.
    void clear_check_time() noexcept { m_Checktime = 0; }

    // Reset m_Updatetime after a DB flush. Legacy resets to 0.
    void clear_update_time() noexcept { m_Updatetime = 0; }
    const std::unordered_map<std::uint32_t, MovePointEntry>& move_points() const noexcept {
        return m_movePoints;
    }

private:
    void* m_pPlayer = nullptr;

    std::uint32_t m_DupIncantation = 0;
    std::uint32_t m_DupCharm       = 0;
    std::uint32_t m_DupHerb        = 0;
    std::uint32_t m_DupSundries    = 0;
    std::uint32_t m_DupPetEquip    = 0;

    std::uint32_t m_ProtectItemIdx = 0;
    std::uint32_t m_Checktime      = 0;
    std::uint32_t m_Updatetime     = 0;

    std::unordered_map<std::uint64_t, UsingShopItemEntry> m_usingItems;
    std::unordered_map<std::uint32_t, MovePointEntry>     m_movePoints;
};

} // namespace mxh::server
