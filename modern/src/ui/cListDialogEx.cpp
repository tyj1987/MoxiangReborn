// cListDialogEx.cpp — 1:1 port of 墨香 cListDialogEx (link list with
// WE_ROWCLICK callback + selected highlight + multi-color link chains).
//
// See cListDialogEx.hpp for the design rationale (data model
// duplication vs. inheritance from cListDialog) and the
// emLink_* enum mapping.

#include "cListDialogEx.hpp"

namespace mxh::ui {

cListDialogEx::cListDialogEx() = default;

cListDialogEx::~cListDialogEx() = default;

void cListDialogEx::InitLinkList(std::uint16_t maxLines) {
    m_linkItems.clear();
    m_rowClicked = false;
    SetMaxLine(maxLines);
    RemoveAll();  // also clears base cListDialog::m_rows
}

void cListDialogEx::AddLinkItem(const std::string& text,
                                std::uint8_t type,
                                std::uint32_t color,
                                std::uint32_t overColor,
                                int line) {
    LinkItem item;
    item.text      = text;
    item.type      = type;
    item.color     = color;
    item.overColor = overColor;
    if (line < 0 || static_cast<std::size_t>(line) >= m_linkItems.size()) {
        m_linkItems.push_back(std::move(item));
    } else {
        m_linkItems.insert(m_linkItems.begin() + line, std::move(item));
    }
    // 1:1 quirk: the legacy also adds a base row with the
    // same text + color (so the base cListDialog still has
    // something to scroll/clip if a subclass ever inherits
    // from cListDialogEx). Modern port keeps the
    // m_linkItems list as the single source of truth — the
    // base cListDialog::m_rows is left empty. This matches
    // the original's intent (the link item is rendered with
    // its own callback-aware code path) without forcing
    // duplicated state.
}

void cListDialogEx::AddLinkItemChain(const LinkItem& head) {
    m_linkItems.push_back(head);
}

void cListDialogEx::RemoveAll() noexcept {
    m_linkItems.clear();
    m_rowClicked = false;
    // Delegate to the base implementation so any rows that
    // the host might have added via cListDialog::AddItem
    // (e.g. via the base interface after a future refactor)
    // are also cleared.
    cListDialog::RemoveAll();
}

int cListDialogEx::ListMouseCheck(std::int32_t x, std::int32_t y,
                                  std::uint32_t we) noexcept {
    // Reuse the base class's hit test (1:1 with legacy
    // cListDialogEx::ListMouseCheck's call to PtIdxInRow).
    int selIdx = cListDialog::PtIdxInRow(x, y);
    if (selIdx < 0) {
        SetCurSelectedRowIdx(-1);
        return -1;
    }
    if (static_cast<std::size_t>(selIdx) >= m_linkItems.size()) {
        // Out of range for our link items — match legacy
        // behavior of leaving selection at -1 (the legacy
        // checks against m_lLineNum which is the visible
        // line count, but we compare against the actual
        // item count to avoid an off-by-one if the host
        // calls PtIdxInRow with a clip rect smaller than
        // the item list).
        SetCurSelectedRowIdx(-1);
        return -1;
    }
    SetCurSelectedRowIdx(selIdx);
    if (we & WE_LBTNCLICK) {
        const LinkItem& item = m_linkItems[selIdx];
        if (item.type > emLink_Null) {
            m_rowClicked = true;
            if (m_onRowClicked) {
                m_onRowClicked(selIdx, m_rowClickedUserData);
            }
        }
    }
    return selIdx;
}

}  // namespace mxh::ui
