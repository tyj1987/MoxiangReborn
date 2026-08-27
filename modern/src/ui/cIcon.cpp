#include "cIcon.hpp"
#include "cImage.hpp"

namespace mxh::ui {

void cIcon::InitIcon(std::int32_t x, std::int32_t y, std::uint16_t w,
                     std::uint16_t h, cImage* image,
                     std::uint32_t type, std::int32_t id) noexcept {
    Init(x, y, w, h, image, id);
    m_image = image;
    m_type = type;
}

void cIcon::Render() {
    if (!isVisible()) return;
    cWindow::Render();
    if (m_image && basicImage() != m_image) {
        m_image->render(absX(), absY(), width(), height());
    }
}

} // namespace mxh::ui
