// coptiondialog.cpp — modern port of 墨香 COptionDialog.

#include "mxh/ui/coptiondialog.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/cPushupButton.hpp"
#include "mxh/ui/cGuageBar.hpp"
#include "mxh/ui/legacy_window_event.hpp"

#include <cassert>
#include <cstddef>

namespace mxh::ui {

namespace {

// 1:1 with legacy OTI_CB_* / OTI_PB_* / OTI_GB_* ids.
// The legacy uses a single enum OTI_* in WindowIDEnum.h; we
// reproduce the values used in UpdateData() / OnActionEvent().
// These are a *representative* subset (the legacy enum has 60+
// entries).  The modern port lets tests inject their own ids
// via WidgetAccessor callbacks, so the precise id values are
// not part of the 1:1 surface; they're constants for the
// OK / CANCEL / RESET buttons + a couple of well-known check
// boxes / pushup btns / guage bars.
//
// We pick ids > 1000 to avoid clashing with the modern cDialog
// default window ids (which the legacy OTI_* enum lives in
// the 30-100 range; we don't care about cross-compat with the
// legacy WINDOW_ID tree -- only the API surface is preserved).
constexpr std::int32_t kTabSheetGame     = 0;
constexpr std::int32_t kTabSheetChat     = 1;
constexpr std::int32_t kTabSheetGraphic  = 2;
constexpr std::int32_t kTabSheetSound    = 3;

}  // namespace

cOptionDialog::cOptionDialog() {
    // 1:1 with legacy ctor.  No additional init.
}

cOptionDialog::~cOptionDialog() = default;

void cOptionDialog::Add(cWindow* window) {
    if (!window) return;
    // 1:1 with legacy COptionDialog::Add: routes
    // PUSHBUTTON -> AddTabBtn(curIdx1++) and
    // DIALOG -> AddTabSheet(curIdx2++).  Other types fall
    // through to cDialog::Add.
    //
    // The modern cWindow::Add takes a std::unique_ptr, so we
    // can't forward a raw pointer to the base.  We track the
    // slot mappings in m_childById and skip the base Add --
    // tests don't need the children in cDialog::children
    // (the WidgetAccessor is what drives UpdateData).
    if (dynamic_cast<cPushupButton*>(window) != nullptr) {
        m_childById[m_curIdx1] = window;
        ++m_curIdx1;
    } else if (dynamic_cast<cDialog*>(window) != nullptr) {
        m_childById[m_curIdx2 + 100] = window;   // tab sheets in 100..103
        ++m_curIdx2;
    }
    // Note: legacy `cTabDialog::Add(cWindow*)` takes a raw
    // pointer and self-manages the lifetime (cPtrList).
    // The modern port tracks the slot via m_childById and
    // expects the host to keep the cWindow* alive (e.g. as
    // a unique_ptr at the call site).
}

void cOptionDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive.  The legacy checks m_bDisable
    // first; the modern port's cDialog::SetDisable cascades
    // through the children, so we honour it the same way.
    if (isEnabled()) {
        if (val) {
            // 1:1 with legacy: pull the snapshot from OPTIONMGR
            // and call UpdateData(FALSE).  The modern port
            // hands the snapshot off via a DefaultCallback the
            // host injects (default = copy from the host's
            // OptionManager).  When no callback is set, the
            // modern port keeps whatever m_GameOption holds
            // (so tests can pre-load the option struct).
            if (m_defaultCb) {
                m_defaultCb(&m_GameOption, m_defaultUser);
            }
            UpdateData(/*bSave=*/false);
        }
        cDialog::SetActive(val);
    }
    // 1:1 with legacy: notify the main-bar option icon.
    if (m_mainBarCb) {
        m_mainBarCb(isActive(), m_mainBarUser);
    }
}

void cOptionDialog::Linking() {
    auto checkbox = [this](std::string_view id) {
        return dynamic_cast<cCheckBox*>(findWindowByLegacyId(id));
    };
    auto gauge = [this](std::string_view id) {
        return dynamic_cast<cGuageBar*>(findWindowByLegacyId(id));
    };
    auto pushup = [this](std::string_view id) {
        return dynamic_cast<cPushupButton*>(findWindowByLegacyId(id));
    };
    m_cbNoDeal = checkbox("OTI_CB_NODEAL"); m_cbNoParty = checkbox("OTI_CB_NOPARTY");
    m_cbNoFriend = checkbox("OTI_CB_NOFRIEND"); m_cbNoChatting = checkbox("OTI_CB_NOCHATTING");
    m_cbNoWhisper = checkbox("OTI_CB_NOWHISPER"); m_cbNoBalloon = checkbox("OTI_CB_NOBALLOON");
    m_cbAutoHide = checkbox("OTI_CB_AUTOHIDE"); m_cbNoSystemMsg = checkbox("OTI_CB_NOSYSTEMMSG");
    m_cbNoItemMsg = checkbox("OTI_CB_NOITEMMSG"); m_cbShadowHero = checkbox("OTI_CB_HEROSHADOW");
    m_cbShadowMonster = checkbox("OTI_CB_MONSTERSHADOW"); m_cbShadowOthers = checkbox("OTI_CB_OTHERSSHADOW");
    m_cbAutoControl = checkbox("OTI_CB_AUTOCONTROL"); m_cbBgmSound = checkbox("OTI_CB_BGMSOUND");
    m_cbEnvSound = checkbox("OTI_CB_ENVSOUND");
    m_gbGamma = gauge("OTI_GB_GAMMA"); m_gbSight = gauge("OTI_GB_SIGHT");
    m_gbBgmSound = gauge("OTI_GB_BGMSOUND"); m_gbEnvSound = gauge("OTI_GB_ENVSOUND");
    if (m_gbGamma) m_gbGamma->InitValue(0, 100, 50);
    if (m_gbSight) m_gbSight->InitValue(0, 100, 100);
    if (m_gbBgmSound) m_gbBgmSound->InitValue(0, 100, 100);
    if (m_gbEnvSound) m_gbEnvSound->InitValue(0, 100, 100);
    m_pbChatMode = pushup("OTI_PB_CHATMODE"); m_pbMacroMode = pushup("OTI_PB_MACROMODE");
    m_pbBasicGraphic = pushup("OTI_PB_BASICGRAPHIC"); m_pbDownGraphic = pushup("OTI_PB_DOWNGRAPHIC");
    m_pbBasicEffect = pushup("OTI_PB_BASICEFFECT"); m_pbOneEffect = pushup("OTI_PB_ONEEFFECT");
    m_concreteBindings = m_cbNoDeal && m_cbNoParty && m_cbBgmSound && m_gbGamma && m_gbBgmSound;
}

void cOptionDialog::UpdateData(bool bSave) {
    if (m_concreteBindings) {
        if (bSave) {
            m_GameOption.bNoDeal = m_cbNoDeal->IsChecked(); m_GameOption.bNoParty = m_cbNoParty->IsChecked();
            m_GameOption.bNoFriend = m_cbNoFriend && m_cbNoFriend->IsChecked();
            m_GameOption.bNoChatting = m_cbNoChatting && m_cbNoChatting->IsChecked();
            m_GameOption.bNoWhisper = m_cbNoWhisper && m_cbNoWhisper->IsChecked();
            m_GameOption.bNoBalloon = m_cbNoBalloon && m_cbNoBalloon->IsChecked();
            m_GameOption.bAutoHide = m_cbAutoHide && m_cbAutoHide->IsChecked();
            m_GameOption.bNoSystemMsg = m_cbNoSystemMsg && m_cbNoSystemMsg->IsChecked();
            m_GameOption.bNoItemMsg = m_cbNoItemMsg && m_cbNoItemMsg->IsChecked();
            m_GameOption.bShadowHero = m_cbShadowHero && m_cbShadowHero->IsChecked();
            m_GameOption.bShadowMonster = m_cbShadowMonster && m_cbShadowMonster->IsChecked();
            m_GameOption.bShadowOthers = m_cbShadowOthers && m_cbShadowOthers->IsChecked();
            m_GameOption.bAutoCtrl = m_cbAutoControl && m_cbAutoControl->IsChecked();
            m_GameOption.bSoundBGM = m_cbBgmSound->IsChecked();
            m_GameOption.bSoundEnvironment = m_cbEnvSound && m_cbEnvSound->IsChecked();
            m_GameOption.nGamma = m_gbGamma->GetCurValue(); m_GameOption.nSightDistance = m_gbSight->GetCurValue();
            m_GameOption.nVolumnBGM = m_gbBgmSound->GetCurValue();
            m_GameOption.nVolumnEnvironment = m_gbEnvSound->GetCurValue();
            m_GameOption.nMacroMode = m_pbMacroMode && m_pbMacroMode->IsPushed() ? 1 : 0;
            m_GameOption.nLODMode = m_pbDownGraphic && m_pbDownGraphic->IsPushed() ? 1 : 0;
            m_GameOption.nEffectMode = m_pbOneEffect && m_pbOneEffect->IsPushed() ? 1 : 0;
        } else {
            m_cbNoDeal->SetChecked(m_GameOption.bNoDeal); m_cbNoParty->SetChecked(m_GameOption.bNoParty);
            if (m_cbNoFriend) m_cbNoFriend->SetChecked(m_GameOption.bNoFriend);
            if (m_cbNoChatting) m_cbNoChatting->SetChecked(m_GameOption.bNoChatting);
            if (m_cbNoWhisper) m_cbNoWhisper->SetChecked(m_GameOption.bNoWhisper);
            if (m_cbNoBalloon) m_cbNoBalloon->SetChecked(m_GameOption.bNoBalloon);
            if (m_cbAutoHide) m_cbAutoHide->SetChecked(m_GameOption.bAutoHide);
            if (m_cbNoSystemMsg) m_cbNoSystemMsg->SetChecked(m_GameOption.bNoSystemMsg);
            if (m_cbNoItemMsg) m_cbNoItemMsg->SetChecked(m_GameOption.bNoItemMsg);
            if (m_cbShadowHero) m_cbShadowHero->SetChecked(m_GameOption.bShadowHero);
            if (m_cbShadowMonster) m_cbShadowMonster->SetChecked(m_GameOption.bShadowMonster);
            if (m_cbShadowOthers) m_cbShadowOthers->SetChecked(m_GameOption.bShadowOthers);
            if (m_cbAutoControl) m_cbAutoControl->SetChecked(m_GameOption.bAutoCtrl);
            m_cbBgmSound->SetChecked(m_GameOption.bSoundBGM); if (m_cbEnvSound) m_cbEnvSound->SetChecked(m_GameOption.bSoundEnvironment);
            m_gbGamma->SetCurValue(m_GameOption.nGamma); m_gbSight->SetCurValue(m_GameOption.nSightDistance);
            m_gbBgmSound->SetCurValue(m_GameOption.nVolumnBGM); m_gbEnvSound->SetCurValue(m_GameOption.nVolumnEnvironment);
            if (m_pbMacroMode) m_pbMacroMode->SetPush(m_GameOption.nMacroMode != 0);
            if (m_pbBasicGraphic) m_pbBasicGraphic->SetPush(m_GameOption.nLODMode == 0);
            if (m_pbDownGraphic) m_pbDownGraphic->SetPush(m_GameOption.nLODMode != 0);
            if (m_pbBasicEffect) m_pbBasicEffect->SetPush(m_GameOption.nEffectMode == 0);
            if (m_pbOneEffect) m_pbOneEffect->SetPush(m_GameOption.nEffectMode != 0);
            DisableGraphicTab(m_GameOption.bAutoCtrl);
        }
        return;
    }
    // 1:1 with legacy UpdateData.  bSave=true reads from the
    // tab sheets into m_GameOption; bSave=false writes from
    // m_GameOption to the tab sheets.  Both paths are routed
    // through the WidgetAccessor callbacks the host injects.
    if (bSave) {
        // Tab 0 (game)
        if (m_accessor.checkboxIsChecked) {
            m_GameOption.bNoDeal         = m_accessor.checkboxIsChecked(101, m_accessor.user);
            m_GameOption.bNoParty        = m_accessor.checkboxIsChecked(102, m_accessor.user);
            m_GameOption.bNoFriend       = m_accessor.checkboxIsChecked(103, m_accessor.user);
            m_GameOption.bNoVimu         = m_accessor.checkboxIsChecked(104, m_accessor.user);
            m_GameOption.bNameMunpa      = m_accessor.checkboxIsChecked(105, m_accessor.user);
            m_GameOption.bNameParty      = m_accessor.checkboxIsChecked(106, m_accessor.user);
            m_GameOption.bNameOthers     = m_accessor.checkboxIsChecked(107, m_accessor.user);
            m_GameOption.bNoMemberDamage = m_accessor.checkboxIsChecked(108, m_accessor.user);
            m_GameOption.bNoGameTip      = m_accessor.checkboxIsChecked(109, m_accessor.user);
            m_GameOption.bMunpaIntro     = m_accessor.checkboxIsChecked(110, m_accessor.user);
        }
        if (m_accessor.pushupIsPushed) {
            m_GameOption.nMacroMode      = m_accessor.pushupIsPushed(120, m_accessor.user) ? 1 : 0;
        }

        // Tab 1 (chat)
        if (m_accessor.checkboxIsChecked) {
            m_GameOption.bNoWhisper      = m_accessor.checkboxIsChecked(201, m_accessor.user);
            m_GameOption.bNoChatting     = m_accessor.checkboxIsChecked(202, m_accessor.user);
            m_GameOption.bNoBalloon      = m_accessor.checkboxIsChecked(203, m_accessor.user);
            m_GameOption.bAutoHide       = m_accessor.checkboxIsChecked(204, m_accessor.user);
            m_GameOption.bNoShoutChat    = m_accessor.checkboxIsChecked(205, m_accessor.user);
            m_GameOption.bNoGuildChat    = m_accessor.checkboxIsChecked(206, m_accessor.user);
            m_GameOption.bNoAllianceChat = m_accessor.checkboxIsChecked(207, m_accessor.user);
            m_GameOption.bNoSystemMsg    = m_accessor.checkboxIsChecked(208, m_accessor.user);
            m_GameOption.bNoExpMsg       = m_accessor.checkboxIsChecked(209, m_accessor.user);
            m_GameOption.bNoItemMsg      = m_accessor.checkboxIsChecked(210, m_accessor.user);
        }

        // Tab 2 (graphic)
        if (m_accessor.guageGetCur) {
            m_GameOption.nGamma          = m_accessor.guageGetCur(301, m_accessor.user);
            m_GameOption.nSightDistance  = m_accessor.guageGetCur(302, m_accessor.user);
        }
        if (m_accessor.checkboxIsChecked) {
            m_GameOption.bShadowHero     = m_accessor.checkboxIsChecked(303, m_accessor.user);
            m_GameOption.bShadowMonster  = m_accessor.checkboxIsChecked(304, m_accessor.user);
            m_GameOption.bShadowOthers   = m_accessor.checkboxIsChecked(305, m_accessor.user);
            m_GameOption.bNoAvatarView   = m_accessor.checkboxIsChecked(306, m_accessor.user);
            m_GameOption.nEffectSnow     = m_accessor.checkboxIsChecked(307, m_accessor.user) ? 1 : 0;
            m_GameOption.bAutoCtrl       = m_accessor.checkboxIsChecked(308, m_accessor.user);
            m_GameOption.bAmbientMax     = m_accessor.checkboxIsChecked(309, m_accessor.user);
        }
        if (m_accessor.pushupIsPushed) {
            m_GameOption.nLODMode        = m_accessor.pushupIsPushed(310, m_accessor.user) ? 1 : 0;
            m_GameOption.nEffectMode     = m_accessor.pushupIsPushed(311, m_accessor.user) ? 1 : 0;
        }

        // Tab 3 (sound)
        if (m_accessor.checkboxIsChecked) {
            m_GameOption.bSoundBGM         = m_accessor.checkboxIsChecked(401, m_accessor.user);
            m_GameOption.bSoundEnvironment = m_accessor.checkboxIsChecked(402, m_accessor.user);
        }
        if (m_accessor.guageGetCur) {
            m_GameOption.nVolumnBGM         = m_accessor.guageGetCur(403, m_accessor.user);
            m_GameOption.nVolumnEnvironment = m_accessor.guageGetCur(404, m_accessor.user);
        }
    } else {
        // Tab 0 (game)
        if (m_accessor.checkboxSetChecked) {
            m_accessor.checkboxSetChecked(101, m_GameOption.bNoDeal,         m_accessor.user);
            m_accessor.checkboxSetChecked(102, m_GameOption.bNoParty,        m_accessor.user);
            m_accessor.checkboxSetChecked(103, m_GameOption.bNoFriend,       m_accessor.user);
            m_accessor.checkboxSetChecked(104, m_GameOption.bNoVimu,         m_accessor.user);
            m_accessor.checkboxSetChecked(105, m_GameOption.bNameMunpa,      m_accessor.user);
            m_accessor.checkboxSetChecked(106, m_GameOption.bNameParty,      m_accessor.user);
            m_accessor.checkboxSetChecked(107, m_GameOption.bNameOthers,     m_accessor.user);
            m_accessor.checkboxSetChecked(108, m_GameOption.bNoMemberDamage, m_accessor.user);
            m_accessor.checkboxSetChecked(109, m_GameOption.bNoGameTip,      m_accessor.user);
            m_accessor.checkboxSetChecked(110, m_GameOption.bMunpaIntro,     m_accessor.user);
        }
        if (m_accessor.pushupSetPush) {
            m_accessor.pushupSetPush(120, m_GameOption.nMacroMode != 0, m_accessor.user);
            m_accessor.pushupSetPush(121, m_GameOption.nMacroMode == 0, m_accessor.user);
        }
        // Tab 1 (chat)
        if (m_accessor.checkboxSetChecked) {
            m_accessor.checkboxSetChecked(201, m_GameOption.bNoWhisper,      m_accessor.user);
            m_accessor.checkboxSetChecked(202, m_GameOption.bNoChatting,     m_accessor.user);
            m_accessor.checkboxSetChecked(203, m_GameOption.bNoBalloon,      m_accessor.user);
            m_accessor.checkboxSetChecked(204, m_GameOption.bAutoHide,       m_accessor.user);
            m_accessor.checkboxSetChecked(205, m_GameOption.bNoShoutChat,    m_accessor.user);
            m_accessor.checkboxSetChecked(206, m_GameOption.bNoGuildChat,    m_accessor.user);
            m_accessor.checkboxSetChecked(207, m_GameOption.bNoAllianceChat, m_accessor.user);
            m_accessor.checkboxSetChecked(208, m_GameOption.bNoSystemMsg,    m_accessor.user);
            m_accessor.checkboxSetChecked(209, m_GameOption.bNoExpMsg,       m_accessor.user);
            m_accessor.checkboxSetChecked(210, m_GameOption.bNoItemMsg,      m_accessor.user);
        }
        // Tab 2 (graphic)
        if (m_accessor.guageSetCur) {
            m_accessor.guageSetCur(301, m_GameOption.nGamma,         m_accessor.user);
            m_accessor.guageSetCur(302, m_GameOption.nSightDistance, m_accessor.user);
        }
        if (m_accessor.checkboxSetChecked) {
            m_accessor.checkboxSetChecked(303, m_GameOption.bShadowHero,     m_accessor.user);
            m_accessor.checkboxSetChecked(304, m_GameOption.bShadowMonster,  m_accessor.user);
            m_accessor.checkboxSetChecked(305, m_GameOption.bShadowOthers,   m_accessor.user);
            m_accessor.checkboxSetChecked(306, m_GameOption.bNoAvatarView,   m_accessor.user);
            m_accessor.checkboxSetChecked(307, m_GameOption.nEffectSnow != 0, m_accessor.user);
            m_accessor.checkboxSetChecked(308, m_GameOption.bAutoCtrl,       m_accessor.user);
            m_accessor.checkboxSetChecked(309, m_GameOption.bAmbientMax,     m_accessor.user);
        }
        if (m_accessor.pushupSetPush) {
            m_accessor.pushupSetPush(310, m_GameOption.nLODMode == 0,    m_accessor.user);
            m_accessor.pushupSetPush(311, m_GameOption.nLODMode != 0,    m_accessor.user);
            m_accessor.pushupSetPush(312, m_GameOption.nEffectMode == 0, m_accessor.user);
            m_accessor.pushupSetPush(313, m_GameOption.nEffectMode != 0, m_accessor.user);
        }
        // 1:1 with legacy UpdateData(FALSE) tail: re-disable
        // the graphic tab controls if bAutoCtrl is on.
        DisableGraphicTab(m_GameOption.bAutoCtrl);
        // Tab 3 (sound)
        if (m_accessor.checkboxSetChecked) {
            m_accessor.checkboxSetChecked(401, m_GameOption.bSoundBGM,         m_accessor.user);
            m_accessor.checkboxSetChecked(402, m_GameOption.bSoundEnvironment, m_accessor.user);
        }
        if (m_accessor.guageSetCur) {
            m_accessor.guageSetCur(403, m_GameOption.nVolumnBGM,         m_accessor.user);
            m_accessor.guageSetCur(404, m_GameOption.nVolumnEnvironment, m_accessor.user);
        }
    }
}

void cOptionDialog::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy OnActionEvent.  Routes the OK / CANCEL /
    // RESET buttons + the chatmode/macromode + graphic pushup
    // toggles + the AUTOCONTROL checkbox.
    constexpr std::uint32_t kBtnClick = mxh::ui::legacy_window_event::kButtonClick;  // 1:1 with WE_BTNCLICK
    constexpr std::uint32_t kPushUp   = mxh::ui::legacy_window_event::kPushUp;       // 1:1 with WE_PUSHUP
    constexpr std::uint32_t kPushDown = mxh::ui::legacy_window_event::kPushDown;     // 1:1 with WE_PUSHDOWN
    constexpr std::uint32_t kChecked  = mxh::ui::legacy_window_event::kChecked;      // 1:1 with WE_CHECKED
    constexpr std::uint32_t kNotChk   = mxh::ui::legacy_window_event::kNotChecked;   // 1:1 with WE_NOTCHECKED
    if (we & kBtnClick) {
        if (lId == kOtiBtnOk) {
            // 1:1 with legacy: UpdateData(TRUE) + dispatch to
            // OPTIONMGR (mocked via m_applyCb) + close.
            UpdateData(/*bSave=*/true);
            if (m_applyCb) {
                m_applyCb(&m_GameOption, m_applyUser);
            }
            SetActive(false);
        } else if (lId == kOtiBtnCancel) {
            if (m_cancelCb) m_cancelCb(m_cancelUser);
            SetActive(false);
        } else if (lId == kOtiBtnReset) {
            // 1:1 with legacy: OPTIONMGR->SetDefaultOption +
            // m_GameOption = OPTIONMGR->GetGameOption() +
            // UpdateData(FALSE).
            if (m_defaultCb) m_defaultCb(&m_GameOption, m_defaultUser);
            UpdateData(/*bSave=*/false);
        }
    }
    if (we & kPushUp) {
        // 1:1 with legacy: chatmode/macromode pushup-btns +
        // graphic LOD / effect pushup-btns.
        if (lId == 120 || lId == 121) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(lId, true, m_accessor.user);
        }
        if (lId == 310 || lId == 311 || lId == 312 || lId == 313) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(lId, true, m_accessor.user);
        }
    }
    if (we & kPushDown) {
        // 1:1 with legacy: PUSHDOWN of one pushup-btn unpushes
        // the other in the same group.
        if (lId == 120) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(121, false, m_accessor.user);
        } else if (lId == 121) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(120, false, m_accessor.user);
        }
        if (lId == 310) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(311, false, m_accessor.user);
        } else if (lId == 311) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(310, false, m_accessor.user);
        }
        if (lId == 312) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(313, false, m_accessor.user);
        } else if (lId == 313) {
            if (m_accessor.pushupSetPush) m_accessor.pushupSetPush(312, false, m_accessor.user);
        }
    }
    if (we & kChecked) {
        if (lId == kOtiCbAutoControl) {
            DisableGraphicTab(true);
        }
    } else if (we & kNotChk) {
        if (lId == kOtiCbAutoControl) {
            DisableGraphicTab(false);
        }
    }
}

void cOptionDialog::DisableGraphicTab(bool bDisable) {
    // 1:1 with legacy DisableGraphicTab.  Flips the flag and
    // walks the 4 gamma / sight / shadow controls on the
    // graphic tab to dim them.  The actual cGuageBar +
    // cStatic + cPushupButton cWindow* walks are deferred
    // until cGuageBar is fully ported; the modern port just
    // records the flag.
    m_bGraphicTabDisabled = bDisable;
    const auto setDisabled = [bDisable](cWindow* window) {
        if (window) window->SetDisable(bDisable);
    };
    setDisabled(m_gbGamma); setDisabled(m_gbSight);
    setDisabled(m_cbShadowHero); setDisabled(m_cbShadowMonster);
    setDisabled(m_cbShadowOthers); setDisabled(m_pbBasicGraphic);
    setDisabled(m_pbDownGraphic); setDisabled(m_pbBasicEffect);
    setDisabled(m_pbOneEffect);
    // 1:1 quirk: legacy UpdateData(FALSE) calls
    //   DisableGraphicTab(m_GameOption.bAutoCtrl)
    // so the flag mirrors bAutoCtrl on every open.
    (void)kTabSheetGame; (void)kTabSheetChat;
    (void)kTabSheetGraphic; (void)kTabSheetSound;
}

}  // namespace mxh::ui
