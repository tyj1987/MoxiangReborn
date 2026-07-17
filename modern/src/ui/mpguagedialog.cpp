// mpguagedialog.cpp — 1:1 port of 墨香
// CMPGuageDialog (event-map timer + experience
// gauge). See mpguagedialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "mpguagedialog.hpp"
#include "cstatic.hpp"

#include <cstdio>
#include <string>

namespace mxh::ui {

cMPGuageDialog::cMPGuageDialog() {
    // 1:1 with legacy CMPGuageDialog ctor:
    //   m_type = WT_MPGUAGEDLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field. The ctor body is dropped.
}

cMPGuageDialog::~cMPGuageDialog() = default;

void cMPGuageDialog::Linking() {
    // 1:1 with legacy CMPGuageDialog::Linking.
    // The legacy is:
    //   m_ExpGuage = (CObjectGuagen*)GetWindowForID(MP_GEXPGUAGE);
    //   m_Time = (cStatic*)GetWindowForID(MP_GTIME);
    //   m_ExpPercent = (cStatic*)GetWindowForID(MP_GEXPERCENT);
    //   m_pTitle = (cStatic*)GetWindowForID(MP_TITLE);
    //
    // The modern port: CObjectGuagen is forward-
    // declared and stored as void* (untyped).
    m_ExpGuage = findWindowById(kIdExpGuage);
    m_Time = static_cast<cStatic*>(findWindowById(kIdTime));
    m_ExpPercent =
        static_cast<cStatic*>(findWindowById(kIdExpPercent));
    m_pTitle = static_cast<cStatic*>(findWindowById(kIdTitle));
}

void cMPGuageDialog::SetExpGuage(float percent) {
    // 1:1 with legacy CMPGuageDialog::SetExpGuage.
    // The legacy is:
    //   m_ExpGuage->SetValue(Percent, 0);
    //   char temp[128];
    //   sprintf(temp, "%4.2f%%", Percent*100);
    //   m_ExpPercent->SetStaticText(temp);
    //
    // The modern port:
    //   - CObjectGuagen::SetValue is TODO (R-12.x
    //     deferred). The void* m_ExpGuage is left
    //     untouched.
    //   - The m_ExpPercent->SetStaticText with
    //     sprintf "%4.2f%%" is REAL.
    // TODO: 1:1 with legacy m_ExpGuage->SetValue
    //       (Percent, 0); CObjectGuagen not ported.
    if (m_ExpPercent) {
        char temp[32];
        std::snprintf(temp, sizeof(temp), "%4.2f%%", percent * 100.0f);
        m_ExpPercent->SetStaticText(temp);
    }
}

void cMPGuageDialog::SetTime(std::uint32_t remainTime) {
    // 1:1 with legacy CMPGuageDialog::SetTime.
    // The legacy is:
    //   if (RemainTime < 30000)
    //     m_Time->SetFGColor(RGB_HALF(255, 0, 0));
    //   sprintf(buf, "%02d:%02d", RemainTime/60000, (RemainTime%60000)/1000);
    //   m_Time->SetStaticText(buf);
    if (m_Time) {
        if (remainTime < kRedTextThreshold) {
            // 1:1 with legacy RGB_HALF(255, 0, 0)
            // (red).
            m_Time->SetFGColor(0xFFFF0000u);
        }
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02u:%02u",
                      remainTime / 60000u,
                      (remainTime % 60000u) / 1000u);
        m_Time->SetStaticText(buf);
    }
}

void cMPGuageDialog::SetEventMapTimer(std::uint32_t remainTime,
                                      std::uint8_t bFlag) {
    // 1:1 with legacy CMPGuageDialog::SetEventMapTimer.
    // The legacy is:
    //   switch (bFlag) {
    //   case 0: m_Time->SetFGColor(RGB_HALF(0, 0, 255)); break;
    //   case 1:
    //     if (RemainTime < 30000)
    //       m_Time->SetFGColor(RGB_HALF(255, 0, 0));
    //     break;
    //   case 2: m_Time->SetFGColor(RGB_HALF(0, 0, 255)); break;
    //   }
    //   sprintf(buf, "%02d:%02d", RemainTime/60000, (RemainTime%60000)/1000);
    //   m_Time->SetStaticText(buf);
    if (m_Time) {
        switch (bFlag) {
        case kFlagReady:
            // 1:1 with legacy RGB_HALF(0, 0, 255)
            // (blue).
            m_Time->SetFGColor(0xFF0000FFu);
            break;
        case kFlagActive:
            if (remainTime < kRedTextThreshold) {
                m_Time->SetFGColor(0xFFFF0000u);
            }
            break;
        case kFlagStopped:
            m_Time->SetFGColor(0xFF0000FFu);
            break;
        default:
            break;
        }
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02u:%02u",
                      remainTime / 60000u,
                      (remainTime % 60000u) / 1000u);
        m_Time->SetStaticText(buf);
    }
}

void cMPGuageDialog::ShowEventMap() {
    // 1:1 with legacy CMPGuageDialog::ShowEventMap.
    // The legacy is:
    //   SetActive(TRUE);
    //   m_pTitle->SetStaticText(CHATMGR->GetChatMsg(140));
    SetActive(true);
    if (m_pTitle) {
        // 1:1 with legacy CHATMGR->GetChatMsg(140).
        // Modern port uses kEventMapTitle placeholder
        // until CHATMGR is ported.
        m_pTitle->SetStaticText(kEventMapTitle);
    }
}

}  // namespace mxh::ui
