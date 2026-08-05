// shop_item_manager.cpp - data-plane implementation of legacy CShopItemManager.
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
    return deadline > static_cast<std::uint64_t>(now_ms);
}

bool ShopItemManager::is_memory_move_extend_icon(std::uint16_t icon_idx) noexcept {
    return icon_idx == INCANTATION_MEMORY_MOVE_EXTEND
        || icon_idx == INCANTATION_MEMORY_MOVE_EXTEND7
        || icon_idx == INCANTATION_MEMORY_MOVE2
        || icon_idx == INCANTATION_MEMORY_MOVE_EXTEND30;
}

std::size_t ShopItemManager::move_point_capacity() const noexcept {
    // Legacy: capacity is MAX_MOVEDATA_PER_PAGE (10) unless any of the 4
    // MemoryMove incantations is currently in the using-items table, in
    // which case it doubles to MAX_MOVEDATA_PER_PAGE*MAX_MOVEPOINT_PAGE (20).
    for (const auto& kv : m_usingItems) {
        if (is_memory_move_extend_icon(static_cast<std::uint16_t>(kv.first))) {
            return MAX_MOVEDATA_PER_PAGE_MODERN * game::MAX_MOVEPOINT_PAGE_MODERN;
        }
    }
    return MAX_MOVEDATA_PER_PAGE_MODERN;
}

bool ShopItemManager::add_move_point(const MovePointEntry& entry) noexcept {
    if (entry.DBIdx == 0) return false;  // legacy refused DBIdx == 0
    if (m_movePoints.find(entry.DBIdx) != m_movePoints.end()) return false;
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

UsingShopItemEntry* ShopItemManager::find_using_item_by_icon_idx_mutable(
    std::uint16_t icon_idx) noexcept {
    auto it = m_usingItems.find(static_cast<std::uint64_t>(icon_idx));
    if (it == m_usingItems.end()) return nullptr;
    return &it->second;
}

bool ShopItemManager::add_shop_item_use(const game::ShopItemBase& shop_item,
                                       std::uint32_t now_ms) noexcept {
    if (shop_item.ItemBase.wIconIdx == 0) return false;
    const std::uint64_t key = shop_item.ItemBase.wIconIdx;
    if (m_usingItems.find(key) != m_usingItems.end()) return false;
    UsingShopItemEntry e{};
    e.ItemIdx = key;
    e.Data.ShopItem = shop_item;
    e.Data.LastCheckTime = now_ms;
    m_usingItems.emplace(key, e);
    return true;
}

bool ShopItemManager::rename_move_point(std::uint32_t db_idx,
                                        std::string_view new_name) noexcept {
    auto it = m_movePoints.find(db_idx);
    if (it == m_movePoints.end()) return false;
    auto& name = it->second.Data.Name;
    name.fill(0);
    const std::size_t n = std::min(new_name.size(), name.size() - 1);
    std::memcpy(name.data(), new_name.data(), n);
    return true;
}

void ShopItemManager::release_move_points() noexcept {
    m_movePoints.clear();
}

// ---- D4.19 use_shop_item_decision (free function) ----
//
// Implements the data-plane portion of legacy
// CShopItemManager::UseShopItem that *is* observable on the wire: the
// guard sequence at the top of the function (empty-slot reject,
// ITEMMGR->GetItemInfo reject, table-already-has-wIconIdx reject) and
// the BeginTime/Remaintime computation in the middle. Player/Battle/
// event-map/ItemManager side effects are not in scope here; they are
// driven by the Agent/Map handlers in a follow-up commit.

namespace {

// Encode a PackedTime offset (in minutes) using the legacy stTIME bit
// layout so that adding minutes crosses the day boundary the same way
// legacy did.  We only support up to ~25 hours of offset because the
// SHOPITEMBASE EndTime field is a packed 32-bit calendar value, just
// like BeginTime.
game::PackedTime add_minutes_to_packed(game::PackedTime start, std::uint32_t minutes) {
    auto clamp6 = [](std::uint32_t v) -> std::uint32_t {
        return v & 0x3Fu;
    };
    auto clamp4 = [](std::uint32_t v) -> std::uint32_t {
        return v & 0x0Fu;
    };

    std::uint32_t year   = start.year();
    std::uint32_t month  = start.month();
    std::uint32_t day    = start.day();
    std::uint32_t hour   = start.hour();
    std::uint32_t minute = start.minute();

    // legacy Rarity encodes day*1440 + hour*60 + min; we accept the same
    // shape and decompose here.  Anything beyond 6 bits is silently
    // truncated, matching the legacy packed-time field width.
    const std::uint32_t add_day    = (minutes / (24u * 60u)) & 0x3Fu;
    const std::uint32_t add_hour   = ((minutes % (24u * 60u)) / 60u) & 0x3Fu;
    const std::uint32_t add_minute = (minutes % 60u) & 0x3Fu;

    day   = clamp6(day   + add_day);
    hour  = clamp6(hour  + add_hour);
    minute = clamp6(minute + add_minute);
    (void)clamp4(year);
    (void)clamp4(month);

    return game::PackedTime{
        (year   << 28)
      | (month  << 24)
      | (day    << 18)
      | (hour   << 12)
      | (minute << 6)
      | start.second()};
}

// Route an ItemInfo.ItemKind to the matching dup counter. 1:1 with
// legacy m_DupIncantation / m_DupCharm / m_DupHerb / m_DupSundries /
// m_DupPetEquip selections inside CShopItemManager::UseShopItem.
ShopItemDupSlot route_dup_slot(std::uint16_t item_kind) noexcept {
    switch (item_kind) {
        case LEGACY_SHOP_ITEM_INCANTATION: return ShopItemDupSlot::Incantation;
        case LEGACY_SHOP_ITEM_CHARM:       return ShopItemDupSlot::Charm;
        case LEGACY_SHOP_ITEM_HERB:        return ShopItemDupSlot::Herb;
        case LEGACY_SHOP_ITEM_SUNDRIES:    return ShopItemDupSlot::Sundries;
        case LEGACY_SHOP_ITEM_PET_EQUIP:   return ShopItemDupSlot::PetEquip;
        default:                           return ShopItemDupSlot::None;
    }
}

} // namespace

UseShopItemDecision use_shop_item_decision(
    const ShopItemManager& mgr,
    const game::ItemBase& item_base,
    const ItemInfoView& info,
    game::PackedTime now_packed,
    std::uint32_t now_ms) noexcept {
    UseShopItemDecision out;

    // 1. Empty-slot guard: legacy CShopItemManager::UseShopItem returns
    //    eItemUseErr_Err when pItemBase is null OR wIconIdx is 0.
    if (item_base.wIconIdx == 0u) {
        out.status = UseShopItemStatus::InvalidIcon;
        return out;
    }

    // 2. ItemInfo guard: legacy returns eItemUseErr_Err when ITEMMGR->
    //    GetItemInfo(UseBaseInfo.ShopItemIdx) is null. Our Slim view
    //    expresses that with all four fields zero.
    if (info.SellPrice == 0u && info.Rarity == 0u && info.ItemKind == 0u
        && info.ItemType == 0u) {
        out.status = UseShopItemStatus::ItemInfoMissing;
        return out;
    }

    // 3. Already-in-use guard: legacy m_UsingItemTable.GetData(wIconIdx)
    //    returning non-null -> eItemUseErr_AlreadyUse.
    if (mgr.has_using_item_by_icon_idx(item_base.wIconIdx)) {
        out.status = UseShopItemStatus::AlreadyInUse;
        return out;
    }

    // 4. Compute BeginTime / Remaintime + dup slot, mirroring the legacy
    //    SHOP_ITEM_PARAM_STORED_TIME / SHOP_ITEM_PARAM_PLAY_TIME /
    //    SHOP_ITEM_USE_PARAM_CONTINUE / SellPrice==0 branches.
    out.shop_item.ItemBase = item_base;
    out.shop_item.Param    = info.SellPrice;
    out.shop_item.BeginTime = game::PackedTime{0};
    out.shop_item.Remaintime = 0u;
    out.last_check_ms = now_ms;
    out.dup_slot = route_dup_slot(info.ItemKind);

    if (info.SellPrice == SHOP_ITEM_USE_PARAM_REALTIME) {
        // legacy: endtime = startime + Rarity (minutes), Remaintime = endtime.value
        out.shop_item.BeginTime = now_packed;
        out.shop_item.Remaintime = add_minutes_to_packed(now_packed, info.Rarity).value;
    } else if (info.SellPrice == SHOP_ITEM_USE_PARAM_PLAYTIME) {
        // legacy: Remaintime = Rarity * 60_000 ms
        out.shop_item.BeginTime = now_packed;
        out.shop_item.Remaintime = info.Rarity * 60000u;
    } else if (info.SellPrice == SHOP_ITEM_USE_PARAM_CONTINUE) {
        // legacy: Remaintime = 0, LastCheckTime = 0 (timer never expires)
        out.shop_item.BeginTime = game::PackedTime{0};
        out.shop_item.Remaintime = 0u;
        out.last_check_ms = 0u;
    } else {
        // legacy: SellPrice == 0 (one-shot buff). No timer, BeginTime = 0.
        out.shop_item.BeginTime = game::PackedTime{0};
        out.shop_item.Remaintime = 0u;
    }

    out.status = UseShopItemStatus::Ok;
    return out;
}

[[maybe_unused]] constexpr int shop_item_manager_translation_unit_anchor = 0;

} // namespace mxh::server
