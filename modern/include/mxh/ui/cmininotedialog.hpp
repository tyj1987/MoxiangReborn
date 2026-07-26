// cmininotedialog.hpp — modern port of 墨香 CMiniNoteDialog (note read/write).
//
// 1:1 port of legacy `CMiniNoteDialog` from
//   `墨香【源码】\[Client]MH\MiniNoteDialog.{h,cpp}`.
//
// The legacy dialog is a 2-mode note pad:
//   - eMinNoteMode_Read  :  read a note (sender + body + reply/delete buttons)
//   - eMinNoteMode_Write :  compose a new note (receiver + body + send/cancel)
//
// Each mode has its own sub-control set; cPtrList holds the
// per-mode list so SetActiveMiniNoteMode can flip the active
// flag on all controls belonging to a mode in one pass.
//
// The dialog also keeps:
//   * m_SetitemNameTable  -- a CYHHashTable<SETSHOPITEM> for resolving
//                             item display names when a note is
//                             attached to a shop item
//   * m_SelectedNoteID    -- the note ID the user picked (network layer
//                             needs this to know which note was replied to)

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>

namespace mxh::ui {

// Legacy: 1:1 with eMiniNote_Mode in MiniNoteDialog.h.
enum MiniNoteMode : std::int32_t {
    MiniNoteMode_Read  = 0,
    MiniNoteMode_Write = 1,
    MiniNoteMode_Max   = 2,
};

// Minimal stand-in for the legacy SETSHOPITEM struct (the legacy
// SETSHOPITEM lives in `ItemManager.h` / `GameResourceStruct.h` and
// has many fields; the modern port only needs the two fields the
// dialog actually reads).
struct SetShopItem {
    std::uint32_t ItemIdx = 0;
    char          Name[17] = {};   // MAX_NAME_LENGTH+1, NUL-padded
};

// 1:1 with legacy cPtrList<eMinNoteMode_Max> -- a fixed-size array
// of per-mode control lists.  Modern port uses
// std::vector<cWindow*> since the host calls cDialog::Add to
// register child windows.  Empty vectors are valid; SetActiveMiniNoteMode
// iterates the vector and toggles the active flag on each.
class cWindow;
class cStatic;
class cTextArea;
class cButton;
class cEditBox;

class cMiniNoteDialog : public cDialog {
public:
    cMiniNoteDialog();
    ~cMiniNoteDialog() override;

    cMiniNoteDialog(const cMiniNoteDialog&) = delete;
    cMiniNoteDialog& operator=(const cMiniNoteDialog&) = delete;

    // 1:1 with legacy Init: stores position/size, sets
    // WT_MININOTEDLG window type, and kicks off
    // LoadSetShopItemList().  The resource script (cImage*
    // basicImage) is wired through cDialog::Init.
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id = 0);

    // 1:1 with legacy Linking: looks up the read/write sub-
    // controls by window id and adds them to the appropriate
    // per-mode list.  In the modern port the window-id lookup
    // is mocked: the host calls SetChildWindowsForTest to
    // inject the pointers (mirrors the cNumberPadDialog
    // SetStaticsForTest pattern from Phase C Batch 1.1).
    void Linking();

    // 1:1 with legacy ShowMiniNoteMode: switches active
    // sub-control set.  Idempotent: a no-op if the requested
    // mode equals the current mode.
    void ShowMiniNoteMode(int mode);

    // 1:1 with legacy SetMode: just records the current mode
    // (used when the dialog is being opened from a list).
    void SetMode(int mode) noexcept { m_CurMiniNoteMode = mode; }

    // 1:1 with legacy SetActiveMiniNoteMode: flip the active
    // flag on every cWindow in the per-mode list.  Recreates
    // the legacy PTRLISTSEARCHSTART / PTRLISTSEARCHEND macro.
    void SetActiveMiniNoteMode(int mode, bool bActive);

    // 1:1 with legacy SetMiniNote.  Combines the item-name
    // prefix (looked up in m_SetitemNameTable by ItemIdx)
    // with the note body, writes the result to BOTH the
    // read-mode textarea and the write-mode textarea, and
    // copies the sender name into the receiver edit box
    // (legacy behaviour: opening a note pre-fills the
    // reply-receiver with the original sender).
    void SetMiniNote(const char* sender, const char* note, std::uint16_t itemIdx);

    // 1:1 with legacy SetActive: clears the write-mode
    // textarea + receiver edit when activated; clears focus
    // when deactivated; forwards to cDialog::SetActive.
    void SetActive(bool val) noexcept override;

    // Accessors.
    int          GetCurMode() const noexcept         { return m_CurMiniNoteMode; }
    std::uint32_t GetNoteID() const noexcept         { return m_SelectedNoteID; }
    void         SetNoteID(std::uint32_t id) noexcept { m_SelectedNoteID = id; }
    const char*  GetSenderName() const noexcept;  // returns "" when sender stc is null

    // 1:1 with legacy LoadSetShopItemList: reads the
    // Itemidx_Setitem.bin / Itemidx_Setitem.txt resource and
    // populates m_SetitemNameTable.  In the modern port the
    // actual file load is deferred (the resource loader
    // doesn't exist yet); tests inject items via
    // AddSetShopItemForTest.
    void LoadSetShopItemList();

    // Test hook -- inject a SETSHOPITEM entry by id.  The
    // host (or a test) registers the items that the legacy
    // .bin loader would have populated.
    void AddSetShopItemForTest(std::uint32_t itemIdx, const char* name);

    // Test hook -- inject the read/write sub-control windows.
    // Accepts nullptr for any pointer the test does not need.
    // After this call, SetActiveMiniNoteMode works against the
    // injected pointers (no window-id lookup required).
    struct ChildWindows {
        cStatic*  rTitle      = nullptr;   // NOTE_MRTITLE
        cStatic*  wTitle      = nullptr;   // NOTE_MWTITLE
        cTextArea* rNoteText  = nullptr;   // NOTE_MRNOTETEXTREA
        cTextArea* wNoteText  = nullptr;   // NOTE_MWNOTETEXTREA
        cStatic*  sender      = nullptr;   // NOTE_MSENDER
        cStatic*  senderStc   = nullptr;   // NOTE_MSENDERSTC
        cButton*  replayBtn   = nullptr;   // NOTE_MREPLYBTN
        cButton*  deleteBtn   = nullptr;   // NOTE_MDELETEBTN
        cEditBox* receiverEdit = nullptr;  // NOTE_MRECEIVEREDIT
        cStatic*  receiver    = nullptr;   // NOTE_MRECEIVER
        cButton*  sendOkBtn   = nullptr;   // NOTE_MSENDOKBTN
        cButton*  sendCancelBtn = nullptr; // NOTE_MSENDCANCELBTN
    };
    void SetChildWindowsForTest(const ChildWindows& w) noexcept { m_w = w; }

    // Test accessors.
    const std::string& ReadText() const noexcept;
    const std::string& WriteText() const noexcept;
    const std::string& ReceiverEditText() const noexcept;
    const std::string& SenderStaticText() const noexcept;
    std::size_t        SetShopItemCount() const noexcept { return m_SetitemNameTable.size(); }

private:
    // 1:1 with legacy cPtrList m_MinNoteCtlListArray[eMinNoteMode_Max].
    // Per-mode list of cWindow* (legacy was cPtrList which is
    // a doubly-linked list; std::vector is the modern equivalent).
    std::vector<cWindow*> m_MinNoteCtlListArray[MiniNoteMode_Max];

    int m_CurMiniNoteMode = -1;
    std::uint32_t m_SelectedNoteID = 0;

    // 1:1 with legacy CYHHashTable<SETSHOPITEM> m_SetitemNameTable.
    // Modern port uses std::unordered_map for the same surface.
    std::unordered_map<std::uint32_t, SetShopItem> m_SetitemNameTable;

    ChildWindows m_w{};
};

} // namespace mxh::ui
