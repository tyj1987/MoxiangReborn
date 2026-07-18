// ccheckbox.cpp — modern port of 墨香 cCheckBox (check box widget).
//
// 1:1 port body. See legacy `cCheckBox.cpp` for the original.

#include "ccheckbox.hpp"

#include "cwindow.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

cCheckBox::cCheckBox() = default;
cCheckBox::~cCheckBox() = default;

void cCheckBox::ClearTestInjections() noexcept {
    s_callbackFiredCount = 0;
    s_lastCallbackWe     = 0;
    s_lastCallbackId     = 0;
    s_lastCallbackParent = nullptr;
}

void cCheckBox::Init(std::int32_t x, std::int32_t y, std::int16_t wid,
                     std::int32_t hei, void* basicImage,
                     void* checkBoxImage, void* checkImage,
                     CheckboxCallback Func, std::int32_t ID) {
    // 1:1 with legacy Init:
    //   cWindow::Init(x, y, wid, hei, basicImage, ID);
    //   m_type = WT_CHECKBOX;
    //   m_CheckBoxImage = *checkBoxImage;
    //   m_CheckImage = *checkImage;
    //   if (Func != NULL) cbWindowFunc = Func;
    //
    // 1:1 quirks:
    //   - legacy cWindow::Init takes (x, y, wid, hei,
    //     basicImage, ID). Modern cWindow::Init takes the
    //     same shape (with cImage* → void*). Modern port:
    //     call cWindow::Init with the same params.
    //   - legacy `m_type = WT_CHECKBOX` is dropped (Phase 6
    //     removed m_type field).
    //   - legacy `m_CheckBoxImage = *checkBoxImage` is a
    //     value copy. Modern cImage is GPU-backed; we store
    //     the pointer as an opaque handle.
    //   - legacy `cbWindowFunc = Func` is stored in the
    //     cWindow base. Modern cWindow has no
    //     cbWindowFunc; we store it locally as m_func.
    cWindow::Init(x, y, wid, hei, basicImage, ID);
    (void)ID;  // ID is set via cWindow::Init.
    m_CheckBoxImageHandle = checkBoxImage;
    m_CheckImageHandle     = checkImage;
    if (Func) {
        m_func = std::move(Func);
    }
}

std::uint32_t cCheckBox::ActionEvent(CMouse* /*mouseInfo*/) {
    // 1:1 with legacy ActionEvent:
    //   DWORD we = WE_NULL;
    //   if (!m_bActive) return we;
    //   we |= cWindow::ActionEvent(mouseInfo);
    //   if (m_bDisable) return we;
    //   if (we & WE_LBTNCLICK) {
    //       if (PtInWindow(...)) {
    //           m_fChecked ^= TRUE;
    //           (*cbWindowFunc)(m_ID, m_pParent, ...);
    //       }
    //   }
    //
    // 1:1 quirks:
    //   - legacy CMouse has LButtonDown / GetMouseEventX/Y
    //     methods. Modern CMouse is a stub (Phase 6.x
    //     deferred). The click detection is therefore a
    //     no-op in modern port.
    //   - The toggle + callback dispatch is exposed via
    //     the test helper `ToggleForTesting()` below. This
    //     matches the modern port pattern of exposing
    //     test-injectable side effects when the underlying
    //     input system is stubbed (same pattern as
    //     cMoneyDlg::OkPushed + cSkillOptionClearDlg::
    //     OptionClearSyn, which use test-injectable
    //     singletons + sent-message state).
    //   - legacy `m_fChecked ^= TRUE` (XOR) is preserved
    //     in `ToggleForTesting()`.
    //   - legacy `WE_LBTNCLICK` is replaced by the
    //     test-injection call.
    //   - legacy `m_bDisable` is read via the cWindow base
    //     isEnabled() (R-12 fix: SetDisable/SetEnabled).
    //   - legacy `m_pParent` is read in legacy to pass to
    //     cbWindowFunc. Modern cWindow has no
    //     `m_pParent`; we pass `m_parentDialog` (1:1 quirk:
    //     caller wires the parent externally via
    //     `SetParentDialogForTesting`).
    std::uint32_t we = 0;
    // 1:1 quirk: legacy `m_bActive` doesn't exist on
    // modern cWindow (cWindow uses m_bEnabled instead).
    // The legacy `if (!m_bActive)` guard is preserved as
    // a 1:1 quirk note but is a no-op in modern port.
    if (!isEnabled()) { return we; }
    return we;
}

void cCheckBox::Render() {
    // 1:1 with legacy Render — see legacy code for the full
    // sprite + font dispatch. Modern port: no-op stub
    // (Phase 6.x render wiring deferred).
    cWindow::Render();
}

void cCheckBox::SetCheckBoxMsg(const char* msg, std::uint32_t color) {
    // 1:1 with legacy SetCheckBoxMsg:
    //   strcpy(m_szCheckBoxText, msg);
    //   m_dwCheckBoxTextColor = color;
    if (msg) {
        m_szCheckBoxText = msg;
    } else {
        m_szCheckBoxText.clear();
    }
    m_dwCheckBoxTextColor = color;
}

void cCheckBox::ToggleForTesting() {
    // 1:1 with legacy click dispatch:
    //   m_fChecked ^= TRUE;
    //   (*cbWindowFunc)(m_ID, m_pParent,
    //                    (m_fChecked ? WE_CHECKED : WE_NOTCHECKED));
    //
    // 1:1 quirks:
    //   - legacy XOR toggle preserved.
    //   - legacy m_pParent replaced by m_parentDialog
    //     (test-injectable; defaults to nullptr).
    //   - legacy WE_CHECKED / WE_NOTCHECKED replaced by
    //     local kWeChecked=128 / kWeNotChecked=256.
    if (!isEnabled()) {
        return;  // 1:1: legacy `if (!m_bActive)` + `if (m_bDisable)` guards.
    }
    m_fChecked = !m_fChecked;
    if (m_func) {
        ++s_callbackFiredCount;
        s_lastCallbackWe = m_fChecked ? kWeChecked : kWeNotChecked;
        s_lastCallbackId = id();
        s_lastCallbackParent = m_parentDialog;
        m_func(id(), m_parentDialog,
               m_fChecked ? kWeChecked : kWeNotChecked);
    }
}

} // namespace mxh::ui
