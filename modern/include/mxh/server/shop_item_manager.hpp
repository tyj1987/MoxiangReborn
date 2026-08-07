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
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mxh/game/shop_item_types.hpp"
#include <mxh/server/legacy_shop_item_kind.hpp>

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

// Legacy CShopItemManager::AddMovePoint capacity gate. When ANY of the
// Memory-Move Extend incantation items (55365 / 55390 / 55371 / 58010) is
// currently in the using-item table, the move-point capacity doubles
// from MAX_MOVEDATA_PER_PAGE (10) to MAX_MOVEDATA_PER_PAGE*MAX_MOVEPOINT_PAGE (20).
inline constexpr std::uint16_t INCANTATION_MEMORY_MOVE_EXTEND    = 55365u;
inline constexpr std::uint16_t INCANTATION_MEMORY_MOVE_EXTEND7   = 55390u;
inline constexpr std::uint16_t INCANTATION_MEMORY_MOVE2          = 55371u;
inline constexpr std::uint16_t INCANTATION_MEMORY_MOVE_EXTEND30  = 58010u;
inline constexpr std::size_t   MAX_MOVEDATA_PER_PAGE_MODERN      = 10u;

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
    // Capacity for the move-point table. Returns MAX_MOVEDATA_PER_PAGE
    // (10) by default and MAX_MOVEDATA_PER_PAGE*MAX_MOVEPOINT_PAGE (20)
    // when any of the 4 legacy MemoryMove incantation items is currently
    // in the using-items table. Mirrors legacy AddMovePoint gate exactly.
    std::size_t move_point_capacity() const noexcept;

    // True iff a using-item with the given wIconIdx is one of the 4
    // incantation items that expand the move-point table.
    static bool is_memory_move_extend_icon(std::uint16_t icon_idx) noexcept;

    std::size_t move_point_count() const noexcept { return m_movePoints.size(); }
    const MovePointEntry* find_move_point(std::uint32_t db_idx) const noexcept;
    bool delete_move_point(std::uint32_t db_idx) noexcept;

    // Opaque player pointer accessors. After release() this returns nullptr.
    void* player() const noexcept { return m_pPlayer; }

    // ---- Pure-data query/mutate helpers (D4.18) ----

    // Legacy CShopItemManager::GetUsingItemInfo(ItemIdx). O(1) lookup
    // by wIconIdx returning a mutable pointer; returns nullptr if the
    // icon is not currently in the using-items table.
    UsingShopItemEntry* find_using_item_by_icon_idx_mutable(
        std::uint16_t icon_idx) noexcept;

    // Legacy CShopItemManager::AddShopItemUse(pShopItem) -- inserts a
    // pre-built ShopItemBase into the using-items table keyed by wIconIdx
    // with LastCheckTime=now_ms. Returns false if wIconIdx is 0 (invalid)
    // or already present (legacy Add() guard). The full legacy UsedShopItem
    // path also sets BeginTime/Remaintime from pShopItem -- this helper
    // preserves those exact fields and only assigns LastCheckTime, so
    // callers that pre-build the ShopItemBase get byte-identical state.
    bool add_shop_item_use(const game::ShopItemBase& shop_item,
                           std::uint32_t now_ms) noexcept;

    // Legacy CShopItemManager::ReNameMovePoint(DBIdx, newName). Copies
    // up to MAX_SAVEDMOVE_NAME-1 bytes from new_name into the row's Name
    // field and NUL-terminates. Returns false if the DBIdx is not in
    // the move-point table.
    bool rename_move_point(std::uint32_t db_idx,
                            std::string_view new_name) noexcept;

    // Legacy CShopItemManager::ReleseMovePoint() (legacy typo preserved
    // in a comment). Clears the move-point table only -- the using-items
    // table and the dup counters are untouched. Use release() to clear
    // all state.
    void release_move_points() noexcept;

    // Legacy CShopItemManager::GetSavedMPNum() -- alias for the move
    // point count (legacy called m_MovePointTable.GetDataNum()).
    std::size_t get_saved_mp_num() const noexcept { return m_movePoints.size(); }

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

    // Build the SEND_MOVEDATA_INFO wire bytes for this manager, 1:1 with
    // legacy CShopItemManager wire for the saved-move-point table. The
    // bytes are MSGBASE + BYTE bInited + WORD Count + Count * MoveData,
    // trimmed exactly as legacy SEND_MOVEDATA_INFO::GetSize() does.
    // Returns an empty vector if the table exceeds the legacy 20-entry
    // maximum (the pool capacity is 50 so this cannot happen at the
    // data plane unless the caller bypasses add_move_point).
    std::vector<std::uint8_t> serialize_move_points(
        std::uint8_t category, std::uint8_t protocol,
        bool b_inited) const;

    // Same as serialize_move_points() but leaves the header category/
    // protocol bytes at 0. Useful for tests that only care about the
    // bInited + Count + Data[N] layout.
    std::vector<std::uint8_t> serialize_move_points_headerless(
        bool b_inited) const;

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

    // ---- D4.21 CheckEndTime realtime branch ----
    //
    // Legacy CShopItemManager::CheckEndTime scans m_UsingItemTable and
    // deletes every row whose SellPrice == eShopItemUseParam_Realtime
    // (= SHOP_ITEM_PARAM_STORED_TIME) AND whose packed EndTime
    // (stored in Remaintime by use_shop_item_decision Realtime branch)
    // is earlier than the current packed stTIME. This is the data-plane
    // half of that sweep, with the same predicate: only stored-time
    // rows are considered, and the comparison is now.value > end_time.
    // Returns the number of new indices appended to out.
    std::size_t collect_realtime_expired(game::PackedTime now,
                                         std::vector<std::uint64_t>& out) const;

    // Same as collect_realtime_expired but also removes the matching
    // rows from the table and zeroes their Remaintime, matching the
    // legacy DeleteUsingShopItem + Remaintime=0 sequence at the end of
    // CheckEndTime. Use the const collect_realtime_expired first if the
    // caller needs to surface each row to the wire before deletion.
    std::size_t consume_realtime_expired(game::PackedTime now);

    // ---- D4.22 CheckAvatarEndtime data plane ----
    //
    // Legacy CShopItemManager::CheckAvatarEndtime() in
    // [Server]Map/ShopItemManager.cpp:1142-1180 is the avatar-only
    // counterpart to CheckEndTime(). It is invoked from the avatar /
    // weather event loop (e.g. avatar equip / unequip) and shares the
    // same data-plane predicate as CheckEndTime but a simpler
    // side-effect chain. Specifically:
    //
    //   predicate  : SellPrice == eShopItemUseParam_Realtime (= SHOP_ITEM_PARAM_STORED_TIME)
    //             AND Remaintime (packed stTIME end-time) < curtime
    //   skip       : ItemInfo lookup miss (item info not found)
    //   no-timer   : legacy has no m_Checktime gate -- CheckAvatarEndtime
    //             runs unconditionally every tick it is called.
    //
    // The data-plane half lives here. The 4-step side-effect chain
    // (legacy CheckAvatarEndtime body) is the orchestrator's
    // responsibility and is documented in ShopItemEndSideEffect (D4.22).
    //
    // Returns the number of new indices appended to out. Predicate is
    // byte-identical to collect_realtime_expired; this entry point is
    // kept distinct so call sites that wire to the avatar-specific
    // side-effect chain (DiscardItem + SendMsgDwordToPlayer(USEEND)
    // + ShopItemDeleteToDB + LogItemMoney) can document the intent.
    std::size_t collect_avatar_realtime_expired(
        game::PackedTime now, std::vector<std::uint64_t>& out) const;

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

// ---- D4.19 use_shop_item_decision (legacy CShopItemManager::UseShopItem guard + BeginTime/Remaintime write) ----
//
// The legacy CShopItemManager::UseShopItem body is disabled at the source
// level (the whole function is wrapped in /* ... */ comments in the modern
// reference, with a marker noting legacy Korean /'temporary commented by SungDae/')
// and depends on Player, ItemManager, Battle, ChangeItemMgr, AbilityManager,
// StatsCalcManager and the event-map gate. To stay 1:1 with the data plane
// that *is* observable on the wire (the SHOPITEMWITHTIME row that lands in
// the using-items table), this module factors the deterministic decision
// into a free function that:
//
//   1. Rejects ItemInfo-less / IconIdx==0 / dwDBIdx==0 inputs.
//   2. Rejects rows whose wIconIdx is already in the using-items table.
//   3. Computes BeginTime from now_packed (legacy stTIME encoding) and
//      Remaintime from the ItemInfo.SellPrice + Rarity combination, using
//      the same three branches as legacy eShopItemUseParam_*:
//        - SellPrice == SHOP_ITEM_USE_PARAM_REALTIME:  endtime = now + Rarity minutes.
//        - SellPrice == SHOP_ITEM_USE_PARAM_PLAYTIME:  remain = Rarity * 60_000 ms.
//        - SellPrice == SHOP_ITEM_USE_PARAM_CONTINUE:  remain = 0 (forever).
//        - SellPrice == 0:                            remain = 0, BeginTime = 0 (no timer).
//   4. Picks which legacy dup counter the call should bump (incantation /
//      charm / herb / sundries / pet-equip) based on ItemInfo.ItemKind /
//      ItemInfo.ItemType. The caller is responsible for invoking the
//      corresponding bump_dup_* helper after the row is added.
//
// This split keeps the decision free of side effects (no DB writes, no
// Player/Battle lookups) and exactly matches the bytes the legacy server
// wrote into SHOPITEMBASE before it inserted the row via AddShopItemUse().

inline constexpr std::uint32_t SHOP_ITEM_USE_PARAM_REALTIME =
    mxh::game::SHOP_ITEM_PARAM_STORED_TIME;   // 1 -- store with BeginTime / EndTime
inline constexpr std::uint32_t SHOP_ITEM_USE_PARAM_PLAYTIME =
    mxh::game::SHOP_ITEM_PARAM_PLAY_TIME;     // 2 -- countdown by elapsed play time
inline constexpr std::uint32_t SHOP_ITEM_USE_PARAM_CONTINUE = 3u;  // permanent (no expiry)

// Legacy eSHOP_ITEM_* constants live in legacy_shop_item_kind.hpp.

// ItemInfo surface area that the decision needs. The legacy ITEM_INFO is a
// 200+ field struct owned by the resource manager; the modern port does
// not load that table yet, so callers pass a slim view with the four fields
// the decision actually consults (SellPrice, Rarity, ItemKind, ItemType).
struct ItemInfoView {
    std::uint32_t SellPrice = 0;  // eShopItemUseParam_* discriminator
    std::uint32_t Rarity    = 0;  // minutes (legacy Rarity = day*1440 + hour*60 + min)
    std::uint16_t ItemKind  = 0;  // eSHOP_ITEM_* discriminator
    std::uint16_t ItemType  = 0;  // 10 = buff / 11 = avatar etc.
};

// Which legacy dup counter the call should bump after a successful use.
// The mapping mirrors legacy m_DupIncantation/Charm/Herb/Sundries/PetEquip
// routed by ItemKind (and ItemType for the pet-equip edge case).
enum class ShopItemDupSlot : std::uint8_t {
    None        = 0,
    Incantation = 1,
    Charm       = 2,
    Herb        = 3,
    Sundries    = 4,
    PetEquip    = 5,
};

// Result of a use_shop_item_decision call. status is the legacy error
// code (mapped to a small enum). When status == Ok the caller may insert
// shop_item via add_shop_item_use() and bump the dup counter indicated
// by dup_slot.
enum class UseShopItemStatus : std::uint8_t {
    Ok              = 0,
    InvalidIcon     = 1,  // legacy eItemUseErr_Err -- empty slot
    ItemInfoMissing = 2,  // legacy eItemUseErr_Err -- ITEMMGR->GetItemInfo failed
    AlreadyInUse    = 3,  // legacy eItemUseErr_AlreadyUse
};

struct UseShopItemDecision final {
    UseShopItemStatus   status        = UseShopItemStatus::InvalidIcon;
    ShopItemDupSlot     dup_slot      = ShopItemDupSlot::None;
    game::ShopItemBase  shop_item{};
    std::uint32_t       last_check_ms = 0;  // gCurTime analogue to write into LastCheckTime
};

// Compute the deterministic use-shop-item plan. Does not mutate the
// manager. The caller is expected to:
//   - if decision.status == Ok:
//       1. add_shop_item_use(decision.shop_item, decision.last_check_ms)
//       2. bump the dup counter named by decision.dup_slot
//     and then write the new row to DB / update the client.
//   - else: surface decision.status as the eItemUseErr_* result code.
UseShopItemDecision use_shop_item_decision(
    const ShopItemManager& mgr,
    const game::ItemBase& item_base,
    const ItemInfoView& info,
    game::PackedTime now_packed,
    std::uint32_t now_ms) noexcept;

} // namespace mxh::server
