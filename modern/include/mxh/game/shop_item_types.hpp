// shop_item_types.hpp
//
// 1:1 port of legacy [CC]Header/CommonStruct.h SHOPITEMBASE, SHOPITEMWITHTIME,
// and MOVEDATA. The legacy header wraps every struct in #pragma pack(push,1)
// so these types are byte-packed; modern ports ItemBase under the same pack
// (game/item_types.hpp). We extend the same pack region here to keep field
// offsets and total sizes identical.
//
// 1:1 invariants (under #pragma pack(1), mirroring CommonStruct.h):
//   - SHOPITEMBASE     = 34 bytes (ItemBase 22 + Param 4 + BeginTime 4 + Remaintime 4)
//   - SHOPITEMWITHTIME = 38 bytes (SHOPITEMBASE 34 + LastCheckTime 4)
//   - MOVEDATA         = 31 bytes (DBIdx 4 + Name[21] + MapNum 2 + Point 4)
//
// Field semantics (legacy):
//   - SHOPITEMBASE.Param: 1 = stored-time, 2 = play-time (matches SHOPITEMUSEBASE).
//   - SHOPITEMBASE.BeginTime: packed DWORD year<<28|month<<24|day<<18|hour<<12|minute<<6|second.
//   - MOVEDATA.Point: packed WORD x | WORD z (legacy stMOVEPOINT).
//
// Wire compatibility: SHOPITEMBASE / SHOPITEMWITHTIME / MOVEDATA are server-
// side data-plane types; they are not serialized over the modern T-series
// wire. Shop-item use info is sent via dedicated ack/nack messages whose
// payload shape is locked in a separate module. They are however serialized
// over the legacy CommonStruct.h packed layout when the modern AgentServer
// talks to a legacy MapServer (not in scope for this commit).

#pragma once

#include <array>
#include <cstdint>

#include "mxh/game/item_types.hpp"
#include "mxh/net/net.hpp"

namespace mxh::game {

inline constexpr std::size_t SHOP_ITEM_BASE_SIZE = 34u;
inline constexpr std::size_t SHOP_ITEM_WITH_TIME_SIZE = 38u;
inline constexpr std::size_t MOVE_DATA_SIZE = 31u;
inline constexpr std::size_t MAX_SAVED_MOVE_NAME = 21u;
inline constexpr std::uint32_t SHOP_ITEM_PARAM_STORED_TIME = 1u;
inline constexpr std::uint32_t SHOP_ITEM_PARAM_PLAY_TIME = 2u;

#pragma pack(push, 1)

// Packed 4-byte calendar time, 1:1 with legacy stTIME::value.
struct PackedTime {
    std::uint32_t value = 0;

    constexpr PackedTime() noexcept = default;
    constexpr explicit PackedTime(std::uint32_t v) noexcept : value(v) {}

    constexpr std::uint8_t year()   const noexcept { return static_cast<std::uint8_t>((value >> 28) & 0x0Fu); }
    constexpr std::uint8_t month()  const noexcept { return static_cast<std::uint8_t>((value >> 24) & 0x0Fu); }
    constexpr std::uint8_t day()    const noexcept { return static_cast<std::uint8_t>((value >> 18) & 0x3Fu); }
    constexpr std::uint8_t hour()   const noexcept { return static_cast<std::uint8_t>((value >> 12) & 0x3Fu); }
    constexpr std::uint8_t minute() const noexcept { return static_cast<std::uint8_t>((value >> 6)  & 0x3Fu); }
    constexpr std::uint8_t second() const noexcept { return static_cast<std::uint8_t>(value & 0x3Fu); }
};

// 1:1 port of legacy SHOPITEMBASE.
struct ShopItemBase {
    ItemBase ItemBase{};
    std::uint32_t Param = 0;        // SHOP_ITEM_PARAM_STORED_TIME / SHOP_ITEM_PARAM_PLAY_TIME
    PackedTime BeginTime{};
    std::uint32_t Remaintime = 0;  // milliseconds remaining at BeginTime
};

static_assert(sizeof(ShopItemBase) == SHOP_ITEM_BASE_SIZE,
              "ShopItemBase must be 34 bytes to match legacy CommonStruct.h (under pack(1))");

// 1:1 port of legacy SHOPITEMWITHTIME.
struct ShopItemWithTime {
    ShopItemBase ShopItem{};
    std::uint32_t LastCheckTime = 0;  // MHTimeManager tick at last CheckEndTime()
};

static_assert(sizeof(ShopItemWithTime) == SHOP_ITEM_WITH_TIME_SIZE,
              "ShopItemWithTime must be 38 bytes to match legacy CommonStruct.h (under pack(1))");

// 1:1 port of legacy MOVEDATA. The saved-move-point table is keyed by DBIdx.
// 31 bytes under pack(1). The wire field order in legacy SEND_MOVEDATA_INFO
// stores arrays of MOVEDATA which therefore occupy 31 bytes per row.
struct MoveData {
    std::uint32_t DBIdx = 0;
    std::array<char, MAX_SAVED_MOVE_NAME> Name{};
    std::uint16_t MapNum = 0;
    std::uint32_t Point = 0;       // packed wx|wz (legacy stMOVEPOINT value)
};

static_assert(sizeof(MoveData) == MOVE_DATA_SIZE,
              "MoveData must be 31 bytes to match legacy CommonStruct.h MOVEDATA (under pack(1))");

// 1:1 port of legacy [CC]Header/CommonStruct.h SEND_SHOPITEM_USEDINFO.
// Sent by CShopItemManager::SendShopItemUseInfo() in response to the
// client requesting the full using-item list (category=MP_ITEM,
// protocol=MP_ITEM_SHOPITEM_USEDINFO). The wire size depends on
// ItemCount: GetSize() returns the trimmed total bytes, just like the
// legacy SEND_SHOPITEM_USEDINFO::GetSize().
struct SendShopItemUsedInfo {
    mxh::net::MsgHeader header{};
    std::uint16_t ItemCount = 0;
    ShopItemBase Item[100] = {};
};

inline constexpr std::size_t SEND_SHOPITEM_USEDINFO_MAX_BYTES =
    sizeof(mxh::net::MsgHeader) + sizeof(std::uint16_t) + 100 * sizeof(ShopItemBase);

// GetSize(): legacy returns total minus the unused trailing ShopItemBase
// entries. We replicate the exact arithmetic so the wire byte length
// matches what the legacy server emitted.
inline std::size_t send_shopitem_usedinfo_size(std::uint16_t item_count) noexcept {
    if (item_count > 100) return 0;
    return sizeof(mxh::net::MsgHeader) + sizeof(std::uint16_t) +
           static_cast<std::size_t>(item_count) * sizeof(ShopItemBase);
}

// 1:1 port of legacy [CC]Header/CommonStruct.h SEND_SHOPITEM_INFO.
// Carries the full shop-inventory (legacy TAB_SHOPITEM_NUM=5 tabs of
// TABCELL_SHOPITEM_NUM=30 cells = SLOT_SHOPITEM_NUM=150 entries).
struct SendShopItemInfo {
    mxh::net::MsgHeader header{};
    std::uint16_t ItemCount = 0;
    ItemBase Item[150] = {};  // SLOT_SHOPITEM_NUM = TAB_SHOPITEM_NUM * TABCELL_SHOPITEM_NUM = 5 * 30
};

inline constexpr std::size_t SLOT_SHOPITEM_NUM_MODERN = 150u;
inline constexpr std::size_t SEND_SHOPITEM_INFO_MAX_BYTES =
    sizeof(mxh::net::MsgHeader) + sizeof(std::uint16_t) +
    SLOT_SHOPITEM_NUM_MODERN * sizeof(ItemBase);

inline std::size_t send_shopitem_info_size(std::uint16_t item_count) noexcept {
    if (item_count > SLOT_SHOPITEM_NUM_MODERN) return 0;
    return sizeof(mxh::net::MsgHeader) + sizeof(std::uint16_t) +
           static_cast<std::size_t>(item_count) * sizeof(ItemBase);
}

// 1:1 port of legacy [CC]Header/CommonStruct.h SEND_MOVEDATA_INFO.
// Carries the saved-move-point table (legacy MAX_MOVEDATA_PERPAGE=10
// entries per page x MAX_MOVEPOINT_PAGE=2 pages = 20 entries max).
struct SendMoveDataInfo {
    mxh::net::MsgHeader header{};
    std::uint8_t bInited = 0;
    std::uint16_t Count = 0;
    MoveData Data[20] = {};  // MAX_MOVEDATA_PERPAGE * MAX_MOVEPOINT_PAGE = 10 * 2
};

inline constexpr std::size_t MAX_MOVEDATA_PERPAGE_MODERN = 10u;
inline constexpr std::size_t MAX_MOVEPOINT_PAGE_MODERN = 2u;
inline constexpr std::size_t MOVEPOINT_TOTAL_MODERN =
    MAX_MOVEDATA_PERPAGE_MODERN * MAX_MOVEPOINT_PAGE_MODERN;
inline constexpr std::size_t SEND_MOVEDATA_INFO_MAX_BYTES =
    sizeof(mxh::net::MsgHeader) + sizeof(std::uint8_t) + sizeof(std::uint16_t) +
    MOVEPOINT_TOTAL_MODERN * sizeof(MoveData);

inline std::size_t send_movedata_info_size(std::uint16_t count,
                                           bool b_inited) noexcept {
    if (count > MOVEPOINT_TOTAL_MODERN) return 0;
    (void)b_inited;
    std::size_t base = sizeof(mxh::net::MsgHeader) + sizeof(std::uint8_t)
                        + sizeof(std::uint16_t);
    return base + static_cast<std::size_t>(count) * sizeof(MoveData);
}

#pragma pack(pop)

} // namespace mxh::game
