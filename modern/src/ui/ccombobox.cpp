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
// 4. **Render uses the shared adapters.** The dropdown list
//    (top + middle + down sprites + per-item text + over-image
//    on hover) is drawn through cImage and TextRender.
//
// 5. **ActionEvent owns the local interaction state.** It
//    toggles the list, tracks hover and commits the selected row.
//
// 6. **Engine singletons stubbed.** cWindowManager->IsMouseOverUsed
//    / IsMouseDownUsed / SetMouseOverUsed / SetMouseDownUsed
//    are all no-op. The data-side state is preserved 1:1.

#include "ccombobox.hpp"

#include "cWindow.hpp"
#include "cImage.hpp"
#include "TextRender.hpp"

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

void cComboBox::Render() {
    if (!isVisible()) return;
    cWindow::Render();

    const auto drawImage = [](void* handle, std::int32_t x, std::int32_t y,
                              std::int32_t width, std::int32_t height) {
        if (!handle || width <= 0 || height <= 0) return;
        static_cast<cImage*>(handle)->render(x, y, width, height,
                                             0xFFFFFFFFu, 1);
    };
    if (!m_comboText.empty()) {
        TextRenderRequest text;
        text.text = m_comboText;
        text.x = absX();
        text.y = absY();
        text.width = width();
        text.height = height();
        text.left_inset = m_textClippingRect.left;
        text.right_inset = m_textClippingRect.right;
        text.color = m_comboTextColor;
        text.font_index = 0;
        text.align = TextRenderAlign::Left;
        renderText(text);
    }
    if (!m_dropdownOpen || GetItemCount() == 0 || m_middleHeight == 0) return;

    const auto listTop = absY() + static_cast<std::int32_t>(height());
    drawImage(m_topImage, absX(), listTop, m_listWidth, m_topHeight);
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        const auto rowY = listTop + static_cast<std::int32_t>(m_topHeight) +
                          static_cast<std::int32_t>(i) * m_middleHeight;
        drawImage(m_middleImage, absX(), rowY, m_listWidth, m_middleHeight);
        TextRenderRequest row;
        row.text = m_items[i].text;
        row.x = absX();
        row.y = rowY;
        row.width = m_listWidth;
        row.height = m_middleHeight;
        row.left_inset = m_textClippingRect.left;
        row.right_inset = m_textClippingRect.right;
        row.color = m_comboTextColor;
        row.font_index = 0;
        row.align = TextRenderAlign::Left;
        renderText(row);
        if (static_cast<int>(i) == m_nOverIdx) {
            const auto w = static_cast<std::int32_t>(
                static_cast<float>(m_listWidth) * m_overImageScaleX);
            const auto h = static_cast<std::int32_t>(
                static_cast<float>(m_middleHeight) * m_overImageScaleY);
            drawImage(m_overImage, absX(), rowY, w, h);
        }
    }
    const auto bottomY = listTop + static_cast<std::int32_t>(m_topHeight) +
                         static_cast<std::int32_t>(m_items.size()) * m_middleHeight;
    drawImage(m_downImage, absX(), bottomY, m_listWidth, m_downHeight);
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

std::uint32_t cComboBox::ActionEvent(std::int32_t mouseX,
                                     std::int32_t mouseY,
                                     std::uint32_t mouseFlags) {
    if (!isEnabled()) {
        m_dropdownOpen = false;
        m_nOverIdx = -1;
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    const bool left = (mouseFlags & MouseFlagLButton) != 0;
    if (left && PtInWindow(mouseX, mouseY)) {
        m_dropdownOpen = !m_dropdownOpen;
        m_nOverIdx = -1;
        return static_cast<std::uint32_t>(WindowEvent::LButtonDown);
    }
    if (left && m_dropdownOpen) {
        const auto row = PtIdxInComboList(mouseX, mouseY);
        if (row < GetItemCount()) {
            ListMouseCheck(mouseX, mouseY, true);
            m_dropdownOpen = false;
            return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
        }
        // Clicking elsewhere dismisses an open legacy combo list. Do this
        // before world/UI dispatch sees the same mouse-up so a stale dropdown
        // cannot consume subsequent clicks or leave an unreachable overlay.
        m_dropdownOpen = false;
        m_nOverIdx = -1;
        return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
    } else if (!left && m_dropdownOpen) {
        const auto row = PtIdxInComboList(mouseX, mouseY);
        m_nOverIdx = row < GetItemCount() ? static_cast<int>(row) : -1;
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
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
    const std::int32_t listnum = static_cast<std::int32_t>(GetItemCount());
    // The list is skinned as top cap + one middle slice per row + bottom
    // cap. Cap pixels are not selectable; return the legacy count+1 sentinel
    // for all non-row coordinates.
    if (m_listWidth == 0 || m_middleHeight == 0 || listnum == 0) {
        return static_cast<std::uint16_t>(listnum + 1);
    }
    const std::int32_t list_left = absX();
    const std::int32_t list_top = absY() + static_cast<std::int32_t>(height());
    const std::int32_t list_right = list_left + static_cast<std::int32_t>(m_listWidth);
    const std::int32_t rows_top = list_top + static_cast<std::int32_t>(m_topHeight);
    for (std::int32_t i = 0; i < listnum; ++i) {
        const std::int32_t row_top = rows_top + i * static_cast<std::int32_t>(m_middleHeight);
        const std::int32_t row_bottom = row_top + static_cast<std::int32_t>(m_middleHeight);
        if (list_left < x && x < list_right && row_top < y && y < row_bottom) {
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
