#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

struct AnmPositionKey { std::uint32_t ticks{}, frame{}; std::array<float, 3> value{}; };
struct AnmRotationKey { std::uint32_t ticks{}, frame{}; std::array<float, 4> value{}; };
struct AnmScaleKey { std::uint32_t ticks{}, frame{}; std::array<float, 3> value{}; };

struct AnmMotionObject {
    std::uint32_t index{};
    std::string name;
    std::vector<AnmPositionKey> positions;
    std::vector<AnmRotationKey> rotations;
    std::vector<AnmScaleKey> scales;

    [[nodiscard]] std::array<float, 3> samplePosition(
        float frame, const std::array<float, 3>& fallback) const noexcept;
    [[nodiscard]] std::array<float, 4> sampleRotation(
        float frame, const std::array<float, 4>& fallback) const noexcept;
    [[nodiscard]] std::array<float, 3> sampleScale(
        float frame, const std::array<float, 3>& fallback) const noexcept;
};

class AnmMotion {
public:
    std::uint32_t version{}, ticks_per_frame{}, first_frame{}, last_frame{};
    std::uint32_t frame_speed{}, key_frame_step{};
    std::vector<AnmMotionObject> objects;

    [[nodiscard]] static std::optional<AnmMotion> parse(
        std::span<const std::uint8_t> bytes, std::string* error = nullptr);
    [[nodiscard]] const AnmMotionObject* find(std::string_view name) const noexcept;
};

} // namespace mxh::compat
