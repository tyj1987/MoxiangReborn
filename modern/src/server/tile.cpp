// tile.cpp - Phase D6 Tile 1:1 port.

#include "mxh/server/tile.hpp"

namespace mxh::server {

void tile_init(Tile& t) {
    t.m_Attr.uCharPreoccupied = 0;
    t.m_Attr.uMuCollisonNum   = 0;
}

void tile_increase_preoccupied(Tile& t) {
    if (t.m_Attr.uCharPreoccupied < 255) ++t.m_Attr.uCharPreoccupied;
}

void tile_decrease_preoccupied(Tile& t) {
    if (t.m_Attr.uCharPreoccupied > 0) --t.m_Attr.uCharPreoccupied;
}

std::uint8_t tile_get_preoccupied(const Tile& t) {
    return t.m_Attr.uCharPreoccupied;
}

void tile_inc_mu_collision(Tile& t) {
    if (t.m_Attr.uMuCollisonNum < 255) ++t.m_Attr.uMuCollisonNum;
}

void tile_dec_mu_collision(Tile& t) {
    if (t.m_Attr.uMuCollisonNum > 0) --t.m_Attr.uMuCollisonNum;
}

std::uint8_t tile_get_mu_collision(const Tile& t) {
    return t.m_Attr.uMuCollisonNum;
}

bool tile_is_collision(const Tile& t) {
    return t.m_Attr.uCharPreoccupied != 0 || t.m_Attr.uMuCollisonNum != 0;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int tile_translation_unit_anchor = 0;
}
