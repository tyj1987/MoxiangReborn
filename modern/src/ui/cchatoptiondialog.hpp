// cchatoptiondialog.hpp -- modern port of Moxiang CChatOptionDialog
//   (chat option toggles).
//
// 1:1 port of legacy `CChatOptionDialog` from
//   `[Client]MH\ChatOptionDialog.{h,cpp}` (the header is
//   mostly commented out; the implementation lives in the
//   first 50 lines of the .cpp).
//
// The chat-option dialog is a 12-row toggle list that lets
// the player turn individual chat channels on / off.  The
// legacy state is `sChatOption bOption[CTO_COUNT]` where
// CTO_COUNT = 12; the modern port keeps the same shape
// (`m_options[kChatOptionCount]`) and exposes
// `GetOption` / `SaveOption` callbacks for the host to
// read / write the entire array in one shot (1:1 with
// legacy `CHATMGR->GetOption()` / `CHATMGR->SaveUserOption()`).
//
// 1:1 dependencies:
//   * 12 cCheckBox children (m_pBtnOption[12])
//   * CHATMGR->GetOption() / SaveUserOption() (modern
//     port routes both through the host; the modern port
//     tracks the local m_options array + emits a
//     SaveOption callback when the user closes the dialog
//     for the first time with changes pending)
//
// Modern port keeps the legacy surface (Init +
// SetActive override + OnActionEvent + Linking).  The
// host wires up the 12 cCheckBox pointers via
// SetCheckBoxesForTest (replaces the legacy
// GetWindowForID lookups).  The host supplies the
// GetOption / SaveOption callbacks to integrate with
// the modern chat manager.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cCheckBox;

class cChatOptionDialog : public cDialog {
public:
    cChatOptionDialog();
    ~cChatOptionDialog() override;

    cChatOptionDialog(const cChatOptionDialog&) = delete;
    cChatOptionDialog& operator=(const cChatOptionDialog&) = delete;

    // 1:1 with legacy CChatOptionDialog::Init.
    // 1:1 quirk: cWindow::Init is non-virtual in the
    // modern port, so this is a member function that
    // hides the base class Init (matches the legacy
    // engine's non-virtual override pattern -- callers
    // invoke it through a concrete cChatOptionDialog*).
    void Init(std::int32_t x, std::int32_t y,
              std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id);

    // 1:1 with legacy CChatOptionDialog::SetActive override.
    //   if (m_bDisable) return;
    //   cDialog::SetActive(val);
    //   if (val) {
    //       sChatOption* pOption = CHATMGR->GetOption();
    //       for (int i = 0; i < CTO_COUNT; ++i)
    //           m_pBtnOption[i]->SetChecked(pOption->bOption[i]);
    //       m_bFirst   = FALSE;
    //       m_bChanged = FALSE;
    //   } else if (m_bFirst == FALSE) {
    //       if (m_bChanged) CHATMGR->SaveUserOption();
    //   }
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy CChatOptionDialog::OnActionEvent.
    //   sChatOption* pOption = CHATMGR->GetOption();
    //   if (we & WE_CHECKED) pOption->bOption[lId - kOptBase] = TRUE;
    //   else if (we & WE_NOTCHECKED) pOption->bOption[lId - kOptBase] = FALSE;
    //   m_bChanged = TRUE;
    // The modern port updates the local m_options[] in
    // addition to the host callback (so GetOption() can
    // be queried even without the host wired up).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy CChatOptionDialog::Linking.  Resolves
    // the 12 cCheckBox children via the host-injected
    // pointers.
    void Linking();

    // 1:1 with legacy sChatOption (12 bools).
    static constexpr std::size_t kChatOptionCount = 12;

    // 1:1 with legacy WindowIDEnum.h COI_CB_OPTION01..12.
    // Local 750-761 (avoid 590 / 30-32 / 70-80 / 410 / 420 /
    // 700-703 / 730 used by other recent 1:1 ports).
    static constexpr std::int32_t kIdOptionBase = 750;
    static constexpr std::int32_t kIdOptionEnd  = 761;

    // WE flags (1:1 with legacy WE_CHECKED / WE_NOTCHECKED).
    static constexpr std::uint32_t kWeChecked    = 0x0080u;
    static constexpr std::uint32_t kWeNotChecked = 0x0100u;

    // Test hook -- inject the 12 cCheckBox pointers.
    // 1:1 with legacy `cCheckBox* m_pBtnOption[12]`.
    // Modern port takes a pointer-to-pointer; callers
    // pass `&pBtnOption[0]` (or `pBtnOption` if the
    // array is decayed) to provide the 12 cCheckBox
    // pointers.  Array size is implicit via
    // `kChatOptionCount`.
    void SetCheckBoxesForTest(cCheckBox* const* boxes) noexcept;

    // Test accessors.
    bool IsOptionEnabled(std::size_t idx) const noexcept {
        return (idx < kChatOptionCount) ? m_options[idx] : false;
    }
    void SetOptionForTest(std::size_t idx, bool v) noexcept {
        if (idx < kChatOptionCount) m_options[idx] = v;
    }
    bool HasChanged() const noexcept   { return m_bChanged; }
    bool IsFirst()    const noexcept   { return m_bFirst; }

    // 1:1 with legacy CHATMGR->GetOption() / SaveUserOption().
    using GetOptionCallback  = void(*)(bool out[12], void* user);
    using SaveOptionCallback = void(*)(const bool options[12], void* user);
    void SetGetOptionCallbackForTest(GetOptionCallback cb, void* user) {
        m_getOptionCb = cb; m_getOptionUser = user;
    }
    void SetSaveOptionCallbackForTest(SaveOptionCallback cb, void* user) {
        m_saveOptionCb = cb; m_saveOptionUser = user;
    }

private:
    cCheckBox*       m_pBtnOption[kChatOptionCount] = {};
    bool             m_options[kChatOptionCount]    = {};
    bool             m_bFirst   = true;
    bool             m_bChanged = false;
    GetOptionCallback  m_getOptionCb  = nullptr;
    void*              m_getOptionUser = nullptr;
    SaveOptionCallback m_saveOptionCb  = nullptr;
    void*              m_saveOptionUser = nullptr;
};

} // namespace mxh::ui
