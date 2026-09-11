#include "mxh/game/movement_timeline.hpp"
#include <algorithm>
#include <cmath>

namespace mxh::game {
bool MovementTimeline::valid(MovementPoint p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.z) && p.x >= 0 && p.z >= 0 && p.x < 51200 && p.z < 51200;
}
bool MovementTimeline::reset(MovementPoint p, std::uint64_t now) noexcept {
    if (!valid(p)) return false;
    m_position = m_start = m_target = p;
    m_velocity = {}; m_start_ms = m_last_ms = now;
    m_duration_seconds = m_speed = 0;
    m_moving = false; m_initialized = true;
    return true;
}
bool MovementTimeline::start(MovementPoint target, float speed, std::uint64_t now) noexcept {
    if (!m_initialized || !valid(target) || !std::isfinite(speed) || speed < 0) return false;
    advance(now);
    if (speed == 0) { halt(now); return true; }
    const float dx = target.x - m_position.x, dz = target.z - m_position.z;
    const float distance = std::sqrt(dx*dx + dz*dz);
    const MovementPoint velocity = distance ? MovementPoint{dx*(speed/distance), dz*(speed/distance)} : MovementPoint{};
    const float duration = distance / speed;
    if (!std::isfinite(velocity.x) || !std::isfinite(velocity.z) || !std::isfinite(duration)) return false;
    m_start = m_position; m_target = target; m_speed = speed; m_start_ms = m_last_ms;
    m_velocity = velocity; m_duration_seconds = duration;
    m_moving = true;
    return true;
}
MovementPoint MovementTimeline::advance(std::uint64_t now) noexcept {
    if (!m_initialized) return m_position;
    // Backwards clock input must never rewind authority; uint64 avoids DWORD wrap.
    now = std::max(now, m_last_ms);
    if (m_moving && now != m_last_ms) {
        const float elapsed = static_cast<float>(now - m_start_ms) * 0.001f;
        if (m_duration_seconds < elapsed) {
            m_position = m_target; m_moving = false;
        } else {
            m_position = {m_start.x + m_velocity.x * elapsed, m_start.z + m_velocity.z * elapsed};
        }
    }
    m_last_ms = now;
    m_position.x = std::clamp(m_position.x, 0.0f, 51100.0f);
    m_position.z = std::clamp(m_position.z, 0.0f, 51100.0f);
    return m_position;
}
void MovementTimeline::halt(std::uint64_t now) noexcept {
    advance(now); m_moving = false; m_target = m_position;
    m_velocity = {}; m_speed = 0; m_duration_seconds = 0;
}
MovementStopResult MovementTimeline::stop(MovementPoint claimed, std::uint64_t now) noexcept {
    if (!m_initialized || !valid(claimed)) return MovementStopResult::InvalidInput;
    advance(now);
    const float dx = claimed.x - m_position.x, dz = claimed.z - m_position.z;
    if (std::sqrt(dx*dx + dz*dz) > stop_tolerance) {
        halt(now); return MovementStopResult::CorrectionRequired;
    }
    reset(claimed, m_last_ms);
    return MovementStopResult::Accepted;
}
} // namespace mxh::game
