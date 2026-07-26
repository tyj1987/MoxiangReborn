// cnotedialog.hpp — modern port of 墨香 CNoteDialog (note inbox).
//
// 1:1 port of legacy `CNoteDialog` from
//   `墨香【源码】\[Client]MH\NoteDialog.{h,cpp}`.
//
// The legacy note inbox is a tabbed dialog (Normal / PS tabs)
// with a cListCtrl of received notes + per-row cCheckBox +
// page buttons + a "compose new" / "delete" button pair.
//
// Modern port keeps the 1:1 surface:
//   * SetMode(NormalNote / PsNote) toggles the tab pushup buttons
//   * SetActive(true) -> SetPushBarIcon(OPT_NOTEDLGICON) +
//                         SetAlram(OPT_NOTEDLGICON, FALSE) +
//                         m_pNoteChkAll->SetChecked(FALSE)
//   * SetActive(false) -> SetMode(NormalNote) (reset on close)
//   * SetNoteList populates cListCtrl rows + per-row checkboxes
//   * ShowNotePageBtn enables per-page buttons based on TotalPage
//   * SetChkAll toggles every active per-row checkbox
//   * SetSelectedNotePge / GetSelectedNotePge + Get/SetCurNoteID
//
// The dialog keeps:
//   * m_SelectedNotePge   -- which page (1..MAX_NOTE_PAGE) is open
//   * m_CurNoteMode       -- NormalNote / PsNote tab
//   * m_CurNoteID         -- the note ID the user picked (network
//                            layer uses this for read/delete)
//
// Child windows are injected via SetChildWindowsForTest (the
// legacy cWindowManager lookup is replaced by direct pointer
// injection so unit tests don't need the full window-id tree).

#pragma once

#include "cDialog.hpp"
#include "ccheckbox.hpp"
#include "cbutton.hpp"
#include "cpushupbutton.hpp"
#include "clistctrl.hpp"

#include <cstdint>
#include <vector>

namespace mxh::ui {

// Legacy: 1:1 with eNoteMode in NoteDialog.h.
enum NoteMode : std::uint16_t {
    NoteMode_NormalNote = 0,
    NoteMode_PsNote    = 1,
};

// Legacy: 1:1 with NOTENUM_PER_PAGE / MAX_NOTE_PAGE constants.
inline constexpr std::int32_t kNoteNumPerPage = 10;
inline constexpr std::int32_t kMaxNotePage   = 5;

class cNoteDialog : public cDialog {
public:
    cNoteDialog();
    ~cNoteDialog() override;

    cNoteDialog(const cNoteDialog&) = delete;
    cNoteDialog& operator=(const cNoteDialog&) = delete;

    // 1:1 with legacy Init.  Stores position/size + basic image
    // via cDialog::Init.  The legacy m_type = WT_NOTEDLG is
    // not modeled in the modern port (no equivalent window-type
    // field).
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id = 0);

    // 1:1 with legacy Linking.  Modern port reads injected
    // child pointers from m_w (the host called
    // SetChildWindowsForTest before Linking).
    void Linking();

    // 1:1 with legacy SetActive.  Adds the modern equivalent
    // of the main-bar icon push + alarm clear by exposing a
    // callback (the host wires it to CMainBarDialog::SetPushBarIcon
    // + SetAlram when the host integration is in place).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy SetMode.  Toggles the two pushup tab
    // buttons (m_pNoteBtn / m_pPsNoteBtn) based on the mode.
    // The legacy TAIWAN_LOCAL branch (#ifndef TAIWAN_LOCAL)
    // skips the pushup toggle on the Taiwan client; the modern
    // port keeps the same conditional.
    void SetMode(std::uint16_t mode) noexcept;

    // 1:1 with legacy SetNoteList.  Populates the per-row
    // cListCtrl + per-row cCheckBox.  In the modern port the
    // network-layer MSG_FRIEND_NOTE_LIST struct isn't loaded;
    // the host adapter calls SetNoteListFromFields with the
    // raw fields it parsed from the wire.
    struct NoteListRow {
        std::uint32_t NoteID    = 0;
        char          FromName[13] = {};   // 12 + NUL, truncated
        char          SendDate[20] = {};
        bool          bIsRead    = false;
    };
    void SetNoteListFromFields(const std::vector<NoteListRow>& rows,
                               std::uint8_t totalPage);

    // 1:1 with legacy ShowNotePageBtn.  Enables m_pNotePageBtn[i]
    // for i in [0, totalPage) and disables the rest.
    void ShowNotePageBtn(std::uint8_t totalPage);

    // 1:1 with legacy SetChkAll.  If m_pNoteChkAll is checked,
    // check every active per-row checkbox; if unchecked, uncheck.
    void SetChkAll();

    // 1:1 with legacy Get/SetSelectedNotePge + Get/SetCurNoteID
    // + Get/SetMode.
    std::uint16_t GetSelectedNotePge() const noexcept { return m_SelectedNotePge; }
    void         SetSelectedNotePge(std::uint16_t p) noexcept { m_SelectedNotePge = p; }
    std::uint32_t GetCurNoteID() const noexcept          { return m_CurNoteID; }
    void         SetCurNoteID(std::uint32_t id) noexcept { m_CurNoteID = id; }
    std::uint16_t GetMode() const noexcept              { return m_CurNoteMode; }

    // Test hook -- inject child windows (mirrors the
    // cMiniNoteDialog / cNumberPadDialog pattern).
    struct ChildWindows {
        cButton*       writeNoteBtn   = nullptr;   // NOTE_WRITENOTEBTN
        cButton*       delNoteBtn     = nullptr;   // NOTE_DELNOTEBTN
        cPushupButton* noteBtn        = nullptr;   // NOTE_TABBTN1
        cPushupButton* psNoteBtn      = nullptr;   // NOTE_TABBTN2
        cListCtrl*     noteListLCtrl  = nullptr;   // NOTE_NOTELISTLCTL
        cCheckBox*     noteChk[kNoteNumPerPage] = {};
        cCheckBox*     noteChkAll     = nullptr;   // NOTE_NOTELISTCHK11
        cButton*       notePageBtn[kMaxNotePage] = {};
    };
    void SetChildWindowsForTest(const ChildWindows& w) noexcept { m_w = w; }

    // Test accessors.
    std::size_t NoteListSize() const noexcept;
    std::uint16_t RowCount() const noexcept {
        return static_cast<std::uint16_t>(m_lastRows.size());
    }

    // Modern SetActive callback -- fired when the active state
    // changes.  Replaces the legacy CMainBarDialog::SetPushBarIcon
    // + SetAlram side-effect.  Argument is the new active state.
    using ActiveChangedCallback = std::function<void(bool active)>;
    void SetOnActiveChanged(ActiveChangedCallback cb) noexcept {
        m_onActiveChanged = std::move(cb);
    }

private:
    // 1:1 with legacy cRITEMEx insertion: 1:1 cRITEMEx* per row.
    // Modern port keeps the ritem object alive for the lifetime
    // of the cListCtrl row (legacy cListCtrl takes ownership).
    struct StoredRItem {
        std::uint32_t noteID    = 0;
        bool          bIsRead  = false;
        // Display text (truncated FromName + SendDate, legacy
        // pString[0] / pString[1]).
        char          p0[13] = {};
        char          p1[20] = {};
    };
    std::vector<StoredRItem> m_lastRows;

    std::uint16_t m_SelectedNotePge = 1;
    std::uint16_t m_CurNoteMode     = NoteMode_NormalNote;
    std::uint32_t m_CurNoteID       = 0;

    ChildWindows m_w{};
    ActiveChangedCallback m_onActiveChanged;
};

} // namespace mxh::ui
