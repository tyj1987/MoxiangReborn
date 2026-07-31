// cchatoptiondialog.cpp -- modern implementation of
//   Moxiang CChatOptionDialog.

#include "cchatoptiondialog.hpp"

#include "ccheckbox.hpp"

namespace mxh::ui {

cChatOptionDialog::cChatOptionDialog() = default;

cChatOptionDialog::~cChatOptionDialog() = default;

void cChatOptionDialog::Init(std::int32_t x, std::int32_t y,
                             std::uint16_t wid, std::uint16_t hei,
                             void* basicImage, std::int32_t id) {
    // 1:1 with legacy CChatOptionDialog::Init.
    //   cDialog::Init(x,y,wid,hei,basicImage,ID);
    //   m_bFirst   = TRUE;
    //   m_bChanged = FALSE;
    cDialog::Init(x, y, wid, hei, basicImage, id);
    m_bFirst   = true;
    m_bChanged = false;
}

void cChatOptionDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CChatOptionDialog::SetActive override.
    //   if (m_bDisable) return;
    // 1:1 quirk: legacy reads cDialog::m_bDisable directly.
    // The modern cDialog encodes the disabled flag via
    // cWindow::isEnabled() (m_bEnabled).  We reach through
    // via the public accessor so the override stays at the
    // cDialog API level.
    if (!isEnabled()) return;
    cDialog::SetActive(val);
    if (val) {
        // Load the option array via the host callback.
        // 1:1 with legacy `sChatOption* pOption = CHATMGR->GetOption();`
        if (m_getOptionCb) {
            m_getOptionCb(m_options, m_getOptionUser);
        } else {
            // No host wired up: zero the array so the
            // checkboxes are deterministic (the legacy
            // would read from CHATMGR's globals; the modern
            // port surfaces the option array through the
            // host callback).
            for (auto& opt : m_options) opt = false;
        }
        // 1:1 with legacy checkbox sync loop.
        for (std::size_t i = 0; i < kChatOptionCount; ++i) {
            if (m_pBtnOption[i]) {
                m_pBtnOption[i]->SetChecked(m_options[i]);
            }
        }
        m_bFirst   = false;
        m_bChanged = false;
    } else if (!m_bFirst) {
        // 1:1 with legacy else-if branch.
        //   if (m_bChanged) CHATMGR->SaveUserOption();
        // 1:1 quirk: legacy does NOT reset m_bChanged
        // here (only on open).  Modern port preserves the
        // same semantics -- m_bChanged is latched until
        // the next open resets it.
        if (m_bChanged && m_saveOptionCb) {
            m_saveOptionCb(m_options, m_saveOptionUser);
        }
    }
}

void cChatOptionDialog::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy CChatOptionDialog::OnActionEvent.
    //   sChatOption* pOption = CHATMGR->GetOption();
    //   if (we & WE_CHECKED) pOption->bOption[lId - kOptBase] = TRUE;
    //   else if (we & WE_NOTCHECKED) pOption->bOption[lId - kOptBase] = FALSE;
    //   m_bChanged = TRUE;
    const std::int32_t idx = lId - kIdOptionBase;
    if (idx < 0 || static_cast<std::size_t>(idx) >= kChatOptionCount) {
        return;
    }
    if (we & kWeChecked) {
        m_options[idx] = true;
    } else if (we & kWeNotChecked) {
        m_options[idx] = false;
    }
    m_bChanged = true;
}

void cChatOptionDialog::Linking() {
    // 1:1 with legacy CChatOptionDialog::Linking.  The
    // legacy uses 12 cCheckBox children by id; the
    // modern port lets the host inject the pointers via
    // SetCheckBoxesForTest.
}

void cChatOptionDialog::SetCheckBoxesForTest(cCheckBox* const* boxes) noexcept {
    if (!boxes) return;
    for (std::size_t i = 0; i < kChatOptionCount; ++i) {
        m_pBtnOption[i] = boxes[i];
    }
}

} // namespace mxh::ui
