#pragma once

#include <cstdint>

namespace mxh::client {

// Input state for the character-select/create 3D preview.  The controller is
// deliberately renderer-agnostic so its interaction contract can be tested
// without a window or a D3D device.
class CharacterPreviewController final {
public:
    static constexpr float kDefaultYaw = 0.0f;
    static constexpr float kDefaultDistance = 4.8f;
    static constexpr float kMinDistance = 2.6f;
    static constexpr float kMaxDistance = 8.0f;

    void reset() noexcept;

    // Returns true when the preview consumed the button.  Right-button drag
    // is the legacy-style rotate gesture; left button remains available to
    // the dialog and therefore is never consumed here.
    bool onMouseButton(bool right, bool down, std::int32_t x,
                       std::int32_t y) noexcept;
    bool onMouseMove(std::int32_t x, std::int32_t y) noexcept;
    bool onMouseWheel(std::int32_t wheelDelta) noexcept;

    [[nodiscard]] float yaw() const noexcept { return yaw_; }
    [[nodiscard]] float distance() const noexcept { return distance_; }
    [[nodiscard]] bool rotating() const noexcept { return rotating_; }

private:
    float yaw_ = kDefaultYaw;
    float distance_ = kDefaultDistance;
    std::int32_t last_x_ = 0;
    std::int32_t last_y_ = 0;
    bool rotating_ = false;
};

} // namespace mxh::client
