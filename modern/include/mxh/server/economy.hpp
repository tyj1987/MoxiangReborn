// economy.hpp - Phase D6 Economy 1:1 port.
//
// Source-of-truth: legacy [Server]Map/Economy.h + .cpp.
// Mirrors legacy CEconomy singleton's two-pointer state and the
// memcpy-driven SetRegistEconomy / SetBaseValue / CalculBase entry
// points.  The structs themselves (REGIST_BASEECONOMY, ITEM_INFO)
// are owned by their respective modules; we expose them as opaque
// byte blobs here and rely on the caller to size-check.

#pragma once

#include <cstddef>
#include <cstdint>

namespace mxh::server {

#pragma pack(push, 1)
struct RegistBaseEconomy {
    std::uint16_t MapNum = 0;
    std::uint16_t OriginNum = 0;
    std::uint16_t OriginPrice = 0;
    std::uint16_t OriginAmount = 0;
    std::uint16_t RequireNum = 0;
    std::uint16_t RequirePrice = 0;
    std::uint16_t RequireAmount = 0;
    std::uint8_t BuyRates = 0;
    std::uint8_t SellRates = 0;
};
#pragma pack(pop)

static_assert(sizeof(RegistBaseEconomy) == 16);

// ---- Economy state (mirror CEconomy) ----
struct EconomyState {
    void* m_RegEconomy     = nullptr;  // owned pointer
    void* m_SpacialItemBase = nullptr;  // owned pointer
    std::size_t m_SpacialItemBaseSize = 0;
    bool  initialized     = false;
};

void economy_init(EconomyState& s);
void economy_release(EconomyState& s);

// SetRegistEconomy: legacy memcpy(m_RegEconomy, RegEconomy, sizeof(REGIST_BASEECONOMY)).
bool set_regist_economy(EconomyState& s, const void* reg, std::size_t reg_size);
const void* get_regist_economy(const EconomyState& s);

// SetBaseValue: legacy memcpy(m_SpacialItemBase, ListValue, sizeof(ITEM_INFO)).
bool set_base_value(EconomyState& s, const void* list_value, std::size_t item_size);

// CalculBase is an empty legacy stub; the return value remains zeroed.
struct CalculBaseResult {
    std::uint32_t base_price = 0;
    std::uint32_t buy_price  = 0;
};
CalculBaseResult calcul_base(const EconomyState& s, std::uint32_t origin_num);

}  // namespace mxh::server

#include <cstddef>
