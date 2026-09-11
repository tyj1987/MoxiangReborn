#pragma once
#include <cstdint>

namespace mxh::game {
struct MovementPoint { float x = 0; float z = 0; };
enum class MovementStopResult { Accepted, CorrectionRequired, InvalidInput };

// Single-segment math from [Server]Map/CharMove.cpp:106-234. No rendering,
// networking, collision policy, or speed selection belongs in this component.
// Server callers supply a monotonic millisecond clock and authoritative speed.
class MovementTimeline {
public:
    bool reset(MovementPoint position, std::uint64_t now_ms) noexcept;
    bool start(MovementPoint target, float units_per_second, std::uint64_t now_ms) noexcept;
    MovementPoint advance(std::uint64_t now_ms) noexcept;
    MovementStopResult stop(MovementPoint claimed, std::uint64_t now_ms) noexcept;
    void halt(std::uint64_t now_ms) noexcept;
    // Cached value only: reset mirrors InitMove's raw validated position.
    // Use advance(now) for authoritative reads, including the legacy world clamp.
    MovementPoint position() const noexcept { return m_position; }
    MovementPoint target() const noexcept { return m_target; }
    bool moving() const noexcept { return m_moving; }
    bool initialized() const noexcept { return m_initialized; }
    float speed() const noexcept { return m_speed; }
    static constexpr float stop_tolerance = 1000.0f;
private:
    static bool valid(MovementPoint point) noexcept;
    MovementPoint m_position{}, m_start{}, m_target{}, m_velocity{};
    std::uint64_t m_start_ms = 0, m_last_ms = 0;
    float m_duration_seconds = 0, m_speed = 0;
    bool m_moving = false, m_initialized = false;
};
} // namespace mxh::game
