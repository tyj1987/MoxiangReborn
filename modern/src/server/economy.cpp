// economy.cpp - Phase D6 Economy 1:1 port implementations.

#include "mxh/server/economy.hpp"

#include <cstring>

namespace mxh::server {

void economy_init(EconomyState& s) {
    if (s.initialized) return;
    s.m_RegEconomy     = new RegistBaseEconomy{};
    s.m_SpacialItemBase = nullptr;
    s.initialized = true;
}

void economy_release(EconomyState& s) {
    if (!s.initialized) return;
    delete static_cast<RegistBaseEconomy*>(s.m_RegEconomy);
    s.m_RegEconomy = nullptr;
    delete[] static_cast<unsigned char*>(s.m_SpacialItemBase);
    s.m_SpacialItemBase = nullptr;
    s.m_SpacialItemBaseSize = 0;
    s.initialized = false;
}

bool set_regist_economy(EconomyState& s, const void* reg, std::size_t reg_size) {
    if (!s.initialized) return false;
    if (reg == nullptr || reg_size != sizeof(RegistBaseEconomy)) return false;
    std::memcpy(s.m_RegEconomy, reg, reg_size);
    return true;
}

const void* get_regist_economy(const EconomyState& s) {
    return s.m_RegEconomy;
}

bool set_base_value(EconomyState& s, const void* list_value, std::size_t item_size) {
    if (!s.initialized) return false;
    if (list_value == nullptr || item_size == 0) return false;
    if (s.m_SpacialItemBaseSize != item_size) {
        delete[] static_cast<unsigned char*>(s.m_SpacialItemBase);
        s.m_SpacialItemBase = new unsigned char[item_size];
        s.m_SpacialItemBaseSize = item_size;
    }
    std::memcpy(s.m_SpacialItemBase, list_value, item_size);
    return true;
}

CalculBaseResult calcul_base(const EconomyState&, std::uint32_t) {
    return {};
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int economy_translation_unit_anchor = 0;
}
