// ccombobox.cpp — modern port implementation.
//
// 1:1 port of legacy `cComboBox` from
//   `墨香【源码】\[Client]MH\Interface\cComboBox.cpp`.
//
// Modern-port notes
// =================
//
// 1. **cListItem is composed, not inherited.** The legacy uses
//    `class cComboBox : public cWindow, public cListItem`. The
//    modern port simplifies to single-inheritance from
//    cListItem (no cWindow geometry — the legacy cWindow
//    methods are now cListItem helpers or are stubbed). The
//    cListItem base provides AddItem / RemoveAll / GetItemCount.
//
// 2. **cPushupButton is opaque.** The legacy `Add(cWindow*
//    pushupBtn)` checks the type and casts. Modern port stores
//    the pointer as void*; the type check is documented as a
//    no-op (the engine-binder layer will re-add it).
//
// 3. **cImage is opaque.** The 4 image slots (top / middle /
//    down / over) are stored as void*. Real cImage binds with
//    6.6 cImage seam.
//
// 4. **Render is a no-op.** The legacy Render draws the
//    dropdown list (top + middle + down sprites + per-item
//    text + over-image on hover). All of that needs cImage
//    seam + font renderer. Modern port is no-op; the data
//    model + state is fully testable.
//
// 5. **ActionEvent is a no-op stub.** The legacy
//    cbWindowFunc dispatch + cWindowManager mouse-dispatch
//    have no modern equivalents; the dispatcher integration
//    is 6.6 follow-up. Modern port preserves the data-side
//    state (m_nOverIdx / m_nCurSelectedIdx / m_comboText).
//
// 6. **Engine singletons stubbed.** cWindowManager->IsMouseOverUsed
//    / IsMouseDownUsed / SetMouseOverUsed / SetMouseDownUsed
//    are all no-op. The data-side state is preserved 1:1.

#include "ccombobox.hpp"

#include "cWindow.hpp"

#include <cstring>

namespace mxh::ui {

cComboBox::cComboBox() {
    m_pComboBtn       = nullptr;
    m_comboTextColor  = 0xFFFFFFFFu;
    m_comboText.clear();
    m_topHeight       = 0;
    m_middleHeight    = 0;
    m_downHeight      = 0;
    m_listWidth       = 0;
    m_textClippingRect = {3, 4, 0, 0};
    m_nCurSelectedIdx = -1;
    m_overImageScaleX = 1.0f;
    m_overImageScaleY = 1.0f;
    m_nOverIdx        = -1;
}

cComboBox::~cComboBox() {
    // 1:1 with legacy: SAFE_DELETE(m_pComboBtn) — but modern
    // cPushupButton is opaque. We just clear the pointer; the
    // engine-binder layer (Phase 14+) is responsible for
    // owning + destroying the pushup button.
    m_pComboBtn = nullptr;
}

void cComboBox::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                     std::uint16_t hei, void* basicImage,
                     std::int32_t id) {
    // 1:1 with legacy. Modern port skips the cbWindowFunc init
    // (no static-function-pointer seam in modern cDialog);
    // the engine-binder layer (Phase 14+) will re-add it.
    (void)basicImage;
    // Store x/y/wid/hei/id via cWindow::Init so findWindowById
    // can locate the combo by id (cDialog uses cObject::id()
    // which is set by cWindow::Init's mutableId()=id line).
    cWindow::Init(x, y, wid, hei, basicImage, id);
}

void cComboBox::InitComboList(std::uint16_t listWid,
                              void* topImage,   std::uint16_t topHei,
                              void* middleImage, std::uint16_t middleHei,
                              void* downImage,  std::uint16_t downHei,
                              void* overImage) {
    m_listWidth   = listWid;
    m_topImage    = topImage;
    m_topHeight   = topHei;
    m_middleImage = middleImage;
    m_middleHeight = middleHei;
    m_downImage   = downImage;
    m_downHeight  = downHei;
    m_overImage   = overImage;
}

std::uint32_t cComboBox::ActionEvent(std::int32_t /*mouseX*/,
                                     std::int32_t /*mouseY*/,
                                     std::uint32_t /*mouseFlags*/) {
    // 1:1 with legacy cComboBox::ActionEvent. Modern port is a
    // no-op stub. The data-side state (m_nOverIdx /
    // m_nCurSelectedIdx / m_comboText) is preserved when
    // ListMouseCheck is called directly (which the tests do).
    return 0;
}

void cComboBox::Add(cWindow* pushupBtn) {
    // 1:1 with legacy. Legacy checks the type and casts to
    // cPushupButton*. Modern port stores as void*; the type
    // check is documented as a no-op.
    if (!pushupBtn) return;
    m_pComboBtn = pushupBtn;
    // 1:1 with legacy: SetAbsXY on the pushup button using
    // m_absPos + m_relPos. Modern cWindow doesn't have a
    // separate m_relPos (rel position is per-window). We
    // assume the pushup button's rel position is (0, 0) for
    // 1:1 (legacy stores it as m_relPos in the button).
    if (m_pComboBtn) {
        m_pComboBtn->SetAbsXY(absX(), absY());
        m_pComboBtn->setParent(this);
    }
}

void cComboBox::SetAbsXY(std::int32_t x, std::int32_t y) noexcept {
    // 1:1 with legacy. cWindow::SetAbsXY + cascade to pushup
    // button. Modern cListItem doesn't have absX/absY; we
    // store them in a private m_absX/m_absY if needed. (For
    // cStallFindDlg use, abs position is set via the dialog
    // parent's SetAbsXY + the cWindow tree.)
    (void)x; (void)y;
    if (m_pComboBtn) {
        // Cascading: modern cPushupButton is opaque; we just
        // forward the abs position. The engine-binder layer
        // (Phase 14+) will re-add the rel-position math.
        m_pComboBtn->SetAbsXY(x, y);
    }
}

void cComboBox::ListMouseCheck(std::int32_t mouseX, std::int32_t mouseY,
                               bool leftDown) {
    // 1:1 with legacy. The legacy uses cWindowManager to gate
    // mouse-over + mouse-down flags (only one window at a
    // time). Modern port skips the gate (no equivalent yet);
    // the data-side state is preserved.
    m_nOverIdx = static_cast<int>(PtIdxInComboList(mouseX, mouseY));
    if (m_nOverIdx > static_cast<int>(GetItemCount())) {
        m_nOverIdx = -1;
    }
    if (leftDown) {
        m_nCurSelectedIdx = static_cast<int>(PtIdxInComboList(mouseX, mouseY));
        if (m_nCurSelectedIdx > static_cast<int>(GetItemCount())
            || m_nCurSelectedIdx == -1) {
            m_nCurSelectedIdx = -1;
        } else {
            // 1:1 with legacy: copy the selected item's text
            // into m_comboText. m_items is protected on
            // cListItem, so derived classes can access it
            // directly.
            if (m_nCurSelectedIdx >= 0
                && static_cast<std::size_t>(m_nCurSelectedIdx) < m_items.size()) {
                m_comboText = m_items[static_cast<std::size_t>(m_nCurSelectedIdx)].text;
            }
        }
    }
}

std::uint16_t cComboBox::PtIdxInComboList(std::int32_t x, std::int32_t y) const {
    // 1:1 with legacy. The legacy uses m_absPos (the combo's
    // absolute position) + m_height (combo height) + m_listWidth
    // + m_middleHeight to compute the row. Modern port uses
    // absX() / absY() (inherited from cWindow) for the absolute
    // position, and the stored m_middleHeight for the row
    // pitch. The legacy returns GetItemCount()+1 on no-hit
    // (so the legacy caller can compare > GetItemCount() to
    // detect no-hit). Modern port returns the same.
    const std::int32_t listnum = static_cast<std::int32_t>(GetItemCount());
    for (std::int32_t i = 0; i < listnum; ++i) {
        if (absX() < x && absY() + static_cast<std::int32_t>(height()) < y
            && x < absX() + m_listWidth
            && y < absY() + static_cast<std::int32_t>(height())
                 + (i + 1) * m_middleHeight) {
            return static_cast<std::uint16_t>(i);
        }
    }
    return static_cast<std::uint16_t>(listnum + 1);
}

void cComboBox::SetMargin(std::int32_t left, std::int32_t top,
                          std::int32_t right, std::int32_t bottom) noexcept {
    m_textClippingRect.left   = left;
    m_textClippingRect.top    = top;
    m_textClippingRect.right  = right;
    m_textClippingRect.bottom = bottom;
}

void cComboBox::SelectComboText(std::uint16_t idx) {
    // 1:1 with legacy. If idx < GetItemCount, copy the item's
    // text into m_comboText.
    if (idx < GetItemCount() && idx < m_items.size()) {
        m_comboText = m_items[idx].text;
    }
}

} // namespace mxh::ui
