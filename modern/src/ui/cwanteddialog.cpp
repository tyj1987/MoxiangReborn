// cwanteddialog.cpp -- modern implementation of Moxiang CWantedDialog.

#include "cwanteddialog.hpp"

#include "clistdialog.hpp"

#include <cstdio>
#include <cstring>
#include <utility>

namespace mxh::ui {

cWantedDialog::cWantedDialog() = default;

cWantedDialog::~cWantedDialog() = default;

void cWantedDialog::Linking() {
    // 1:1 with legacy Linking.  The legacy uses
    //   m_pWantedLDG = (cListDialog*)GetWindowForID(QUE_WANTEDLDLG);
    // The modern port lets the host inject the cListDialog
    // pointer via SetListDialogForTest (called before
    // Linking).  When the pointer is already populated, this
    // function is a no-op (preserves the legacy "link only
    // happens once" semantics).
}

void cWantedDialog::SetInfo(const WantedListEntry* pInfo) {
    if (!pInfo || !m_pWantedLDG) return;
    InitWanted();
    const ChatMsgCallback cb = m_chatMsgCb ? m_chatMsgCb : &cWantedDialog::DefaultChatMsg;
    for (int i = 0; i < kMaxWantedNum; ++i) {
        if (pInfo[i].WantedIDX == 0) {
            // 1:1 with legacy `if(pInfo[i].WantedIDX == 0) break;`.
            break;
        }
        m_pWantedLDG->AddItem(pInfo[i].RegistDate, 0xffffffffu);
        char temp[128] = {0};
        // 1:1 with legacy sprintf(temp, CHATMGR->GetChatMsg(545),
        // pInfo[i].WantedName).  The host-injected chatmsg
        // callback supplies the format string; the default is
        // "%s".
        std::snprintf(temp, sizeof(temp), cb(kChatMsgWantedName, m_chatMsgUser),
                     pInfo[i].WantedName);
        m_pWantedLDG->AddItem(temp, 0xffffffffu);
    }
    // 1:1 with legacy `m_pWantedLDG->ResetGuageBarPos()`.  The
    // modern cListDialog does not have a scroll-gauge bar
    // (the bar is part of the legacy GUAGEN child window);
    // scroll the list back to the top to match the legacy
    // visual.
    m_pWantedLDG->SetTopListItemIdx(0);
}

void cWantedDialog::AddInfo(const WantedListEntry* pInfo) {
    if (!pInfo || !m_pWantedLDG) return;
    const ChatMsgCallback cb = m_chatMsgCb ? m_chatMsgCb : &cWantedDialog::DefaultChatMsg;
    m_pWantedLDG->AddItem(pInfo->RegistDate, 0xffffffffu);
    char temp[128] = {0};
    // 1:1 with legacy sprintf(temp, CHATMGR->GetChatMsg(545),
    // pInfo->WantedName).
    std::snprintf(temp, sizeof(temp), cb(kChatMsgWantedName, m_chatMsgUser),
                 pInfo->WantedName);
    m_pWantedLDG->AddItem(temp, 0xffffffffu);
}

void cWantedDialog::InitWanted() {
    if (!m_pWantedLDG) return;
    m_pWantedLDG->RemoveAll();
}

const char* cWantedDialog::DefaultChatMsg(int /*chatMsgId*/, void* /*user*/) {
    // 1:1 with the legacy default for chatmsg 545 -- the
    // .bin chatmsg table ships "%s" as the format string
    // (the name placeholder is filled at sprintf time).
    return "%s";
}

} // namespace mxh::ui
