// coptiondialog.cpp — modern port of 墨香 COptionDialog.

#include "mxh/ui/coptiondialog.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/cPushupButton.hpp"

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
    // 1:1 with legacy Linking.  The legacy walks the WINDOW_ID
    // tree to find a single cCheckBox on the graphic tab and
    // attach a tool-tip.  The modern port defers the WINDOW_ID
    // walk until cWindowManager is fully ported; tests
    // populate the children via SetChildWindowForTest.
}

void cOptionDialog::UpdateData(bool bSave) {
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
    constexpr std::uint32_t kBtnClick = 0x0001;  // 1:1 with WE_BTNCLICK
    constexpr std::uint32_t kPushUp   = 0x0002;  // 1:1 with WE_PUSHUP
    constexpr std::uint32_t kPushDown = 0x0004;  // 1:1 with WE_PUSHDOWN
    constexpr std::uint32_t kChecked  = 0x0010;  // 1:1 with WE_CHECKED
    constexpr std::uint32_t kNotChk   = 0x0020;  // 1:1 with WE_NOTCHECKED
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
    // 1:1 quirk: legacy UpdateData(FALSE) calls
    //   DisableGraphicTab(m_GameOption.bAutoCtrl)
    // so the flag mirrors bAutoCtrl on every open.
    (void)kTabSheetGame; (void)kTabSheetChat;
    (void)kTabSheetGraphic; (void)kTabSheetSound;
}

}  // namespace mxh::ui
