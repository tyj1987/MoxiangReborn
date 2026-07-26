// cmininotedialog.cpp — modern port of 墨香 CMiniNoteDialog (note read/write).
//
// 1:1 port of legacy `CMiniNoteDialog` from
//   `墨香【源码】\[Client]MH\MiniNoteDialog.cpp`.

#include "mxh/ui/cmininotedialog.hpp"
#include "mxh/ui/ctextarea.hpp"
#include "mxh/ui/ceditbox.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/cwindow.hpp"

#include <cstring>
#include <string>

namespace mxh::ui {

cMiniNoteDialog::cMiniNoteDialog() {
    m_CurMiniNoteMode   = -1;
    m_SelectedNoteID    = 0;
    // 1:1 with legacy ctor's m_SetitemNameTable.Initialize(10).
    // std::unordered_map doesn't need a capacity hint; reserve
    // matches the legacy "10 slots" intent.
    m_SetitemNameTable.reserve(10);
}

cMiniNoteDialog::~cMiniNoteDialog() {
    for (int n = 0; n < MiniNoteMode_Max; ++n) {
        m_MinNoteCtlListArray[n].clear();
    }
    m_SetitemNameTable.clear();
}

void cMiniNoteDialog::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                            std::uint16_t hei, void* basicImage, std::int32_t id) {
    cDialog::Init(x, y, wid, hei, basicImage, id);
    // 1:1 with legacy m_type = WT_MININOTEDLG.  Modern cDialog
    // doesn't expose a settable window-type field; the legacy
    // WT_MININOTEDLG was used only for debug / dispatch.  The
    // modern port preserves the load-sidekick: kicking off
    // LoadSetShopItemList() exactly when Init() runs.
    LoadSetShopItemList();
}

void cMiniNoteDialog::Linking() {
    // 1:1 with legacy MiniNoteDialog::Linking.  In the legacy
    // code the controls are looked up by window id (NOTE_MRTITLE
    // etc.) from the cWindowManager.  In the modern port the
    // host injects the pointers via SetChildWindowsForTest
    // (or the equivalent cWindowManager-driven path once the
    // cWindowManager integration is wired up).  After this
    // call, the per-mode control lists are populated and
    // SetActiveMiniNoteMode is ready to use.
    if (m_w.rTitle)     m_MinNoteCtlListArray[MiniNoteMode_Read].push_back(m_w.rTitle);
    if (m_w.rNoteText)  m_MinNoteCtlListArray[MiniNoteMode_Read].push_back(m_w.rNoteText);
    if (m_w.sender)     m_MinNoteCtlListArray[MiniNoteMode_Read].push_back(m_w.sender);
    if (m_w.senderStc)  m_MinNoteCtlListArray[MiniNoteMode_Read].push_back(m_w.senderStc);
    if (m_w.replayBtn)  m_MinNoteCtlListArray[MiniNoteMode_Read].push_back(m_w.replayBtn);
    if (m_w.deleteBtn)  m_MinNoteCtlListArray[MiniNoteMode_Read].push_back(m_w.deleteBtn);

    if (m_w.wTitle)        m_MinNoteCtlListArray[MiniNoteMode_Write].push_back(m_w.wTitle);
    if (m_w.wNoteText)     m_MinNoteCtlListArray[MiniNoteMode_Write].push_back(m_w.wNoteText);
    if (m_w.receiverEdit)  m_MinNoteCtlListArray[MiniNoteMode_Write].push_back(m_w.receiverEdit);
    if (m_w.sendOkBtn)     m_MinNoteCtlListArray[MiniNoteMode_Write].push_back(m_w.sendOkBtn);
    if (m_w.sendCancelBtn) m_MinNoteCtlListArray[MiniNoteMode_Write].push_back(m_w.sendCancelBtn);
    if (m_w.receiver)      m_MinNoteCtlListArray[MiniNoteMode_Write].push_back(m_w.receiver);

    // 1:1 with legacy SetEnterAllow(FALSE) on both textareas.
    // Single-line note display: pressing Enter must not insert
    // a newline.
    if (m_w.rNoteText) m_w.rNoteText->SetEnterAllow(false);
    if (m_w.wNoteText) m_w.wNoteText->SetEnterAllow(false);

    // 1:1 with legacy SetValidCheck on the receiver edit box
    // (legacy: VCM_CHARNAME -- only valid character names).
    if (m_w.receiverEdit) m_w.receiverEdit->SetValidCheck(1 /*VCM_CHARNAME*/);
    if (m_w.receiverEdit) m_w.receiverEdit->SetEditText("");
}

void cMiniNoteDialog::ShowMiniNoteMode(int mode) {
    if (m_CurMiniNoteMode == mode) return;
    if (m_CurMiniNoteMode != -1) {
        SetActiveMiniNoteMode(m_CurMiniNoteMode, false);
    }
    SetActiveMiniNoteMode(mode, true);
    m_CurMiniNoteMode = mode;
}

void cMiniNoteDialog::SetActiveMiniNoteMode(int mode, bool bActive) {
    if (mode < 0 || mode >= MiniNoteMode_Max) return;
    for (cWindow* w : m_MinNoteCtlListArray[mode]) {
        if (w) w->SetActive(bActive);
    }
}

void cMiniNoteDialog::SetMiniNote(const char* sender, const char* note,
                                   std::uint16_t itemIdx) {
    char buf[300] = {0};
    if (itemIdx > 0) {
        // 1:1 with legacy: try the in-memory item name table
        // first, fall back to nothing.  Legacy then asks
        // ITEMMGR->GetItemInfo() for an external lookup; in
        // the modern port the test or host registers the
        // needed items via AddSetShopItemForTest.
        auto it = m_SetitemNameTable.find(itemIdx);
        if (it != m_SetitemNameTable.end()) {
            // Legacy: sprintf(buf, CHATMGR->GetChatMsg(732), pItem->Name)
            // 1:1 simplified: just paste the name with a
            // fixed "Item:" prefix.
            std::strcat(buf, "Item:");
            std::strncat(buf, it->second.Name, sizeof(buf) - std::strlen(buf) - 1);
        }
    }
    if (note) {
        std::strncat(buf, note, sizeof(buf) - std::strlen(buf) - 1);
    }
    if (m_w.senderStc && sender) m_w.senderStc->SetStaticText(sender);
    if (m_w.rNoteText) {
        m_w.rNoteText->SetCaretMoveFirst(true);
        m_w.rNoteText->SetScriptText(buf);
    }
    if (m_w.receiverEdit && sender) m_w.receiverEdit->SetEditText(sender);
    if (m_w.wNoteText) {
        m_w.wNoteText->SetCaretMoveFirst(true);
        m_w.wNoteText->SetScriptText(buf);
    }
}

void cMiniNoteDialog::SetActive(bool val) noexcept {
    if (!isEnabled()) return;
    if (val) {
        // 1:1 with legacy: clear the write-mode textareas so a
        // fresh compose starts empty.
        if (m_w.wNoteText)    m_w.wNoteText->SetScriptText("");
        if (m_w.receiverEdit) m_w.receiverEdit->SetEditText("");
    } else {
        // 1:1 with legacy: drop focus on the receiver edit +
        // write-mode textarea so the IME caret doesn't linger.
        if (m_w.receiverEdit) m_w.receiverEdit->SetFocusEdit(false);
        if (m_w.wNoteText)    m_w.wNoteText->SetFocusEdit(false);
    }
    cDialog::SetActive(val);
}

const char* cMiniNoteDialog::GetSenderName() const noexcept {
    return m_w.senderStc ? m_w.senderStc->GetStaticText().c_str() : "";
}

const std::string& cMiniNoteDialog::ReadText() const noexcept {
    static const std::string kEmpty;
    return m_w.rNoteText ? m_w.rNoteText->GetScriptText() : kEmpty;
}

const std::string& cMiniNoteDialog::WriteText() const noexcept {
    static const std::string kEmpty;
    return m_w.wNoteText ? m_w.wNoteText->GetScriptText() : kEmpty;
}

const std::string& cMiniNoteDialog::ReceiverEditText() const noexcept {
    static const std::string kEmpty;
    return m_w.receiverEdit ? m_w.receiverEdit->editText() : kEmpty;
}

const std::string& cMiniNoteDialog::SenderStaticText() const noexcept {
    static const std::string kEmpty;
    return m_w.senderStc ? m_w.senderStc->GetStaticText() : kEmpty;
}

void cMiniNoteDialog::LoadSetShopItemList() {
    // 1:1 with legacy LoadSetShopItemList.  The legacy code
    // opens "./Image/Itemidx_Setitem.bin" via CMHFile and
    // reads:
    //   WORD Count;
    //   for (i = 0; i < Count; ++i) {
    //       SETSHOPITEM* pItem = new SETSHOPITEM;
    //       pItem->ItemIdx = file.GetDword();
    //       SafeStrCpy(pItem->Name, file.GetString(), MAX_NAME_LENGTH+1);
    //       m_SetitemNameTable.Add(pItem, pItem->ItemIdx);
    //   }
    // The modern port defers the actual file load (CMHFile
    // / cImeEx / ItemManager aren't all wired up yet); the
    // test or host injects the items via
    // AddSetShopItemForTest.  We still call reserve() to
    // match the legacy 10-slot hint in the ctor.
    m_SetitemNameTable.clear();
    m_SetitemNameTable.reserve(10);
}

void cMiniNoteDialog::AddSetShopItemForTest(std::uint32_t itemIdx,
                                              const char* name) {
    SetShopItem item;
    item.ItemIdx = itemIdx;
    if (name) {
        std::strncpy(item.Name, name, sizeof(item.Name) - 1);
        item.Name[sizeof(item.Name) - 1] = '\0';
    }
    m_SetitemNameTable[itemIdx] = item;
}

} // namespace mxh::ui
