// shoutchatdialog.cpp — 1:1 port of 墨香
// CShoutchatDialog (shout chat log dialog). See
// shoutchatdialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "shoutchatdialog.hpp"
#include "clistdialog.hpp"

#include <cstring>

namespace mxh::ui {

cShoutchatDialog::cShoutchatDialog() {
    // 1:1 with legacy CShoutchatDialog ctor:
    //   m_type = WT_SHOUTCHAT_DLG;
    //   m_LastMsgTime = 0;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6). The
    // ctor body is dropped. m_LastMsgTime uses
    // default member init (= 0 in header).
}

cShoutchatDialog::~cShoutchatDialog() = default;

void cShoutchatDialog::SetCurrentTimeProvider(
    ScClockFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cShoutchatDialog::Process() {
    // 1:1 with legacy CShoutchatDialog::Process. The
    // legacy is:
    //   if (gCurTime - m_LastMsgTime < 5000) return;
    //   m_LastMsgTime = gCurTime;
    //
    // The modern port: the gCurTime-based 5 sec
    // throttle is REAL via OPTIONAL host clock provider.
    // A null provider falls through to the legacy no-op
    // behavior (caller can call without restriction).
    if (!m_getCurrentTimeFn) return;
    const std::uint32_t curTime = m_getCurrentTimeFn(m_clockUserData);
    if (curTime - m_LastMsgTime < kMsgThrottleMs) return;
    m_LastMsgTime = curTime;
}

void cShoutchatDialog::Linking() {
    // 1:1 with legacy CShoutchatDialog::Linking. The
    // legacy is:
    //   m_pMsgListDlg = (cListDialog*)GetWindowForID(CHA_LIST);
    //   if (GAMERESRCMNGR->IsLowResolution())
    //     GAMEIN->GetShoutchatDlg()->RefreshPosition();
    m_pMsgListDlg =
        static_cast<cListDialog*>(findWindowById(kIdMsgList));
    // TODO: 1:1 with legacy GAMERESRCMNGR + GAMEIN
    //       dispatch (R-12.x deferred).
}

void cShoutchatDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CShoutchatDialog::SetActive
    // override. The legacy is:
    //   if (val) RefreshPosition();
    //   cDialog::SetActive(val);
    //
    // The modern port: RefreshPosition is TODO
    // (GAMEIN + cChatDialog singletons, R-12.x
    // deferred). Modern port always base SetActive.
    // TODO: 1:1 with legacy RefreshPosition call
    //       (R-12.x deferred).
    cDialog::SetActive(val);
}

void cShoutchatDialog::AddMsg(const char* pstr) {
    // 1:1 with legacy CShoutchatDialog::AddMsg. The
    // legacy is:
    //   char buf[61] = { 0, };
    //   strncpy(buf, pstr, 60);
    //   if (m_pMsgListDlg)
    //     m_pMsgListDlg->AddItem(buf, RGBA_MAKE(217, 206, 247, 255));
    //
    // The modern port: defensive null check (pstr +
    // m_pMsgListDlg) + strncpy + AddItem with
    // kShoutchatItemColor.
    if (!m_pMsgListDlg) return;
    if (!pstr) return;
    char buf[kMaxMsgLen + 1] = {0,};
    std::strncpy(buf, pstr, kMaxMsgLen);
    m_pMsgListDlg->AddItem(buf, kShoutchatItemColor);
}

}  // namespace mxh::ui
