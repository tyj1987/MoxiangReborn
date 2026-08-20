#pragma once

#include <cstdint>
#include <optional>

namespace mxh::client {

struct LogicalPoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct ContentRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

class LogicalViewport {
public:
    static constexpr std::int32_t kLogicalWidth = 800;
    static constexpr std::int32_t kLogicalHeight = 600;

    void update(std::int32_t physical_width, std::int32_t physical_height) noexcept;
    [[nodiscard]] const ContentRect& content_rect() const noexcept { return m_content; }
    [[nodiscard]] float scale() const noexcept { return m_scale; }
    [[nodiscard]] std::optional<LogicalPoint> to_logical(
        std::int32_t physical_x, std::int32_t physical_y) const noexcept;

private:
    ContentRect m_content{0, 0, kLogicalWidth, kLogicalHeight};
    float m_scale = 1.0f;
};

}  // namespace mxh::client
