#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <span>
#include <vector>
#include "mxh/compat/detail/text_parse.hpp"

namespace mxh::game {
struct ExpPenalty {
    float present_percent = 0;
    float login_percent = 0;
};
using ExpPenaltyTable = std::map<std::uint16_t, ExpPenalty>;
enum class ReviveLocation { Present, Login, Village };
struct ReviveLoss { std::uint32_t money; std::uint64_t experience; };
struct ReviveProtectionPlan {
    ReviveLoss loss{};
    bool consume_combined = false, end_combined = false;
    bool consume_money = false, consume_experience = false;
    std::int8_t remaining_combined = 0;
};

// CPlayer's ProtectAll_UseFail fallback: an orphan count does not provide
// protection. Presence flags refer to validated live using-item records.
// Only call for a branch that actually applies a revival penalty.
inline std::optional<ReviveProtectionPlan> plan_revive_protection(
    ReviveLoss loss, std::int8_t combined_count, bool combined_record_valid,
    bool money_record_present, bool experience_record_present) {
    if (combined_count < 0) return std::nullopt;
    ReviveProtectionPlan plan;
    plan.loss = loss;
    plan.remaining_combined = combined_count;
    if (combined_count > 0 && combined_record_valid) {
        plan.consume_combined = true;
        plan.remaining_combined = static_cast<std::int8_t>(combined_count - 1);
        plan.end_combined = plan.remaining_combined == 0;
        plan.loss = {};
    } else {
        plan.consume_money = money_record_present;
        plan.consume_experience = experience_record_present;
        if (money_record_present) plan.loss.money = 0;
        if (experience_record_present) plan.loss.experience = 0;
    }
    return plan;
}

// Computes the ordinary unprotected branch only. The server must first decide
// exemptions and consume protection items; this function does not authorize revival.
inline std::optional<ReviveLoss> unprotected_revive_loss(
    const ExpPenaltyTable& table, ReviveLocation location, std::uint16_t level,
    std::uint32_t money, std::uint64_t level_threshold, bool siege_login = false) {
    if (level == 0 || level_threshold > 0x7fffffffffffffffULL) return std::nullopt;
    if (location != ReviveLocation::Present && location != ReviveLocation::Login &&
        location != ReviveLocation::Village) return std::nullopt;
    if (siege_login && location != ReviveLocation::Login) return std::nullopt;
    const auto found = table.find(level);
    const float percent = location == ReviveLocation::Present
        ? (found == table.end() ? 3.0f : found->second.present_percent)
        : (found == table.end() ? 2.0f : found->second.login_percent);
    if (!std::isfinite(percent) || percent < 0 || percent > 100) return std::nullopt;
    // Original village branch uses double 0.01; present/login use float fRate.
    std::uint64_t experience = 0;
    if (location == ReviveLocation::Village) {
        experience = static_cast<std::uint64_t>(level_threshold * 0.01);
    } else {
        const float rate = siege_login ? 0.01f : percent * 0.01f;
        const float loss = static_cast<float>(level_threshold) * rate;
        if (!std::isfinite(loss) || loss >= 0x1p63f) return std::nullopt;
        experience = static_cast<std::uint64_t>(loss);
    }
    return ReviveLoss{static_cast<std::uint32_t>(money *
        (location == ReviveLocation::Present ? 0.06 : 0.04)), experience};
}

// Input must already be decoded using the explicitly selected resource profile.
// Missing levels remain absent: the caller applies the original mode-specific fallback.
inline std::optional<ExpPenaltyTable> parse_exp_penalty_text(std::string_view text) {
    std::istringstream input{std::string(text)};
    input.imbue(std::locale::classic());
    ExpPenaltyTable table;
    std::string level_token, present_token, login_token;
    auto number = [](const std::string& token, auto& value) {
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
        return result.ec == std::errc{} && result.ptr == token.data() + token.size();
    };
    while (input >> level_token) {
        if (!(input >> present_token >> login_token)) return std::nullopt;
        unsigned level = 0;
        ExpPenalty penalty;
        if (!number(level_token, level) || level == 0 || level > 65535 ||
            !number(present_token, penalty.present_percent) || !number(login_token, penalty.login_percent) ||
            !std::isfinite(penalty.present_percent) || !std::isfinite(penalty.login_percent) ||
            penalty.present_percent < 0 || penalty.present_percent > 100 ||
            penalty.login_percent < 0 || penalty.login_percent > 100 ||
            !table.emplace(static_cast<std::uint16_t>(level), penalty).second)
            return std::nullopt;
    }
    if (table.empty()) return std::nullopt;
    return table;
}
enum class ExpPenaltyProfile { PlayDhCurrent, LegacyMhFile };

// Container integrity here is structural/legacy CRC only. Published resources
// must additionally be checked against their source manifest digest.
inline std::optional<ExpPenaltyTable> decode_exp_penalty(
    std::span<const std::uint8_t> raw, ExpPenaltyProfile profile) {
    if (raw.size() < 14 || raw.size() > 65536) return std::nullopt;
    auto u32 = [&raw](std::size_t at) {
        return std::uint32_t(raw[at]) | (std::uint32_t(raw[at+1]) << 8) |
            (std::uint32_t(raw[at+2]) << 16) | (std::uint32_t(raw[at+3]) << 24);
    };
    std::vector<std::uint8_t> decoded;
    if (profile == ExpPenaltyProfile::PlayDhCurrent) {
        if (raw.size() < 25 || u32(0) != raw.size()) return std::nullopt;
        // Recovered from current resource; plaintext independently matches the
        // original MHFile with both CRCs. Do not infer keys from arbitrary input.
        constexpr std::uint8_t key[]{201,185,170,137,139,153,100,104};
        decoded.assign(raw.begin()+24, raw.end());
        for (std::size_t i = 0; i < decoded.size(); ++i) decoded[i] ^= key[i%8];
    } else if (profile == ExpPenaltyProfile::LegacyMhFile) {
        if (u32(4) == 0 || u32(8) != raw.size()-14) return std::nullopt;
        decoded.assign(raw.begin()+13, raw.end()-1);
        const auto crc = mxh::compat::detail::decode_mhfile_text_payload(u32(4), decoded);
        if (crc != raw[12] || crc != raw.back()) return std::nullopt;
    } else return std::nullopt;
    return parse_exp_penalty_text(std::string_view(
        reinterpret_cast<const char*>(decoded.data()), decoded.size()));
}
} // namespace mxh::game
