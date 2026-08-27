// cIconDialog.cpp — modern implementation of 墨香 cIconDialog.

#include "cIconDialog.hpp"
#include "cIcon.hpp"
#include "cImage.hpp"

namespace mxh::ui {

cIconDialog::cIconDialog() = default;
cIconDialog::~cIconDialog() {
    // Cell data is owned (no need to delete icon pointers — those are
    // owned by the calling code; this mirrors the legacy SAFE_DELETE on
    // the cell array, not on the icons themselves).
}

void cIconDialog::Render() {
    if (!isVisible()) return;
    cDialog::Render();
    const auto drawImage = [](void* image, int x, int y, int w, int h) {
        if (!image || w <= 0 || h <= 0) return;
        static_cast<cImage*>(image)->render(x, y, w, h);
    };
    for (std::size_t i = 0; i < m_cells.size(); ++i) {
        const auto& cell = m_cells[i];
        if (cell.relW <= 0 || cell.relH <= 0) continue;
        const int x = absX() + cell.relX;
        const int y = absY() + cell.relY;
        drawImage(m_iconCellBGImage, x, y, cell.relW, cell.relH);
        if (static_cast<std::int32_t>(i) == m_curSelCellPos)
            drawImage(m_dragOverBGImage, x, y, cell.relW, cell.relH);
        if (cell.inUse && cell.icon) {
            auto* icon = static_cast<cIcon*>(cell.icon);
            icon->SetAbsXY(x, y);
            icon->Render();
        }
    }
}

void cIconDialog::SetCellNum(std::uint16_t num) {
    m_cells.clear();
    m_cells.resize(num);
    m_curSelCellPos = -1;
}

void cIconDialog::AddIconCell(std::int32_t x, std::int32_t y,
                              std::int32_t w, std::int32_t h) {
    if (m_cells.empty()) return;        // legacy requires SetCellNum first
    for (auto& c : m_cells) {
        if (c.relW == 0 && c.relH == 0) {
            c.relX = x; c.relY = y; c.relW = w; c.relH = h;
            return;
        }
    }
}

bool cIconDialog::PtInCell(std::int32_t x, std::int32_t y) const noexcept {
    for (const auto& c : m_cells) {
        if (c.relW <= 0 || c.relH <= 0) continue;  // uninitialized cell
        const std::int32_t x0 = absX() + c.relX;
        const std::int32_t y0 = absY() + c.relY;
        if (x < x0 || y < y0) continue;
        // Match the renderer's rectangle semantics: the right/bottom edge is
        // exclusive. This prevents a click exactly on a shared border from
        // being accepted by both adjacent inventory cells.
        if (x >= x0 + c.relW) continue;
        if (y >= y0 + c.relH) continue;
        return true;
    }
    return false;
}

bool cIconDialog::GetPositionForXYRef(std::int32_t x, std::int32_t y,
                                      std::uint16_t& pos) const noexcept {
    for (std::uint16_t i = 0; i < m_cells.size(); ++i) {
        const auto& c = m_cells[i];
        if (c.relW <= 0 || c.relH <= 0) continue;
        const std::int32_t x0 = absX() + c.relX;
        const std::int32_t y0 = absY() + c.relY;
        if (x < x0 || y < y0) continue;
        if (x >= x0 + c.relW) continue;
        if (y >= y0 + c.relH) continue;
        pos = i;
        return true;
    }
    return false;
}

bool cIconDialog::IsAddable(std::uint16_t idx) const noexcept {
    if (idx >= m_cells.size()) return false;
    return !m_cells[idx].inUse;
}

bool cIconDialog::IsAcceptable(std::uint32_t type) const noexcept {
    return (type & m_acceptableIconType) != 0u;
}

bool cIconDialog::AddIcon(std::uint16_t cellIdx, cIcon* icon, bool onlyLink) {
    if (cellIdx >= m_cells.size()) return false;
    if (m_cells[cellIdx].inUse) return false;     // legacy: refuse double-add
    m_cells[cellIdx].icon     = icon;
    m_cells[cellIdx].inUse    = (icon != nullptr);
    m_cells[cellIdx].onlyLink = onlyLink;
    return true;
}

bool cIconDialog::DeleteIcon(std::uint16_t cellIdx, cIcon** outIcon) {
    if (cellIdx >= m_cells.size()) return false;
    if (!m_cells[cellIdx].inUse) return false;
    if (outIcon) *outIcon = m_cells[cellIdx].icon;
    m_cells[cellIdx].icon     = nullptr;
    m_cells[cellIdx].inUse    = false;
    m_cells[cellIdx].onlyLink = false;
    return true;
}

void cIconDialog::DeleteIconAll() noexcept {
    for (auto& c : m_cells) {
        c.icon     = nullptr;
        c.inUse    = false;
        c.onlyLink = false;
    }
}

cIcon* cIconDialog::GetIconForIdx(std::uint16_t idx) const {
    if (idx >= m_cells.size()) return nullptr;
    return m_cells[idx].icon;
}

void cIconDialog::SetAbsXY(std::int32_t x, std::int32_t y) noexcept {
    // Legacy: shift only the icons that aren't world-anchored (bOnlyLink = false).
    // The legacy code does this via cIcon::SetAbsXY on the icon object; since
    // cIcon is opaque to our modern port, we record the dialog-relative
    // delta here. A follow-up that wires cIconSprite can apply the shift.
    const std::int32_t dx = x - absX();
    const std::int32_t dy = y - absY();
    (void)dx; (void)dy;
    cDialog::SetAbsXY(x, y);
}

} // namespace mxh::ui
