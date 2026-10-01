#pragma once
#include "mxh/game/hero_total_layout.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>

namespace mxh::unity {
// Wire layout from the reference CommonStruct/CommonGameDefine. Decode explicitly;
// the former 124-byte modern layout must never be accepted as a wire block.
struct ShopAppearanceWire {
    static constexpr std::size_t size = 120;
    static constexpr std::size_t skin_offset = 106;
    std::array<std::uint8_t,size> raw{};
    std::array<std::uint16_t,23> avatar{};
    std::array<std::uint16_t,5> skin{};
    std::uint32_t street_stall_decoration{};
};
static_assert(mxh::game::HERO_TOTAL_MUGONG_OFFSET - mxh::game::HERO_TOTAL_SHOP_OPTION_OFFSET == ShopAppearanceWire::size);

inline std::optional<ShopAppearanceWire> parse_shop_appearance_wire(std::span<const std::uint8_t> bytes) {
    if(bytes.size()!=ShopAppearanceWire::size)return std::nullopt;
    ShopAppearanceWire out;
    std::copy(bytes.begin(),bytes.end(),out.raw.begin());
    const auto u16=[&](std::size_t offset) {return static_cast<std::uint16_t>(bytes[offset] | (static_cast<std::uint16_t>(bytes[offset+1])<<8));};
    for(std::size_t i=0;i<out.avatar.size();++i)out.avatar[i]=u16(i*2);
    for(std::size_t i=0;i<out.skin.size();++i)out.skin[i]=u16(ShopAppearanceWire::skin_offset+i*2);
    for(std::size_t i=0;i<4;++i)out.street_stall_decoration |= static_cast<std::uint32_t>(bytes[116+i])<<(8*i);
    return out;
}
}
