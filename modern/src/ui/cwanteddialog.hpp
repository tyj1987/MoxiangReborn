// cwanteddialog.hpp -- modern port of Moxiang CWantedDialog (wanted list).
//
// 1:1 port of legacy `CWantedDialog` from
//   `[Client]MH\WantedDialog.{h,cpp}`.
//
// The wanted dialog is a vertical list (cListDialog) of wanted
// targets, each row showing RegistDate + a CHATMGR-formatted
// "<name>" line.  The list is populated either wholesale via
// SetInfo (initial server snapshot, MAX_WANTED_NUM rows) or
// incrementally via AddInfo (single-row append from the
// broadcast stream).
//
// 1:1 dependencies:
//   * cListDialog for the row list
//   * CHATMGR->GetChatMsg(545) for the wanted-name format
//     (modern port routes this through a host-injected
//     ChatMsgCallback; the default format is "%s")
//   * cListDialog::ResetGuageBarPos() at the end of SetInfo
//     (modern port: no-op, cListDialog does not have a
//     scroll-gauge bar in the modern port; the legacy
//     behaviour is preserved by calling m_pWantedLDG->
//     SetTopListItemIdx(0) at the end of SetInfo)
//
// Modern port keeps the legacy surface (Linking / SetInfo /
// AddInfo / InitWanted) so callers can be ported 1:1.  The
// host wires up the cListDialog via Linking(); the host
// calls SetInfo when MP_WANTED's full-list snapshot arrives
// and AddInfo for each incremental registration.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cListDialog;

// 1:1 with legacy [CC]Header/CommonGameDefine.h constants used
// by the wanted dialog.
inline constexpr std::int32_t kMaxWantedNum   = 20;
inline constexpr std::int32_t kMaxNameLength  = 16;
inline constexpr std::int32_t kRegistDateSize = 11;

// 1:1 with legacy WANTEDLIST struct in [CC]Header/CommonStruct.h
// (lines 3860-3866).  The legacy struct is a network message
// payload and the field layout / order is part of the wire
// format.  The modern port keeps a 1:1 copy (with the same
// field order + sizes) instead of including the legacy
// [CC]Header/CommonStruct.h directly (the legacy header is
// pulled into a C++ translation unit only via the modern
// wire-format test harness, not into UI dialog code).
struct WantedListEntry {
    std::uint32_t  WantedIDX   = 0;                            // 1:1 with legacy WANTEDTYPE WantedIDX
    std::uint32_t  WantedChrID = 0;                            // 1:1 with legacy DWORD WantedChrID
    char           WantedName[kMaxNameLength + 1] = {};       // 1:1 with legacy char WantedName[MAX_NAME_LENGTH+1]
    char           RegistDate[kRegistDateSize]      = {};      // 1:1 with legacy char RegistDate[11]
};

class cWantedDialog : public cDialog {
public:
    cWantedDialog();
    ~cWantedDialog() override;

    cWantedDialog(const cWantedDialog&) = delete;
    cWantedDialog& operator=(const cWantedDialog&) = delete;

    // 1:1 with legacy Linking.  Resolves m_pWantedLDG via
    // the host-injected pointer; the legacy GetWindowForID
    // (QUE_WANTEDLDLG) call is replaced by a test/host
    // injection (cListDialog* pointer).
    void Linking();

    // 1:1 with legacy SetInfo(WANTEDLIST*).  Resets the
    // list and re-fills it from the snapshot array, up to
    // MAX_WANTED_NUM rows.  Stops on the first row with
    // WantedIDX == 0 (legacy sentinel).
    void SetInfo(const WantedListEntry* pInfo);

    // 1:1 with legacy AddInfo(WANTEDLIST*).  Appends a
    // single row to the list (no sentinel check).
    void AddInfo(const WantedListEntry* pInfo);

    // 1:1 with legacy InitWanted.  Clears the list.
    void InitWanted();

    // 1:1 chatmsg id used by the wanted dialog.
    static constexpr int kChatMsgWantedName = 545;

    // Test hook -- inject a "chatmsg lookup" callback.  The
    // dialog uses chatmsg 545 to format the name line
    // (legacy: sprintf(temp, CHATMGR->GetChatMsg(545),
    // pInfo[i].WantedName)).  Default callback returns
    // "%s" (matches the legacy default in the .bin chatmsg
    // table).
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) {
        m_chatMsgCb = cb; m_chatMsgUser = user;
    }

    // Test hook -- inject the cListDialog pointer (replaces
    // the legacy GetWindowForID(QUE_WANTEDLDLG) lookup).
    void SetListDialogForTest(cListDialog* list) noexcept { m_pWantedLDG = list; }
    cListDialog* GetListDialogForTest() const noexcept     { return m_pWantedLDG; }

private:
    cListDialog*    m_pWantedLDG = nullptr;
    ChatMsgCallback m_chatMsgCb  = nullptr;
    void*           m_chatMsgUser = nullptr;

    // Default chatmsg lookup -- returns "%s" so the modern
    // port matches the legacy default behaviour without
    // having to load the chatmsg table.
    static const char* DefaultChatMsg(int /*chatMsgId*/, void* /*user*/);
};

} // namespace mxh::ui
