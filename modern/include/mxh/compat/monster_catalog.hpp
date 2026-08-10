#pragma once

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
