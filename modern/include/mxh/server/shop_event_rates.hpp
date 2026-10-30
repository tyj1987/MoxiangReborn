#pragma once
#include <mxh/server/calc_shop_item_option.hpp>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::server {
// Value snapshot: one admission uses one consistent baseline/current pair.
// Rate indices match source CommonStruct.h eCheatEvent (0..11).
class ShopRateEnvironment final : public CalcShopItemOptionEnv {
public:
    using Rates = std::array<float,12>;
    ShopRateEnvironment(ShopLocale locale, const Rates& baseline)
        : m_locale(locale), m_baseline(baseline), m_current(baseline) {}
    ShopLocale locale() const noexcept override { return m_locale; }
    bool event_rate_active(std::uint16_t id) const noexcept override {
        return id < m_current.size() && std::isfinite(m_current[id]) &&
            std::isfinite(m_baseline[id]) && m_current[id] == m_baseline[id];
    }
    const Rates& baseline() const noexcept { return m_baseline; }
    const Rates& current() const noexcept { return m_current; }
    std::optional<ShopRateEnvironment> with_current(const Rates& current) const {
        for (float value : current) if (!std::isfinite(value)) return std::nullopt;
        auto result = *this; result.m_current = current; return result;
    }
private:
    ShopLocale m_locale;
    Rates m_baseline, m_current;
};

inline std::optional<ShopRateEnvironment> parse_shop_rate_text(
    std::string_view text, ShopLocale locale, std::string& error) {
    static constexpr std::array<std::string_view,12> labels = {"", "#EXP", "#ITEM", "#MONEY",
        "#DAMAGERECIVE", "#DAMAGERATE", "#NAERYUKSPEND", "#UNGISPEED", "#PARTYEXP",
        "#ABIL", "#GETMONEY", "#MUGONGEXPRATE"};
    ShopRateEnvironment::Rates rates; rates.fill(1.0f);
    std::array<bool,12> seen{};
    const auto token = [&]() {
        auto start = text.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) { text = {}; return std::string_view{}; }
        text.remove_prefix(start);
        auto end = text.find_first_of(" \t\r\n");
        if (end == std::string_view::npos) end = text.size();
        auto result = text.substr(0,end); text.remove_prefix(end); return result;
    };
    error.clear();
    while (true) {
        auto label = token(); if (label.empty()) break;
        std::size_t id = 1;
        while (id < labels.size() && labels[id] != label) ++id;
        if (id == labels.size() || seen[id]) { error="unknown or duplicate event-rate label"; return std::nullopt; }
        auto value = token(); float rate = 0;
        if (value.empty()) { error="missing event-rate value"; return std::nullopt; }
        auto parsed = std::from_chars(value.data(),value.data()+value.size(),rate);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size() || !std::isfinite(rate)) {
            error="invalid event-rate number"; return std::nullopt;
        }
        rates[id]=rate; seen[id]=true;
    }
    // The audited playdh-current resource contains all eleven named rates.
    for (std::size_t id=1; id<seen.size(); ++id)
        if (!seen[id]) { error="incomplete playdh-current rate table"; return std::nullopt; }
    return ShopRateEnvironment(locale,rates);
}

inline std::optional<ShopRateEnvironment> parse_playdh_shop_rates(
    std::span<const std::uint8_t> raw, ShopLocale locale, std::string& error) {
    if (raw.size()<24 || raw.size()>16384) { error="invalid rate container size"; return std::nullopt; }
    std::uint32_t size=0;
    for (unsigned i=0; i<4; ++i) size |= std::uint32_t(raw[i]) << (8*i);
    if (size!=raw.size() || raw[12]!=0xcc || raw[13]!=0xcd || raw[14]!=0xce || raw[15]!=0xcf) {
        error="unsupported rate container"; return std::nullopt;
    }
    // Recovered specifically from the audited DropRate.bin: 4-byte size,
    // 20-byte opaque header, repeating XOR. All 11 source labels and numeric
    // tokens validate; this is not the AIGroup decoder or a cross-file fallback.
    constexpr std::array<std::uint8_t,8> key{0x24,0x60,0x05,0x70,0x66,0x40,0x47,0x50};
    std::string decoded(raw.size()-24,'\0');
    for (std::size_t i=0; i<decoded.size(); ++i) decoded[i]=static_cast<char>(raw[i+24]^key[i%8]);
    return parse_shop_rate_text(decoded,locale,error);
}

inline std::optional<ShopRateEnvironment> load_playdh_shop_rates(
    const std::filesystem::path& path, ShopLocale locale, std::string& error) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file) { error="cannot open DropRate.bin"; return std::nullopt; }
    const auto size=file.tellg();
    if (size<24 || size>16384) { error="invalid rate file length"; return std::nullopt; }
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(size)); file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(raw.data()),static_cast<std::streamsize>(raw.size()))) {
        error="cannot read DropRate.bin"; return std::nullopt;
    }
    return parse_playdh_shop_rates(raw,locale,error);
}
} // namespace mxh::server
