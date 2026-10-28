// cpartywardialog.hpp -- modern port of Moxiang CPartyWarDialog (clan-war challenge).
//
// 1:1 port of legacy `CPartyWarDialog` from
//   `[Client]MH\PartyWarDialog.{h,cpp}`.
//
// The party-war dialog is a 14-slot two-team PvP challenge setup.  Each
// team holds 7 slots (1 master + 6 members).  Legacy behaviour:
//
//   * ShowPartyWarDlg(BOOL bMaster)   -- enable Lock/UnLock/Start/Cancel
//                                        buttons (bMaster), clear 14 checkboxes
//                                        + 14 name statics
//   * HidePartyWarDlg()                -- SetActive(FALSE), reset both locks
//   * NoChangeCheckBoxState(int)       -- toggle checkbox, return previous
//   * SetPartyMemberName(pName, idx, bLogged) -- assign member, refresh
//                                        team1/team2 + recompose title via
//                                        CHATMGR->GetChatMsg(867)
//   * AddPartyWarMember/RemovePartyWarMember(int)
//   * SetLock(bParty, bMaster)         -- mark party or enemy locked; if
//                                        bMaster and both locked -> enable
//                                        Start, else enable Cancel
//   * SetUnLock(bMaster)               -- both locks=false, enable Cancel
//   * SetTime(dwTime)                  -- CHATMGR->GetChatMsg(868) -> "%d:%02d"
//
// 1:1 dependencies:
//   * cDialog base
//   * cButton for Lock / UnLock / Start / Cancel
//   * cCheckBox x 14 (one per slot, master-only toggle)
//   * cStatic x 14 (member name), x 3 (team1 / team2 / title), x 1 (time)
//   * cTextArea x 1 (sprocket/rolling text)
//
// Modern port keeps the legacy surface (ShowPartyWarDlg / HidePartyWarDlg /
// NoChangeCheckBoxState / SetPartyMemberName / AddPartyWarMember /
// RemovePartyWarMember / SetLock / SetUnLock / SetTime) so callers can be
// ported 1:1.  The host wires up the child-window pointers via Linking();
// the host calls each dialog method directly when the legacy network msg
// arrives.  CHATMGR->GetChatMsg(867 / 868) is dropped (no modern chat
// manager yet) -- the modern port uses a plain snprintf "%s VS %s" for the
// title and "%d:%02d" for the time, both of which match the legacy
// format strings in the default resource file.

#pragma once

#include "cDialog.hpp"

#include <array>
#include <cstdint>

namespace mxh::ui {

class cButton;
class cCheckBox;
class cStatic;
class cTextArea;

class cPartyWarDialog : public cDialog {
public:
    // Legacy: 14 members, 7 per team.
    static constexpr std::size_t kMemberCount = 14;
    static constexpr std::size_t kTeamSize    = 7;

    // 1:1 with the legacy CPartyWarDlg members (m_pcbName, m_psName,
    // m_pTeam1, m_pTeam2, m_pSprocket, m_pNameStatic, m_pbtnLock,
    // m_pbtnUnLock, m_pbtnStart, m_pbtnCancel, m_pTimeStatic,
    // m_pImageLock).  Modern port packs them into a single ChildWindows
    // struct so the test can wire all 14+ child pointers in one shot.
    struct ChildWindows {
        std::array<cCheckBox*, kMemberCount> memberCheck  = {};
        std::array<cStatic*,   kMemberCount> memberStatic = {};
        cTextArea* team1   = nullptr;
        cTextArea* team2   = nullptr;
        cTextArea* title   = nullptr;
        cTextArea* time    = nullptr;
        cTextArea* sprocket = nullptr;
        cButton*   btnLock   = nullptr;
        cButton*   btnUnLock = nullptr;
        cButton*   btnStart  = nullptr;
        cButton*   btnCancel = nullptr;
    };

    cPartyWarDialog();
    ~cPartyWarDialog() override;

    cPartyWarDialog(const cPartyWarDialog&) = delete;
    cPartyWarDialog& operator=(const cPartyWarDialog&) = delete;

    // 1:1 with legacy Init().  Stores position/size; the host attaches
    // the basic image via cDialog::Init.
    void Init(std::int32_t x, std::int32_t y,
              std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id);

    // 1:1 with legacy Render().  Modern port is a no-op for the lock
    // sprite (no script manager yet); the lock state is tracked via
    // m_bPartyLock + m_bEnemyLock so callers can still inspect it.
    void Render() override;

    // 1:1 with legacy Linking().  Resolves all 14+ child windows via
    // the host-injected ChildWindows struct.  The legacy
    // SCRIPTMGR->GetImage(65, &m_LockImage, PFT_HARDPATH) call is
    // dropped (no modern script manager) and the m_psName[i]->SetAlign
    // loop is dropped (cStatic is single-line by default).
    void Linking();

    // 1:1 with legacy ShowPartyWarDlg(BOOL).
    void ShowPartyWarDlg(bool bMaster);

    // 1:1 with legacy HidePartyWarDlg().
    void HidePartyWarDlg();

    // 1:1 with legacy NoChangeCheckBoxState(int).  Toggles the slot's
    // checkbox, returns the previous state.  Out-of-range index is a
    // no-op returning false (legacy asserts in debug, returns FALSE
    // undefined in release).
    bool NoChangeCheckBoxState(int nIndex);

    // 1:1 with legacy SetPartyMemberName(char*, int, BOOL).  When
    // nIndex==0 or nIndex==7, also updates m_pTeam1 / m_pTeam2 and
    // recomposes the title.  bLogged is dropped (no global RGBA
    // constant yet) -- the FG color is left default.
    void SetPartyMemberName(const char* pName, int nIndex, bool bLogged);

    // 1:1 with legacy AddPartyWarMember(int).  Checkbox -> true.
    void AddPartyWarMember(int nIndex);

    // 1:1 with legacy RemovePartyWarMember(int).  Checkbox -> false.
    void RemovePartyWarMember(int nIndex);

    // 1:1 with legacy SetLock(BOOL bParty, BOOL bMaster).  If both
    // bParty and bEnemy are locked and bMaster, enable Start; else
    // enable Cancel (only if bMaster).
    void SetLock(bool bParty, bool bMaster);

    // 1:1 with legacy SetUnLock(BOOL bMaster).  Both locks=false,
    // Cancel enabled if bMaster.
    void SetUnLock(bool bMaster);

    // 1:1 with legacy SetTime(DWORD).  Formats "%d:%02d" (min:sec).
    void SetTime(std::uint32_t dwTime);

    // Test hooks.
    void SetChildWindowsForTest(const ChildWindows& w) noexcept { m_w = w; }
    const ChildWindows& GetChildWindowsForTest() const noexcept { return m_w; }
    bool IsPartyLocked() const noexcept { return m_bPartyLock; }
    bool IsEnemyLocked() const noexcept { return m_bEnemyLock; }
    bool IsStartButtonActive() const noexcept;
    bool IsCancelButtonActive() const noexcept;

private:
    void recomposeTitleFromTeams();

    ChildWindows m_w;
    bool m_bPartyLock = false;
    bool m_bEnemyLock = false;
};

} // namespace mxh::ui
