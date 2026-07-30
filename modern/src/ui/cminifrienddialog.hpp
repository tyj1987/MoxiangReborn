// cminifrienddialog.hpp -- modern port of Moxiang CMiniFriendDialog (mini-friend add).
//
// 1:1 port of legacy `CMiniFriendDialog` from
//   `[Client]MH\MiniFriendDialog.{h,cpp}`.
//
// The mini-friend dialog is a 4-child prompt that lets the
// player add a friend by name: a static label, an edit
// box (with VCM_CHARNAME valid-check), an OK button, and
// a cancel button.
//
// 1:1 dependencies:
//   * 1 cStatic (m_pName) -- a "name:" label
//   * 1 cEditBox (m_pNameEdit) -- the name input field
//     (valid-check VCM_CHARNAME, init text "")
//   * 2 cButton (m_pAddOkBtn / m_pAddCancelBtn)
//
// Modern port keeps the legacy surface (Linking + SetName +
// SetActive override).  The host wires up the 4 child
// pointers via SetChildrenForTest (replaces the legacy
// GetWindowForID lookups).  The VCM_CHARNAME valid-check
// on the edit box is dropped (no modern text-validation
// helper yet); the dialog does not implement OnActionEvent
// because the OK / cancel buttons are wired by the host
// via the standard cButton click flow.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;
class cEditBox;
class cStatic;

class cMiniFriendDialog : public cDialog {
public:
    cMiniFriendDialog();
    ~cMiniFriendDialog() override;

    cMiniFriendDialog(const cMiniFriendDialog&) = delete;
    cMiniFriendDialog& operator=(const cMiniFriendDialog&) = delete;

    // 1:1 with legacy Linking.  Resolves the 4 child
    // windows via the host-injected pointers.  The
    // legacy cEditBox::SetValidCheck(VCM_CHARNAME) +
    // SetEditText("") is dropped (the modern cEditBox
    // does not have a text-validation helper; the host
    // is expected to wire the validation policy in its
    // own setup step).
    void Linking();

    // 1:1 with legacy SetActive override.  When
    // activating, clears the edit box text.  When
    // m_bDisable is set, refuses to change state.
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy SetName(char*).  Pre-fills the
    // edit box with `name`.
    void SetName(const char* name);

    // Disable flag accessor (1:1 with legacy m_bDisable).
    bool IsDisabled() const noexcept { return m_bDisable; }
    void SetDisabled(bool v) noexcept { m_bDisable = v; }

    // 1:1 with legacy WindowIDs.h FRI_NAME / FRI_NAMEEDIT /
    // FRI_ADDOKBTN / FRI_ADDCANCELBTN.  Local 700-703
    // (avoid collision with 590-591 / 30-32 / 70-80 / 410
    // / 730 used by other recent 1:1 ports).
    static constexpr std::int32_t kIdName      = 700;
    static constexpr std::int32_t kIdNameEdit  = 701;
    static constexpr std::int32_t kIdAddOkBtn  = 702;
    static constexpr std::int32_t kIdAddCancelBtn = 703;

    // 1:1 with legacy valid-check enum.
    static constexpr std::int32_t kValidCheckCharName = 1;  // VCM_CHARNAME

    // Test hook -- inject the 4 child pointers.
    void SetChildrenForTest(cStatic* name, cEditBox* nameEdit,
                            cButton* addOk, cButton* addCancel) noexcept {
        m_pName = name; m_pNameEdit = nameEdit;
        m_pAddOkBtn = addOk; m_pAddCancelBtn = addCancel;
    }
    cStatic* GetNameForTest()        const noexcept { return m_pName; }
    cEditBox* GetNameEditForTest()   const noexcept { return m_pNameEdit; }
    cButton*  GetAddOkBtnForTest()   const noexcept { return m_pAddOkBtn; }
    cButton*  GetAddCancelBtnForTest() const noexcept { return m_pAddCancelBtn; }

private:
    cStatic*  m_pName         = nullptr;
    cEditBox* m_pNameEdit     = nullptr;
    cButton*  m_pAddOkBtn     = nullptr;
    cButton*  m_pAddCancelBtn = nullptr;
    bool      m_bDisable      = false;
};

} // namespace mxh::ui
