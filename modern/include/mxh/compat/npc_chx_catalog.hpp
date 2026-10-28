#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

// Legacy Resource/Client/NpcChxList.bin index table. Slot zero is the
// original null.chx sentinel and is deliberately not exposed by find().
class NpcChxCatalog {
public:
    [[nodiscard]] static std::optional<NpcChxCatalog> parse_text(
        std::span<const std::uint8_t> payload);
    [[nodiscard]] static std::optional<NpcChxCatalog> parse_bin(
        std::span<const std::uint8_t> file_bytes);
    [[nodiscard]] const std::string* find(std::uint16_t index) const noexcept;
    [[nodiscard]] const std::vector<std::string>& entries() const noexcept {
        return entries_;
    }

private:
    std::vector<std::string> entries_;
};

} // namespace mxh::compat
