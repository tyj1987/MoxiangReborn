// guildcreatedialog.hpp — modern port of 墨香 CGuildCreateDialog
// + CGuildUnionCreateDialog (guild create / guild union
// create dialogs).
//
// 1:1 port of legacy `CGuildCreateDialog` and
// `CGuildUnionCreateDialog` from
//   `墨香【源码】\[Client]MH\GuildCreateDialog.h` (679 B).
//
// Both classes are in the same header. The modern port
// keeps them together (1:1 with the legacy source
// layout) to minimize churn when reading.
//
// What the legacy does:
//
//   CGuildCreateDialog:
//   - Ctor: m_type = WT_GUILDCREATEDLG (legacy cWindow
//     type tag; modern cWindow / cDialog don't have
//     m_type, so modern port drops the ctor body).
//   - Linking: resolve 5 children (cStatic m_pLocation
//     + cEditBox m_pGuildName + cTextArea m_pIntro +
//     cButton m_OkBtn + cStatic m_CaptionName) by id.
//   - SetActive override: complex 7-singleton dispatch
//     (MAP + HERO + GUILDMGR + GAMEIN + RESRCMGR +
//     OBJECTSTATEMGR + OBJECTSTATE). The modern port
//     calls base SetActive and documents the singleton
//     dispatch as TODO.
//   - SetMunpaName / SetMunpaIntro: 1:1 wrappers that
//     set the edit box text + text area text. The
//     SetMunpaName also sets read-only mode on the
//     edit box (1:1 quirk).
//
//   CGuildUnionCreateDialog:
//   - Ctor: m_type = WT_GUILDUNIONCREATEDLG +
//     m_pNameEdit = NULL. Modern port: drops both.
//   - Linking: resolve 3 children (cEditBox m_pNameEdit
//     + cButton m_pOkBtn + cTextArea m_pText) by id.
//     Set the cTextArea's script text to a CHATMGR
//     message (msg 1125). The modern port uses
//     placeholder text "GUILD_UNION_TEXT" until
//     CHATMGR is ported.
//   - SetActive override: 1:1 with base noexcept spec.
//     If val: clear the name edit text. If !val:
//     HERO + GAMEIN + OBJECTSTATEMGR singleton check.
//     Modern port: base + TODO for singleton.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// these are the 12th + 13th **Tier 2** dialog ports
// (after cExitDialog, cMacroDialog, cCharMakeDlg,
// cGuildJoinDialog, cCharStateDialog, cSOSDialog,
// cWearedExDialog, cMiniFriendDialog, cReviveDialog,
// cMPNoticeDialog, cEventNotifyDialog). Both dialogs
// exercise cTextArea (already ported in 0.13.23) —
// the cTextArea infrastructure port directly enables
// both ports. The SetXxx wrappers are 1:1 REAL on
// the cEditBox / cTextArea / cStatic sub-widgets.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_GUILDCREATEDLG /
//     WT_GUILDUNIONCREATEDLG (legacy cWindow type tag
//     removed in Phase 6).
//   - Linking's SetScriptText call uses placeholder
//     text "GUILD_UNION_TEXT" until CHATMGR is ported.
//   - SetMunpaName also sets read-only on the edit
//     box (1:1 with legacy m_pGuildName->SetReadOnly(TRUE)).
//   - SetActive matches base noexcept (R-12
//     polymorphic virtual required).
//   - The 7-singleton dispatch in CGuildCreateDialog::SetActive
//     is documented as TODO (MAP / HERO / GUILDMGR /
//     GAMEIN / RESRCMGR / OBJECTSTATEMGR / OBJECTSTATE).
//   - The 4-singleton dispatch in
//     CGuildUnionCreateDialog::SetActive is documented
//     as TODO (HERO / GAMEIN / OBJECTSTATEMGR / OBJECTSTATE).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cEditBox;
class cTextArea;
class cButton;

class cGuildCreateDialog : public cDialog {
public:
    cGuildCreateDialog();
    ~cGuildCreateDialog() override;

    // ----- 1:1 with legacy CGuildCreateDialog::Linking -----

    // Resolves 5 children by id (kLocationId=280,
    // kGuildNameId=281, kIntroId=282, kOkBtnId=283,
    // kCaptionNameId=284). Pure widget ops.
    void Linking();

    // ----- 1:1 with legacy CGuildCreateDialog::SetActive -----

    // 1:1 override: complex 7-singleton dispatch.
    // Modern port: calls base SetActive + TODO for
    // singleton dispatch.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CGuildCreateDialog::SetMunpaName / SetMunpaIntro -----

    // SetMunpaName: sets the guild name edit text +
    // marks the edit box read-only. 1:1 quirk: the
    // legacy also sets m_pGuildName->SetReadOnly(TRUE)
    // after setting the text.
    void SetMunpaName(const char* name);
    void SetMunpaIntro(const char* intro);

    // ----- Accessors (used by tests) -----

    cStatic*  GetLocation()    const noexcept { return m_pLocation; }
    cEditBox* GetGuildName()   const noexcept { return m_pGuildName; }
    cTextArea* GetIntro()      const noexcept { return m_pIntro; }
    cButton*  GetOkButton()    const noexcept { return m_OkBtn; }
    cStatic*  GetCaptionName() const noexcept { return m_CaptionName; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kLocationId    = 280;  // was GD_CLOCATION
    static constexpr std::int32_t kGuildNameId   = 281;  // was GD_CNAME
    static constexpr std::int32_t kIntroId       = 282;  // was GD_CINTROTEXT
    static constexpr std::int32_t kOkBtnId       = 283;  // was GD_CCREATEOKBTN
    static constexpr std::int32_t kCaptionNameId = 284;  // was CR_CAP

private:
    cStatic*  m_pLocation    = nullptr;
    cEditBox* m_pGuildName   = nullptr;
    cTextArea* m_pIntro      = nullptr;
    cButton*  m_OkBtn        = nullptr;
    cStatic*  m_CaptionName  = nullptr;
};

class cGuildUnionCreateDialog : public cDialog {
public:
    cGuildUnionCreateDialog();
    ~cGuildUnionCreateDialog() override;

    // ----- 1:1 with legacy CGuildUnionCreateDialog::Linking -----

    // Resolves 3 children by id (kNameEditId=290,
    // kOkBtnId=291, kTextId=292) and calls SetScriptText
    // on the cTextArea. Uses placeholder text
    // "GUILD_UNION_TEXT" until CHATMGR is ported.
    void Linking();

    // ----- 1:1 with legacy CGuildUnionCreateDialog::SetActive -----

    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using GetHeroStateFn = std::int32_t (*)(void* userData);
    using IsNpcScriptDialogActiveFn = bool (*)(void* userData);
    using EndObjectStateFn = void (*)(std::uint32_t objectId,
                                      std::int32_t stateIdx,
                                      void* userData);

    void SetCallbacks(GetHeroObjectIdFn getHeroObjectId,
                      GetHeroStateFn getHeroState,
                      IsNpcScriptDialogActiveFn isNpcScriptDialogActive,
                      EndObjectStateFn endObjectState,
                      void* userData = nullptr) noexcept;

    // 1:1 override: HERO-null early return, deal-state
    // cancellation, focus release, then base SetActive.
    void SetActive(bool val) noexcept override;

    // ----- Accessors (used by tests) -----

    cEditBox* GetNameEdit() const noexcept { return m_pNameEdit; }
    cButton*  GetOkButton() const noexcept { return m_pOkBtn; }
    cTextArea* GetText()    const noexcept { return m_pText; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kNameEditId = 290;  // was GDU_NAME
    static constexpr std::int32_t kOkBtnId   = 291;  // was GDU_OKBTN
    static constexpr std::int32_t kTextId     = 292;  // was GDU_TEXT
    static constexpr std::int32_t kObjectStateDeal = 6;

private:
    GetHeroObjectIdFn m_getHeroObjectId = nullptr;
    GetHeroStateFn m_getHeroState = nullptr;
    IsNpcScriptDialogActiveFn m_isNpcScriptDialogActive = nullptr;
    EndObjectStateFn m_endObjectState = nullptr;
    void* m_callbackUserData = nullptr;

    cEditBox* m_pNameEdit = nullptr;
    cButton*  m_pOkBtn    = nullptr;
    cTextArea* m_pText    = nullptr;
};

}  // namespace mxh::ui
