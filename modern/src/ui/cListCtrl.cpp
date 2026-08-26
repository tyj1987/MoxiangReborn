// mxh/ui/cListCtrl.cpp
// Phase 6.5 — implementation of the modern cListCtrl widget.
#include "cListCtrl.hpp"

#include <algorithm>

namespace mxh::ui {

void cListCtrl::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                     std::uint16_t hei, void* basicImage, std::int32_t id) {
    cWindow::Init(x, y, wid, hei, basicImage, id);
    m_columns.clear();
    m_rows.clear();
    m_selectedRowIdx = -1;
    m_topItemIdx     = 0;
    m_overRowIdx     = -1;
    m_linePerPage    = 0;
    m_maxColumns     = 0;
    m_selectOption   = 0;
    m_marginLeft     = 3;
    m_marginTop      = 4;
}

void cListCtrl::InitListCtrl(std::uint16_t wMaxColumns,
                              std::uint16_t wLinePerPage) {
    m_maxColumns  = wMaxColumns;
    m_linePerPage = wLinePerPage;
}

void cListCtrl::InitListCtrlImage(void* headImage, std::uint8_t headLineHeight,
                                  void* bodyImage, std::uint8_t bodyLineHeight,
                                  void* overImage) {
    m_headImage      = headImage;
    m_bodyImage      = bodyImage;
    m_overImage      = overImage;
    m_headLineHeight = headLineHeight;
    m_bodyLineHeight = bodyLineHeight;
}

void cListCtrl::SetColumns(std::vector<Column> cols) {
    m_columns = std::move(cols);
    // Validate row data against the new column count: rows that don't
    // match are accepted as-is (the render path will bounds-check), but
    // we at least cap each row's vector lengths to the column count to
    // avoid waste.
    for (auto& r : m_rows) {
        if (r.texts.size() > m_columns.size())   r.texts.resize(m_columns.size());
        if (r.colors.size() > m_columns.size())  r.colors.resize(m_columns.size());
    }
}

void cListCtrl::AddRow(Row row) {
    // The legacy engine permits rows with fewer cells than columns (the
    // missing cells render as empty). We accept whatever the caller
    // provides and let render() skip cells beyond row.texts.size().
    m_rows.push_back(std::move(row));
}

void cListCtrl::RemoveRowAt(std::size_t idx) {
    if (idx >= m_rows.size()) return;
    // Capture the pre-erase state so we can fix up the selection pointer.
    const std::int32_t sel = m_selectedRowIdx;
    m_rows.erase(m_rows.begin() + static_cast<std::ptrdiff_t>(idx));
    // Keep selection valid:
    //   - if selection was -1 (none)        → stays -1
    //   - if selection pointed at the removed row → clear (the data is gone)
    //   - if selection was after the removed row   → shift down by 1
    //   - if selection was before the removed row   → unchanged
    //   - if selection now points past the end of the (smaller) row array →
    //     clear.
    if (sel < 0) {
        // already cleared
    } else if (sel == static_cast<std::int32_t>(idx)) {
        m_selectedRowIdx = -1;
    } else if (sel > static_cast<std::int32_t>(idx)) {
        const std::int32_t newSel = sel - 1;
        m_selectedRowIdx = (newSel < static_cast<std::int32_t>(m_rows.size()))
                            ? newSel : -1;
    } else {
        // sel < idx → selection unaffected, but verify it's still in range.
        if (sel >= static_cast<std::int32_t>(m_rows.size())) {
            m_selectedRowIdx = -1;
        }
    }
}

void cListCtrl::RemoveAll() noexcept {
    m_rows.clear();
    m_selectedRowIdx = -1;
    m_overRowIdx     = -1;
    m_topItemIdx     = 0;
}

void cListCtrl::SetTopItemIdx(std::int32_t idx) noexcept {
    if (idx < 0) idx = 0;
    const std::int32_t maxTop = (m_linePerPage > 0)
        ? static_cast<std::int32_t>(m_rows.size()) - m_linePerPage
        : 0;
    if (idx > maxTop) idx = maxTop;
    if (idx < 0)      idx = 0;
    m_topItemIdx = idx;
}

void cListCtrl::SetMargin(std::int32_t left, std::int32_t top) noexcept {
    m_marginLeft = left;
    m_marginTop  = top;
}

std::uint16_t cListCtrl::PtIdxInRow(std::int32_t x, std::int32_t y) const noexcept {
    // Hit test only the body area (below the header). y must be strictly
    // greater than the header bottom edge; x must be inside the list's
    // x-extent; y must be inside one of the visible rows.
    const std::int32_t headerBottom = absY() + m_headLineHeight;
    if (x <= absX() || x >= absX() + width())          return m_linePerPage + 1;
    if (y <= headerBottom)                            return m_linePerPage + 1;
    const std::int32_t bodyTop    = headerBottom;
    const std::int32_t bodyHeight = static_cast<std::int32_t>(m_linePerPage) * m_bodyLineHeight;
    if (y > bodyTop + bodyHeight)                    return m_linePerPage + 1;
    const std::int32_t rel = y - bodyTop;
    if (m_bodyLineHeight == 0)                       return m_linePerPage + 1;
    const std::int32_t row = rel / m_bodyLineHeight;
    if (row < 0 || row >= m_linePerPage)             return m_linePerPage + 1;
    return static_cast<std::uint16_t>(row);
}

std::uint32_t cListCtrl::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                      std::uint32_t mouseFlags) {
    if (!isEnabled() || !isVisible()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    const std::uint16_t visibleRow = PtIdxInRow(mouseX, mouseY);
    const bool insideRow = visibleRow < m_linePerPage;
    const std::int32_t row = insideRow
        ? m_topItemIdx + static_cast<std::int32_t>(visibleRow)
        : -1;
    if (insideRow && row >= 0 && row < static_cast<std::int32_t>(m_rows.size())) {
        m_overRowIdx = row;
    } else {
        m_overRowIdx = -1;
    }
    // Click: select the row + invoke the click callback.
    if (mouseFlags & MouseFlagLButton) {
        if (insideRow && row >= 0 && row < static_cast<std::int32_t>(m_rows.size())) {
            m_selectedRowIdx = row;
            if (m_onClick) m_onClick(*this, m_selectedRowIdx, m_userdata);
            return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
        }
        // Click outside any row: clear selection.
        m_selectedRowIdx = -1;
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    return static_cast<std::uint32_t>(WindowEvent::MouseMove);
}

std::uint32_t cListCtrl::ActionKeyboardEvent(std::int32_t key,
                                             std::int32_t /*ch*/) {
    if (!isEnabled() || !isVisible() || !hasFocus() || m_rows.empty()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    const int visible = std::max(1, static_cast<int>(m_linePerPage));
    int selected = m_selectedRowIdx;
    if (selected < 0) selected = m_topItemIdx;
    if (key == 38) --selected;             // VK_UP
    else if (key == 40) ++selected;        // VK_DOWN
    else if (key == 33) selected -= visible; // VK_PRIOR
    else if (key == 34) selected += visible; // VK_NEXT
    else return static_cast<std::uint32_t>(WindowEvent::Null);
    selected = std::clamp(selected, 0, static_cast<int>(m_rows.size()) - 1);
    m_selectedRowIdx = selected;
    if (selected < m_topItemIdx) m_topItemIdx = selected;
    if (selected >= m_topItemIdx + visible) m_topItemIdx = selected - visible + 1;
    return static_cast<std::uint32_t>(WindowEvent::KeyDown);
}

} // namespace mxh::ui
