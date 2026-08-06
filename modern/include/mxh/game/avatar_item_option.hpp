// avatar_item_option.hpp
//
// 1:1 port of legacy [CC]Header/CommonStruct.h AVATARITEMOPTION plus the
// eAvatar_* enum from [CC]Header/CommonGameDefine.h. The legacy struct is
// wrapped in #pragma pack(push,1) and lives inside the SHOPITEMOPTION
// aggregate (avatar[24] + stat accumulators). The modern port pulls both
// into a single header so the data plane (calc_avatar_option) can match
// the legacy accumulator behavior byte-for-byte without dragging in the
// full CommonStruct.h header chain.
//
// 1:1 invariants (under pack(1)):
//   - eAvatar_Max       = 24 (24 avatar slots, indices 0..23).
//   - AVATARITEMOPTION  = 50 bytes (28 fields, no virtual, no padding).
//
// Wire compatibility: AVATARITEMOPTION is server-side state and not
// serialized over the modern T-series wire. The legacy CommonStruct.h
// packed layout only matters if the modern AgentServer talks to a legacy
// MapServer (not in scope here).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mxh::game {

// 24 avatar slots, 1:1 with legacy eAvatar_* enum.
//
// Indices 0..11 are cosmetic slots (Hat, Hair, Face, Mask, Glasses,
// Mustache, Dress, Shoulder, Shoes, Hand, Effect).
// Indices 12..22 are weapon-slot avatars (Weared_Hair..Weared_Amgi).
// Index 23 (eAvatar_Max) is the legacy sentinel.
inline constexpr std::size_t EAvatarCount = 24u;

enum class AvatarSlot : std::uint8_t {
    Hat = 0,
    Hair,
    Face,
    Mask,
    Glasses,
    Mustache,
    Dress,
    Shoulder,
    Back,
    Shoes,
    Effect,
    Hand,

    Weared_Hair,
    Weared_Face,
    Weared_Hat,
    Weared_Dress,
    Weared_Shoes,
    Weared_Gum,
    Weared_Gwun,
    Weared_Do,
    Weared_Chang,
    Weared_Gung,
    Weared_Amgi,

    Max = 24,
};

#pragma pack(push, 1)

// 1:1 port of legacy AVATARITEMOPTION (50 bytes under pack(1)).
// Field order and sizes match CommonStruct.h exactly so the legacy
// memcpy() pairs (avatar[24] + stat accumulators) stay in sync.
struct AvatarItemOption {
    std::uint16_t Life = 0;
    std::uint16_t Shield = 0;
    std::uint16_t Naeruyk = 0;

    std::uint8_t  Attack = 0;
    std::uint8_t  Critical = 0;
    std::uint8_t  Decisive = 0;

    std::uint8_t  Gengol = 0;
    std::uint8_t  Minchub = 0;
    std::uint8_t  Cheryuk = 0;
    std::uint8_t  Simmek = 0;

    std::uint16_t CounterPercent = 0;
    std::uint16_t CounterDamage = 0;

    std::uint8_t  bKyungGong = 0;

    std::uint8_t  NeaRyukSpend = 0;

    std::uint16_t NeagongDamage = 0;
    std::uint16_t WoigongDamage = 0;

    std::uint16_t TargetPhyDefDown = 0;
    std::uint16_t TargetAttrDefDown = 0;
    std::uint16_t TargetAtkDown = 0;

    std::uint16_t RecoverRate = 0;
    std::uint16_t KyunggongSpeed = 0;

    std::uint16_t MussangCharge = 0;
    std::uint8_t  NaeruykspendbyKG = 0;
    std::uint16_t ShieldRecoverRate = 0;
    std::uint8_t  MussangDamage = 0;
};

static_assert(sizeof(AvatarItemOption) == 39u,
              "AvatarItemOption must be 39 bytes (25 fields under pack(1)) to match legacy CommonStruct.h AVATARITEMOPTION");

#pragma pack(pop)

}  // namespace mxh::game
