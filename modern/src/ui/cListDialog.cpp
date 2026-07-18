// cListDialog.cpp — modern implementation of 墨香 cListDialog.

#include "cListDialog.hpp"

namespace mxh::ui {

cListDialog::cListDialog() = default;
cListDialog::~cListDialog() = default;

void cListDialog::InitList(std::uint16_t maxLines,
                            std::int32_t clipX, std::int32_t clipY,
                            std::int32_t clipW, std::int32_t clipH) {
    m_maxLine  = maxLines;
    m_clipX    = clipX;
    m_clipY    = clipY;
    m_clipW    = clipW;
    m_clipH    = clipH;
    m_rows.clear();
    m_selectedRow = -1;
    m_topRow      = 0;
}

void cListDialog::AddItem(std::string text, std::uint32_t color, int line) {
    // Legacy: if `line == -1`, append at the end; otherwise insert at
    // `line`. We honor the same convention.
    if (line < 0 || line >= static_cast<int>(m_rows.size())) {
        m_rows.emplace_back(std::move(text), color);
    } else {
        m_rows.insert(m_rows.begin() + line, Row{std::move(text), color});
    }
    // Clamp selection if it was a valid index now out of range.
    if (m_selectedRow >= static_cast<int>(m_rows.size())) {
        m_selectedRow = static_cast<int>(m_rows.size()) - 1;
    }
}

bool cListDialog::RemoveItem(const std::string& text) {
    // 1:1 with legacy cListDialog::RemoveItem(const char* text) — first
    // match wins. We do exact match (legacy cStrcmp-based); a partial
    // match would be a behavior change.
    for (auto it = m_rows.begin(); it != m_rows.end(); ++it) {
        if (it->first == text) {
            const int removedIdx = static_cast<int>(it - m_rows.begin());
            m_rows.erase(it);
            // Adjust selection if the removed row was at or before the
            // current selected row (1:1 with legacy list-dialog semantics).
            if (m_selectedRow > removedIdx) {
                --m_selectedRow;
            } else if (m_selectedRow == removedIdx) {
                m_selectedRow = -1;
            }
            if (m_topRow >= static_cast<int>(m_rows.size())) {
                m_topRow = static_cast<int>(m_rows.size()) - 1;
                if (m_topRow < 0) m_topRow = 0;
            }
            return true;
        }
    }
    return false;
}

void cListDialog::SetTopListItemIdx(int idx) noexcept {
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(m_rows.size())) {
        idx = static_cast<int>(m_rows.size()) - 1;
    }
    if (idx < 0) idx = 0;
    m_topRow = idx;
}

void cListDialog::OnUpwardItem() {
    if (m_rows.empty()) return;
    if (m_selectedRow > 0) --m_selectedRow;
    if (m_selectedRow < m_topRow) m_topRow = m_selectedRow;
}

void cListDialog::OnDownwardItem() {
    if (m_rows.empty()) return;
    if (m_selectedRow < static_cast<int>(m_rows.size()) - 1) ++m_selectedRow;
    const int visible = (m_lineHeight > 0)
                            ? (m_clipH / m_lineHeight)
                            : static_cast<int>(m_rows.size());
    if (m_selectedRow >= m_topRow + visible) {
        m_topRow = m_selectedRow - visible + 1;
    }
    if (m_topRow < 0) m_topRow = 0;
}

int cListDialog::PtIdxInRow(std::int32_t x, std::int32_t y) const noexcept {
    if (m_lineHeight <= 0) return -1;
    if (x < m_clipX || x > m_clipX + m_clipW) return -1;
    if (y < m_clipY || y > m_clipY + m_clipH) return -1;
    const int row = m_topRow + (y - m_clipY) / m_lineHeight;
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return -1;
    return row;
}

} // namespace mxh::ui
