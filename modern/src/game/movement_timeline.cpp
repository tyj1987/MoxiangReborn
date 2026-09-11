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
    clear_route();
    return true;
}
bool MovementTimeline::start(MovementPoint target, float speed, std::uint64_t now) noexcept {
    return start_route(std::span<const MovementPoint>(&target, 1),speed,now);
}
bool MovementTimeline::start_route(std::span<const MovementPoint> targets, float speed,
                                  std::uint64_t now) noexcept {
    if (!m_initialized || targets.empty() || targets.size() > max_route_points ||
        !std::isfinite(speed) || speed < 0) return false;
    for (const auto target : targets) if (!valid(target)) return false;
    // Validate the complete replacement on a copy. A bad later segment must
    // not discard a valid live route. Copy input before advancing (it may alias us).
    std::array<MovementPoint,max_route_points> route{};
    std::copy(targets.begin(),targets.end(),route.begin());
    advance(now);
    auto candidate = *this;
    if (speed == 0) { candidate.halt(now); *this = candidate; return true; }
    auto probe = candidate;
    for (std::size_t i=0; i<targets.size(); ++i) {
        if (!probe.start_segment(route[i],speed,probe.m_last_ms)) return false;
        probe.m_position = {std::clamp(route[i].x,0.0f,51100.0f),
                            std::clamp(route[i].z,0.0f,51100.0f)};
    }
    if (!candidate.start_segment(route[0],speed,candidate.m_last_ms)) return false;
    candidate.m_route = route;
    candidate.m_route_index = 0;
    candidate.m_route_count = targets.size();
    *this = candidate;
    return true;
}
bool MovementTimeline::start_segment(MovementPoint target, float speed, std::uint64_t now) noexcept {
    const float dx = target.x - m_position.x, dz = target.z - m_position.z;
    const float distance = std::sqrt(dx*dx + dz*dz);
    const MovementPoint velocity = distance ? MovementPoint{dx*(speed/distance), dz*(speed/distance)} : MovementPoint{};
    const float duration = distance / speed;
    if (!std::isfinite(velocity.x) || !std::isfinite(velocity.z) || !std::isfinite(duration)) return false;
    m_start = m_position; m_target = target; m_speed = speed; m_start_ms = now;
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
            m_position = m_target;
            if (m_route_index + 1 < m_route_count) {
                ++m_route_index;
                // Original StartMoveEx reenters CalcPositionEx with the same
                // timestamp and an aliased CurPosition start: that call clamps
                // the waypoint before it is used as the next segment's origin.
                m_position.x = std::clamp(m_position.x,0.0f,51100.0f);
                m_position.z = std::clamp(m_position.z,0.0f,51100.0f);
                // Original CalcPositionEx starts only one next segment at CurTime.
                // Overshoot is not carried across waypoints, including zero-length ones.
                (void)start_segment(m_route[m_route_index],m_speed,now);
            } else {
                m_moving = false;
                clear_route();
            }
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
    clear_route();
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
