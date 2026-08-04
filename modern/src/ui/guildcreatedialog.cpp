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

void cGuildCreateDialog::SetCallbacks(
    GetHeroObjectIdFn         getHeroObjectId,
    GetHeroGuildIdxFn         getHeroGuildIdx,
    GetHeroStateFn            getHeroState,
    GetMapNameFn              getMapName,
    GetGuildNameFn            getGuildName,
    IsNpcScriptDialogActiveFn isNpcScriptDialogActive,
    GetLocalizedMessageFn     getLocalizedMessage,
    EndObjectStateFn          endObjectState,
    void*                     userData) noexcept {
    m_getHeroObjectId         = getHeroObjectId;
    m_getHeroGuildIdx         = getHeroGuildIdx;
    m_getHeroState            = getHeroState;
    m_getMapName              = getMapName;
    m_getGuildName            = getGuildName;
    m_isNpcScriptDialogActive = isNpcScriptDialogActive;
    m_getLocalizedMessage     = getLocalizedMessage;
    m_endObjectState          = endObjectState;
    m_callbackUserData         = userData;
}

void cGuildCreateDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGuildCreateDialog::SetActive.
    // The legacy is a 7-singleton dispatch:
    //   - val == TRUE: query MAP for the map
    //     name, clear the guild-name + intro
    //     widgets, branch on HERO->GetGuildIdx():
    //     * existing-guild view: caption=msg 270,
    //       button=msg 335, edit box = current
    //       guild name (read-only).
    //     * create-guild view: caption=msg 510,
    //       button=msg 513, edit box is
    //       writeable.
    //   - val == FALSE: release edit focus,
    //     if HERO == 0 return; if HERO state
    //     is eObjectState_Deal AND the NPC
    //     script dialog is inactive, dispatch
    //     OBJECTSTATEMGR->EndObjectState(HERO,
    //     eObjectState_Deal).
    //   cDialog::SetActive(val) is called last
    //   in both branches (1:1 with legacy).
    if (val) {
        // 1. Set location text (host MAP callback).
        if (m_getMapName && m_pLocation) {
            const char* mapName =
                m_getMapName(m_callbackUserData);
            if (mapName) m_pLocation->SetStaticText(mapName);
        }
        // 2. Clear guild-name edit text (REAL).
        if (m_pGuildName) m_pGuildName->SetEditText("");
        // 3. Clear intro (REAL).
        if (m_pIntro) m_pIntro->SetScriptText("");
        // 4. Branch on HERO->GetGuildIdx().
        const std::uint32_t heroGuildIdx =
            m_getHeroGuildIdx
                ? m_getHeroGuildIdx(m_callbackUserData)
                : 0u;
        if (heroGuildIdx != 0u) {
            // Existing-guild member view.
            if (m_getLocalizedMessage && m_CaptionName) {
                m_CaptionName->SetStaticText(
                    m_getLocalizedMessage(kMsgEditExistingGuildCaption,
                                          m_callbackUserData));
            }
            if (m_getLocalizedMessage && m_OkBtn) {
                m_OkBtn->SetText(
                    m_getLocalizedMessage(kMsgRenameGuildButton,
                                          m_callbackUserData));
            }
            // Pre-fill the read-only edit box
            // with the current guild name.
            if (m_getGuildName) {
                const char* gName = m_getGuildName(m_callbackUserData);
                if (gName) SetMunpaName(gName);
            }
        } else {
            // Create-new-guild view.
            if (m_getLocalizedMessage && m_CaptionName) {
                m_CaptionName->SetStaticText(
                    m_getLocalizedMessage(kMsgCreateGuildCaption,
                                          m_callbackUserData));
            }
            if (m_getLocalizedMessage && m_OkBtn) {
                m_OkBtn->SetText(
                    m_getLocalizedMessage(kMsgCreateGuildButton,
                                          m_callbackUserData));
            }
            // 1:1 quirk: legacy un-readonly's
            // the edit box here (so the user
            // can type a guild name).
            if (m_pGuildName) m_pGuildName->SetReadOnly(false);
        }
    } else {
        // 1:1 with legacy: legacy releases the
        // edit-box focus FIRST, then performs
        // the HERO null-check. Modern port
        // preserves this order byte-for-byte:
        // SetFocusEdit happens regardless of
        // HERO state, then we return early
        // when HERO is null.
        if (m_pGuildName) m_pGuildName->SetFocusEdit(false);
        const auto heroObjectId =
            m_getHeroObjectId
                ? m_getHeroObjectId(m_callbackUserData)
                : 0u;
        if (heroObjectId == 0u) return;
        // 1:1 with legacy guard:
        //   if (HERO->GetState() == eObjectState_Deal &&
        //       GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE)
        //   then OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
        if (m_getHeroState && m_isNpcScriptDialogActive && m_endObjectState &&
            m_getHeroState(m_callbackUserData) == kObjectStateDeal &&
            !m_isNpcScriptDialogActive(m_callbackUserData)) {
            m_endObjectState(heroObjectId, kObjectStateDeal, m_callbackUserData);
        }
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

void cGuildUnionCreateDialog::SetCallbacks(
    GetHeroObjectIdFn getHeroObjectId,
    GetHeroStateFn getHeroState,
    IsNpcScriptDialogActiveFn isNpcScriptDialogActive,
    EndObjectStateFn endObjectState,
    void* userData) noexcept {
    m_getHeroObjectId = getHeroObjectId;
    m_getHeroState = getHeroState;
    m_isNpcScriptDialogActive = isNpcScriptDialogActive;
    m_endObjectState = endObjectState;
    m_callbackUserData = userData;
}

void cGuildUnionCreateDialog::SetActive(bool val) noexcept {
    if (val) {
        if (m_pNameEdit) m_pNameEdit->SetEditText("");
    } else {
        const auto heroObjectId = m_getHeroObjectId
            ? m_getHeroObjectId(m_callbackUserData)
            : 0u;
        if (heroObjectId == 0u) return;

        if (m_getHeroState && m_isNpcScriptDialogActive && m_endObjectState &&
            m_getHeroState(m_callbackUserData) == kObjectStateDeal &&
            !m_isNpcScriptDialogActive(m_callbackUserData)) {
            m_endObjectState(heroObjectId, kObjectStateDeal, m_callbackUserData);
        }
        if (m_pNameEdit) m_pNameEdit->SetFocusEdit(false);
    }
    cDialog::SetActive(val);
}

}  // namespace mxh::ui
