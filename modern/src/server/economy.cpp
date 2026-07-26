// economy.cpp - Phase D6 Economy 1:1 port implementations.

#include "mxh/server/economy.hpp"

#include <cstring>

namespace mxh::server {

namespace {
struct RegistBaseEconomyBlob {
    std::uint32_t origin_num  = 0;
    std::uint32_t origin_price = 0;
};
}  // namespace

void economy_init(EconomyState& s) {
    if (s.initialized) return;
    s.m_RegEconomy     = new RegistBaseEconomyBlob{};
    s.m_SpacialItemBase = nullptr;
    s.initialized = true;
}

void economy_release(EconomyState& s) {
    if (!s.initialized) return;
    delete static_cast<RegistBaseEconomyBlob*>(s.m_RegEconomy);
    s.m_RegEconomy = nullptr;
    delete static_cast<unsigned char*>(s.m_SpacialItemBase);
    s.m_SpacialItemBase = nullptr;
    s.initialized = false;
}

bool set_regist_economy(EconomyState& s, const void* reg, std::size_t reg_size) {
    if (!s.initialized) return false;
    if (reg == nullptr || reg_size != sizeof(RegistBaseEconomyBlob)) return false;
    std::memcpy(s.m_RegEconomy, reg, reg_size);
    return true;
}

const void* get_regist_economy(const EconomyState& s) {
    return s.m_RegEconomy;
}

bool set_base_value(EconomyState& s, const void* list_value, std::size_t item_size) {
    if (!s.initialized) return false;
    if (list_value == nullptr) return false;
    if (s.m_SpacialItemBase == nullptr) {
        s.m_SpacialItemBase = new unsigned char[item_size];
    }
    std::memcpy(s.m_SpacialItemBase, list_value, item_size);
    return true;
}

CalculBaseResult calcul_base(const EconomyState& s, std::uint32_t origin_num) {
    CalculBaseResult out{};
    if (!s.initialized || s.m_RegEconomy == nullptr) return out;
    const auto* reg = static_cast<const RegistBaseEconomyBlob*>(s.m_RegEconomy);
    if (reg->origin_num != origin_num) return out;
    out.base_price = reg->origin_price;
    out.buy_price  = reg->origin_price;
    return out;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int economy_translation_unit_anchor = 0;
}
