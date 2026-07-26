// fixed_tile.cpp - Phase D6 FixedTile 1:1 port.

#include "mxh/server/fixed_tile.hpp"

namespace mxh::server {

void fixed_tile_init(FixedTile& t, std::uint8_t attr) {
    t.m_FixedAttr.uFixedAttr = attr;
}

bool fixed_tile_is_collision(const FixedTile& t) {
    return (t.m_FixedAttr.uFixedAttr & TFA_COLLISON) != 0;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int fixed_tile_translation_unit_anchor = 0;
}
