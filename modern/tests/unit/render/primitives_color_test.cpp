#include "primitives.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace mxh::gx::dx11::test {

TEST(PrimitiveTexturedColor, CanonicalLightBlueProducesRgbaVertexBytes) {
    // PlayDH 17.bin TEXTCOLOR 208 234 245 becomes legacy ARGB 0xFFD0EAF5.
    const std::uint32_t vertex = pack_argb_for_rgba_vertex(0xFFD0EAF5u);
    std::array<std::uint8_t, 4> bytes{};
    std::memcpy(bytes.data(), &vertex, bytes.size());
    EXPECT_EQ(bytes, (std::array<std::uint8_t, 4>{208, 234, 245, 255}));
}

TEST(PrimitiveTexturedColor, AsymmetricRgbAndAlphaKeepChannelOrder) {
    const std::uint32_t vertex = pack_argb_for_rgba_vertex(0x7F12A3E5u);
    std::array<std::uint8_t, 4> bytes{};
    std::memcpy(bytes.data(), &vertex, bytes.size());
    EXPECT_EQ(bytes, (std::array<std::uint8_t, 4>{0x12, 0xA3, 0xE5, 0x7F}));
    EXPECT_EQ(pack_argb_for_rgba_vertex(0xFFFFFFFFu), 0xFFFFFFFFu);
}

} // namespace mxh::gx::dx11::test
