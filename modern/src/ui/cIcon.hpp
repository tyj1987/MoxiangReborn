#pragma once

#include "cWindow.hpp"

#include <cstdint>

namespace mxh::ui {

class cImage;

// Minimal concrete icon instance used by inventory-style containers. The
// sprite is borrowed from the resource manager, matching cImage ownership.
class cIcon : public cWindow {
public:
    cIcon() = default;
    ~cIcon() override = default;

    void InitIcon(std::int32_t x, std::int32_t y, std::uint16_t w,
                  std::uint16_t h, cImage* image,
                  std::uint32_t type = 0, std::int32_t id = 0) noexcept;
    void SetImage(cImage* image) noexcept { m_image = image; }
    cImage* image() const noexcept { return m_image; }
    void SetIconType(std::uint32_t type) noexcept { m_type = type; }
    std::uint32_t iconType() const noexcept { return m_type; }
    void Render() override;

private:
    cImage* m_image = nullptr;
    std::uint32_t m_type = 0;
};

} // namespace mxh::ui
