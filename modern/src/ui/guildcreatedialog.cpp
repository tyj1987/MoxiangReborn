// guildcreatedialog.cpp — 1:1 port of 墨香
// CGuildCreateDialog + CGuildUnionCreateDialog. See
// guildcreatedialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "guildcreatedialog.hpp"
#include "cstatic.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

// ===========================================================================
// cGuildCreateDialog
// ===========================================================================

cGuildCreateDialog::cGuildCreateDialog() = default;

cGuildCreateDialog::~cGuildCreateDialog() = default;

void cGuildCreateDialog::Linking() {
    // 1:1 with legacy CGuildCreateDialog::Linking.
    // REAL — resolve 5 children by id. Defensive
    // null-checks (the legacy unconditionally
    // dereferences each in SetActive).
    m_pLocation   = static_cast<cStatic*>(findWindowById(kLocationId));
    m_pGuildName  = static_cast<cEditBox*>(findWindowById(kGuildNameId));
    m_pIntro      = static_cast<cTextArea*>(findWindowById(kIntroId));
    m_OkBtn       = static_cast<cButton*>(findWindowById(kOkBtnId));
    m_CaptionName = static_cast<cStatic*>(findWindowById(kCaptionNameId));
}

void cGuildCreateDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGuildCreateDialog::SetActive.
    // The legacy is:
    //   if (val == TRUE) {
    //       m_pLocation->SetStaticText(MAP->GetMapName(...));
    //       m_pGuildName->SetEditText("");
    //       m_pIntro->SetScriptText("");
    //       if (HERO->GetGuildIdx()) {
    //           m_CaptionName->SetStaticText(RESRCMGR->GetMsg(270));
    //           m_OkBtn->SetText(RESRCMGR->GetMsg(335), ...);
    //           SetMunpaName(GUILDMGR->GetGuildName());
    //       } else {
    //           m_CaptionName->SetStaticText(RESRCMGR->GetMsg(510));
    //           m_OkBtn->SetText(RESRCMGR->GetMsg(513), ...);
    //           m_pGuildName->SetReadOnly(FALSE);
    //       }
    //   } else {
    //       m_pGuildName->SetFocusEdit(FALSE);
    //       if (HERO == 0) return;
    //       if (HERO->GetState() == eObjectState_Deal &&
    //           GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE)
    //           OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //   }
    //   cDialog::SetActive(val);
    //
    // Modern port: calls base SetActive last (1:1 with
    // legacy: legacy calls base last). The 7-singleton
    // dispatch is TODO.
    if (val) {
        // TODO: dispatch to MAP/HERO/GUILDMGR/GAMEIN/
        //       RESRCMGR/OBJECTSTATEMGR/OBJECTSTATE.
        //       The branch is:
        //         m_pLocation->SetStaticText(MAP->GetMapName(...))
        //         m_pGuildName->SetEditText("")
        //         m_pIntro->SetScriptText("")
        //         if (HERO->GetGuildIdx()) {
        //             // existing guild member view
        //             m_CaptionName->SetStaticText(RESRCMGR->GetMsg(270))
        //             m_OkBtn->SetText(RESRCMGR->GetMsg(335), ...)
        //             SetMunpaName(GUILDMGR->GetGuildName())
        //         } else {
        //             // create new guild view
        //             m_CaptionName->SetStaticText(RESRCMGR->GetMsg(510))
        //             m_OkBtn->SetText(RESRCMGR->GetMsg(513), ...)
        //             m_pGuildName->SetReadOnly(FALSE)
        //         }
    } else {
        // TODO: m_pGuildName->SetFocusEdit(FALSE) +
        //       HERO + GAMEIN + OBJECTSTATEMGR check +
        //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
    }
    cDialog::SetActive(val);
}

void cGuildCreateDialog::SetMunpaName(const char* name) {
    // 1:1 with legacy CGuildCreateDialog::SetMunpaName.
    // The legacy is:
    //   m_pGuildName->SetEditText(strName);
    //   m_pGuildName->SetReadOnly(TRUE);
    //
    // Defensive null-checks on the edit box.
    if (m_pGuildName) {
        m_pGuildName->SetEditText(name);
        m_pGuildName->SetReadOnly(true);
    }
}

void cGuildCreateDialog::SetMunpaIntro(const char* intro) {
    // 1:1 with legacy CGuildCreateDialog::SetMunpaIntro.
    // The legacy is:
    //   m_pIntro->SetScriptText(strIntro);
    if (m_pIntro) m_pIntro->SetScriptText(intro);
}

// ===========================================================================
// cGuildUnionCreateDialog
// ===========================================================================

cGuildUnionCreateDialog::cGuildUnionCreateDialog() = default;

cGuildUnionCreateDialog::~cGuildUnionCreateDialog() = default;

void cGuildUnionCreateDialog::Linking() {
    // 1:1 with legacy CGuildUnionCreateDialog::Linking.
    // REAL — resolve 3 children by id + call SetScriptText
    // on the cTextArea. Defensive null-checks.
    m_pNameEdit = static_cast<cEditBox*>(findWindowById(kNameEditId));
    m_pOkBtn    = static_cast<cButton*>(findWindowById(kOkBtnId));
    m_pText     = static_cast<cTextArea*>(findWindowById(kTextId));

    // 1:1 quirk: legacy calls
    //   m_pText->SetScriptText(CHATMGR->GetChatMsg(1125))
    // Modern port uses placeholder text "GUILD_UNION_TEXT"
    // until CHATMGR is ported.
    if (m_pText) m_pText->SetScriptText("GUILD_UNION_TEXT");
}

void cGuildUnionCreateDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGuildUnionCreateDialog::SetActive.
    // The legacy is:
    //   if (val == TRUE) {
    //       m_pNameEdit->SetEditText("");
    //   } else {
    //       if (HERO == 0) return;
    //       if (HERO->GetState() == eObjectState_Deal &&
    //           GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE)
    //           OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //       if (m_pNameEdit) m_pNameEdit->SetFocusEdit(FALSE);
    //   }
    //   cDialog::SetActive(val);
    //
    // Modern port: base SetActive + TODO for singleton
    // dispatch. The "if (HERO == 0) return" early-out
    // is preserved in the TODO (it skips the singleton
    // dispatch when HERO is null).
    if (val) {
        // TODO: m_pNameEdit->SetEditText("") when
        //       m_pNameEdit is non-null.
    } else {
        // TODO: HERO check (early return if null) +
        //       OBJECTSTATEMGR check +
        //       m_pNameEdit->SetFocusEdit(FALSE).
    }
    cDialog::SetActive(val);
}

}  // namespace mxh::ui
