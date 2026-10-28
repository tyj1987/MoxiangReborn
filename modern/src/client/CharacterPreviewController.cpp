#include "mxh/client/CharacterPreviewController.hpp"

#include <algorithm>

namespace mxh::client {

void CharacterPreviewController::reset() noexcept {
    yaw_ = kDefaultYaw;
    distance_ = kDefaultDistance;
    last_x_ = 0;
    last_y_ = 0;
    rotating_ = false;
}

bool CharacterPreviewController::onMouseButton(bool right, bool down,
                                                std::int32_t x,
                                                std::int32_t y) noexcept {
    if (!right) return false;
    if (down) {
        rotating_ = true;
        last_x_ = x;
        last_y_ = y;
    } else {
        rotating_ = false;
    }
    return true;
}

bool CharacterPreviewController::onMouseMove(std::int32_t x,
                                             std::int32_t y) noexcept {
    if (!rotating_) return false;
    // The original client uses a modest mouse-to-yaw ratio; keep the same
    // practical feel while making the result frame-rate independent.
    yaw_ += static_cast<float>(x - last_x_) * 0.0125f;
    last_x_ = x;
    last_y_ = y;
    return true;
}

bool CharacterPreviewController::onMouseWheel(std::int32_t wheelDelta) noexcept {
    if (wheelDelta == 0) return false;
    distance_ = std::clamp(distance_ - static_cast<float>(wheelDelta) / 120.0f * 0.45f,
                           kMinDistance, kMaxDistance);
    return true;
}

} // namespace mxh::client
