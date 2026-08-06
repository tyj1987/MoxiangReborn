// shop_item_option.hpp
//
// 1:1 port of legacy [CC]Header/CommonStruct.h SHOPITEMOPTION plus the
// eSkinItem_* enum from [CC]Header/CommonGameDefine.h. The legacy struct
// is wrapped in #pragma pack(push,1) and is the server-side aggregate
// that holds every stat bonus a player's currently-equipped shop items
// (charms, herbs, incantations, etc.) have contributed. The personal
// avatar stats are tracked in the Avatar[24] WORD array (consumed by
// calc_avatar_option / AVATARITEMOPTION, see avatar_item_option.hpp).
//
// 1:1 invariants (under pack(1)):
//   - Avatar[24]      = 48 bytes (24 * WORD), indices 0..23 = eAvatar_Hat..WeeMaxAmgi.
//   - wSkinItem[6]    = 12 bytes (6 * WORD), indices 0..5 = eSkinItem_Hat..Shoes.
//   - All other fields are 1/2/4-byte primitives laid out in legacy order.
//   - Total size = 124 bytes (validated by the test suite).
//
// Wire compatibility: SHOPITEMOPTION is server-side state and not
// serialized over the modern T-series wire. The legacy CommonStruct.h
// packed layout only matters if the modern AgentServer talks to a legacy
// MapServer (not in scope here).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mxh::game {

// 1:1 with legacy eSkinItem_* enum (eSkinItem_Hat..eSkinItem_Shoes).
// 6 cosmetic skin slots. The eSkinItem_Max sentinel is the legacy
// array bound and is kept as a named constant so the struct layout
// can be asserted in tests.
inline constexpr std::size_t ESkinItemCount = 6u;

#pragma pack(push, 1)

// 1:1 port of legacy SHOPITEMOPTION (124 bytes under pack(1)).
// Field order and sizes match CommonStruct.h exactly so the legacy
// memcpy / aggregation semantics stay in sync.
struct ShopItemOption {
    std::array<std::uint16_t, 24> Avatar{};

    // 4 base stats (legacy Gengol/Minchub/Cheryuk/Simmek).
    std::uint16_t Gengol  = 0;
    std::uint16_t Minchub = 0;
    std::uint16_t Cheryuk = 0;
    std::uint16_t Simmek  = 0;

    // 3 resource caps (legacy Life/Shield/Naeryuk).
    std::uint16_t Life    = 0;
    std::uint16_t Shield  = 0;
    std::uint16_t Naeryuk = 0;

    // 2 percentage accumulators (legacy AddExp/AddItemDrop).
    std::uint16_t AddExp      = 0;
    std::uint16_t AddItemDrop = 0;

    // 2 penalty counters (legacy ExpPeneltyPoint/MoneyPeneltyPoint).
    std::int8_t   ExpPeneltyPoint  = 0;
    std::int8_t   MoneyPeneltyPoint = 0;

    // 1 special stat + 3 damage types (legacy AddSung / Neagong /
    // Woigong / Combo).
    std::uint16_t AddSung        = 0;
    std::int8_t   NeagongDamage  = 0;
    std::int8_t   WoigongDamage  = 0;
    std::int8_t   ComboDamage    = 0;
    std::int8_t   RecoverRate    = 0;

    // 1 crit accumulator + 1 crit sub-stat + 1 decisive.
    std::uint16_t Critical   = 0;
    std::int8_t   StunByCri  = 0;
    std::uint16_t Decisive   = 0;

    // 1 mix success + 2 state-point accumulators.
    std::int8_t   ItemMixSuccess = 0;
    std::uint16_t StatePoint     = 0;
    std::uint16_t UseStatePoint  = 0;

    // 3 elemental/resist fields.
    std::int8_t   RegistPhys  = 0;
    std::int8_t   RegistAttr  = 0;
    std::int8_t   NeaRyukSpend = 0;

    // 2 skill-point accumulators (legacy SkillPoint/UseSkillPoint DWORD).
    std::uint32_t SkillPoint    = 0;
    std::uint32_t UseSkillPoint = 0;

    // 1 protect-count char (legacy ProtectCount).
    std::int8_t   ProtectCount = 0;

    // 2 ability/mugong-exp accumulators.
    std::uint16_t AddAbility   = 0;
    std::uint16_t AddMugongExp = 0;

    // 3 plustime fields.
    std::int8_t   PlustimeExp     = 0;
    std::int8_t   PlustimeAbil    = 0;
    std::int8_t   PlustimeNaeruyk = 0;

    // 2 kyung-gong fields (legacy BYTE).
    std::uint8_t  bKyungGong     = 0;
    std::uint8_t  KyungGongSpeed = 0;

    // 3 misc accumulators (legacy BYTE).
    std::uint8_t  EquipLevelFree = 0;
    std::uint8_t  ReinforceAmp   = 0;
    std::uint8_t  bStreetStall   = 0;

    // Skin-item accumulator (legacy WORD wSkinItem[eSkinItem_Max]).
    std::array<std::uint16_t, ESkinItemCount> wSkinItem{};

    // Street-stall decoration flag (legacy dwStreetStallDecoration DWORD).
    std::uint32_t dwStreetStallDecoration = 0;
};

#pragma pack(pop)

// Compile-time size assertion. The legacy struct is 124 bytes under pack(1);
// the modern port deviates by 0 bytes.
static_assert(sizeof(ShopItemOption) == 124,
              "ShopItemOption must be 124 bytes (1:1 with legacy "
              "CommonStruct.h SHOPITEMOPTION under pack(1)).");

}  // namespace mxh::game
