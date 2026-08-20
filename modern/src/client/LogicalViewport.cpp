#include "LogicalViewport.hpp"

#include <algorithm>
#include <cmath>

namespace mxh::client {

void LogicalViewport::update(
    std::int32_t physical_width, std::int32_t physical_height) noexcept {
    if (physical_width <= 0 || physical_height <= 0) {
        m_content = {};
        m_scale = 0.0f;
        return;
    }

    const float sx = static_cast<float>(physical_width) /
                     static_cast<float>(kLogicalWidth);
    const float sy = static_cast<float>(physical_height) /
                     static_cast<float>(kLogicalHeight);
    m_scale = std::min(sx, sy);
    m_content.width = static_cast<std::int32_t>(
        std::lround(static_cast<float>(kLogicalWidth) * m_scale));
    m_content.height = static_cast<std::int32_t>(
        std::lround(static_cast<float>(kLogicalHeight) * m_scale));
    m_content.width = std::min(m_content.width, physical_width);
    m_content.height = std::min(m_content.height, physical_height);
    m_content.x = (physical_width - m_content.width) / 2;
    m_content.y = (physical_height - m_content.height) / 2;
}

std::optional<LogicalPoint> LogicalViewport::to_logical(
    std::int32_t physical_x, std::int32_t physical_y) const noexcept {
    if (m_scale <= 0.0f || m_content.width <= 0 || m_content.height <= 0) {
        return std::nullopt;
    }
    if (physical_x < m_content.x || physical_y < m_content.y ||
        physical_x >= m_content.x + m_content.width ||
        physical_y >= m_content.y + m_content.height) {
        return std::nullopt;
    }
    return LogicalPoint{
        static_cast<float>(physical_x - m_content.x) / m_scale,
        static_cast<float>(physical_y - m_content.y) / m_scale,
    };
}

}  // namespace mxh::client
