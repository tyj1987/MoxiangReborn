#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

struct MonsterVisual {
    std::uint16_t kind = 0;
    std::string name;
    std::string chx_name;
    float scale = 1.0f;
    std::uint8_t level = 1;
    std::uint32_t life = 0;
    std::uint32_t shield = 0;
    std::uint32_t exp = 0;
    std::uint16_t attack_min = 0;
    std::uint16_t attack_max = 0;
    std::uint16_t defense = 0;
    float walk_speed = 0.0f;
    float run_speed = 0.0f;
    float domain_range = 0.0f;
    bool aggressive = false;
    float search_period_ms = 0.0f;
    float search_range = 0.0f;
    std::uint8_t attack_count = 0;
    std::array<std::uint32_t, 2> attack_skills{};
    std::array<std::uint32_t, 2> attack_rates{};
};

class MonsterCatalog {
public:
    [[nodiscard]] static std::optional<MonsterCatalog> parse_text(
        std::span<const std::uint8_t> payload);
    [[nodiscard]] static std::optional<MonsterCatalog> parse_bin(
        std::span<const std::uint8_t> file_bytes);
    [[nodiscard]] const MonsterVisual* find(std::uint16_t kind) const noexcept;
    [[nodiscard]] const std::vector<MonsterVisual>& entries() const noexcept { return entries_; }

private:
    std::vector<MonsterVisual> entries_;
};
} // namespace mxh::compat
