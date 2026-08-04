// coptiondialog.hpp — modern port of 墨香 COptionDialog (option/setting dialog).
//
// 1:1 port of legacy `COptionDialog` from
//   `墨香【源码】\[Client]MH\OptionDialog.{h,cpp}`
// and the embedded `sGAMEOPTION` struct from
//   `墨香【源码】\[Client]MH\OptionManager.h`.
//
// The option dialog is a 4-tab cTabDialog (game / chat / graphic /
// sound).  Each tab contains a cDialog with cCheckBox / cPushupButton
// / cGuageBar children.  The dialog reads options from the
// global COptionManager on open (`UpdateData(FALSE)`) and writes
// them back on OK (`UpdateData(TRUE)` + OPTIONMGR->SetGameOption +
// ApplySettings + SendOptionMsg).
//
// The modern port keeps:
//   * sGAMEOPTION 1:1 with legacy field order + types
//   * Add() override routing PUSHBUTTON -> AddTabBtn,
//     DIALOG -> AddTabSheet, else base
//   * SetActive() override that calls UpdateData(FALSE) on enter
//     and notifies the host via a SetMainBarIconCallback
//   * OnActionEvent handling OK / CANCEL / RESET buttons
//   * DisableGraphicTab(BOOL) toggles a flag (the per-control
//     color / disable walk is deferred until cGuageBar port lands)
//   * GetEffectSnow getter (legacy: _JAPAN_LOCAL_-gated;
//     the modern port keeps the API surface for 1:1 source compat
//     without per-locale ifdef)
//
// The legacy code's OPTIONMGR / GameIn / MacroManager /
// MainBarDialog singleton dependencies are deferred (Phase 12+
// will wire the global singletons).  UpdateData() / DisableGraphicTab()
//   are public so unit tests can drive them directly with hand-injected
//   cCheckBox / cPushupButton / cGuageBar pointers via the
//   SetChildWindowsForTest hook.

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include "mxh/ui/legacy_window_event.hpp"

#include <cstdint>
#include <unordered_map>

namespace mxh::ui {

class cCheckBox;
class cPushupButton;

// 1:1 with legacy sGAMEOPTION (legacy OptionManager.h).
// The struct is laid out verbatim from the legacy source so the
// total byte count is preserved (sizeof(sGAMEOPTION) is locked
// via a static_assert in coptiondialog.cpp).
struct sGAMEOPTION {
    bool bNoDeal;          // 0x00
    bool bNoParty;         // 0x01
    bool bNoFriend;        // 0x02
    bool bNoVimu;          // 0x03
    bool bNameMunpa;       // 0x04
    bool bNameParty;       // 0x05
    bool bNameOthers;      // 0x06
    bool bNoMemberDamage;  // 0x07
    bool bNoGameTip;       // 0x08
    bool bMunpaIntro;      // 0x09 magi82

    int  nMacroMode;       // 0x0C

    bool bNoWhisper;       // 0x10
    bool bNoChatting;      // 0x11
    bool bNoBalloon;       // 0x12
    bool bAutoHide;        // 0x13
    bool bNoShoutChat;     // 0x14
    bool bNoGuildChat;     // 0x15
    bool bNoAllianceChat;  // 0x16
    bool bNoSystemMsg;     // 0x17
    bool bNoExpMsg;        // 0x18
    bool bNoItemMsg;       // 0x19

    int  nGamma;           // 0x1C
    int  nSightDistance;   // 0x20

    bool bGraphicCursor;   // 0x24

    bool bShadowHero;      // 0x28
    bool bShadowMonster;   // 0x29
    bool bShadowOthers;    // 0x2A
    // (legacy _JAPAN_LOCAL_ gates the next 4 fields off in
    // the JP build; modern port keeps them on every locale
    // for source compat)
    bool bAutoCtrl;        // 0x2B SW050822
    int  nLODMode;         // 0x2C
    int  nEffectMode;      // 0x30
    int  nEffectSnow;      // 0x34 2005.12.28

    bool bSoundBGM;              // 0x38
    bool bSoundEnvironment;      // 0x39
    int  nVolumnBGM;             // 0x3C
    int  nVolumnEnvironment;     // 0x40

    // SW060904
    bool bAmbientMax;            // 0x44
    // _KOR_LOCAL_ gate: int nLoginCombo; (not ported)
    bool bIntroFlag;             // 0x48 magi82
    bool bNoAvatarView;          // 0x49
};

class cOptionDialog : public cDialog {
public:
    cOptionDialog();
    ~cOptionDialog() override;

    cOptionDialog(const cOptionDialog&) = delete;
    cOptionDialog& operator=(const cOptionDialog&) = delete;

    // 1:1 with legacy Add(cWindow*).  Routes PUSHBUTTON -> tab
    // button slot, DIALOG -> tab sheet slot, else base.  The
    // modern cWindow::Add is non-virtual (it takes
    // std::unique_ptr<cWindow>); the legacy COptionDialog's
    // Add(cWindow*) is the same shape -- a new'd pointer is
    // passed in.  We hide the base Add here (the legacy
    // likewise doesn't `virtual` Add).
    void Add(cWindow* window);

    // 1:1 with legacy SetActive(BOOL).  Pulls option snapshot
    // from OPTIONMGR on enter, updates the main-bar option icon.
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy Linking.  No-op in the modern port until
    // cWindowManager port is complete (the legacy wires 4
    // tab sheets by walking the WINDOW_ID tree).
    void Linking();

    // 1:1 with legacy OnActionEvent(LONG, void*, DWORD).
    // Hides the (non-existent in cWindow) base OnActionEvent.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy DisableGraphicTab(BOOL).  Flips m_bGraphicTabDisabled
    // and dispatches a callback the host uses to dim the
    // gamma/sight/shadow controls.
    void DisableGraphicTab(bool bDisable);

    // 1:1 with legacy GetEffectSnow.
    int GetEffectSnow() const noexcept { return m_GameOption.nEffectSnow; }

    // Access the sGAMEOPTION snapshot (read/write).
    sGAMEOPTION&       gameOption()       noexcept { return m_GameOption; }
    const sGAMEOPTION& gameOption() const noexcept { return m_GameOption; }

    // 1:1 with legacy UpdateData(BOOL).  bSave=true walks the
    // tab sheets to read checkbox/guage values into m_GameOption;
    // bSave=false writes m_GameOption back into the tab sheet
    // children.  The legacy path requires OPTIONMGR /
    // cGuageBar / cCheckBox / cPushupButton, so the modern
    // port routes through a `WidgetAccessor` callback the host
    // provides.  See SetWidgetAccessorForTest.
    struct WidgetAccessor {
        // Read a checkbox's checked state by id.
        bool (*checkboxIsChecked)(std::int32_t id, void* user) = nullptr;
        // Set a checkbox's checked state by id.
        void (*checkboxSetChecked)(std::int32_t id, bool v, void* user) = nullptr;
        // Read a pushup button's pushed state by id.
        bool (*pushupIsPushed)(std::int32_t id, void* user) = nullptr;
        // Set a pushup button's pushed state by id.
        void (*pushupSetPush)(std::int32_t id, bool v, void* user) = nullptr;
        // Read a guage bar's current value by id.
        int  (*guageGetCur)(std::int32_t id, void* user) = nullptr;
        // Set a guage bar's current value by id.
        void (*guageSetCur)(std::int32_t id, int v, void* user) = nullptr;
        void* user = nullptr;
    };
    void SetWidgetAccessorForTest(const WidgetAccessor& w) { m_accessor = w; }

    void UpdateData(bool bSave);

    // Test hook -- inject the "OPTIONMGR Apply" callback (legacy
    // OPTIONMGR->SetGameOption + ApplySettings + SendOptionMsg).
    using ApplyCallback = void(*)(sGAMEOPTION*, void*);
    void SetApplyCallbackForTest(ApplyCallback cb, void* user) {
        m_applyCb = cb; m_applyUser = user;
    }

    // Test hook -- inject the "OPTIONMGR Cancel" callback (legacy
    // OPTIONMGR->CancelSettings).
    using CancelCallback = void(*)(void*);
    void SetCancelCallbackForTest(CancelCallback cb, void* user) {
        m_cancelCb = cb; m_cancelUser = user;
    }

    // Test hook -- inject the "OPTIONMGR Default" callback (legacy
    // OPTIONMGR->SetDefaultOption).
    using DefaultCallback = void(*)(sGAMEOPTION*, void*);
    void SetDefaultCallbackForTest(DefaultCallback cb, void* user) {
        m_defaultCb = cb; m_defaultUser = user;
    }

    // Test hook -- inject the "main-bar option icon" callback
    // (legacy GAMEIN->GetMainInterfaceDialog()->SetPushBarIcon).
    using MainBarIconCallback = void(*)(bool active, void* user);
    void SetMainBarIconCallbackForTest(MainBarIconCallback cb, void* user) {
        m_mainBarCb = cb; m_mainBarUser = user;
    }

    // 1:1 legacy button ids.
    static constexpr std::int32_t kOtiBtnOk      = 600;
    static constexpr std::int32_t kOtiBtnCancel  = 601;
    static constexpr std::int32_t kOtiBtnReset   = 602;
    static constexpr std::int32_t kOtiBtnPreview = 603;
    static constexpr std::int32_t kOtiBtnSetChat = 604;
    static constexpr std::int32_t kOtiBtnSetMacro = 605;

    static constexpr std::int32_t kOtiCbAutoControl = 700;

    // 1:1 with legacy cWindow::we constants.
    static constexpr std::uint32_t kWeBtnClick = legacy_window_event::kButtonClick;
    static constexpr std::uint32_t kWePushUp   = legacy_window_event::kPushUp;
    static constexpr std::uint32_t kWePushDown = legacy_window_event::kPushDown;
    static constexpr std::uint32_t kWeChecked  = legacy_window_event::kChecked;
    static constexpr std::uint32_t kWeNotChecked = legacy_window_event::kNotChecked;

    bool isGraphicTabDisabled() const noexcept { return m_bGraphicTabDisabled; }

private:
    sGAMEOPTION   m_GameOption{};
    WidgetAccessor m_accessor;
    std::unordered_map<std::int32_t, cWindow*> m_childById;
    ApplyCallback   m_applyCb   = nullptr;
    void*           m_applyUser = nullptr;
    CancelCallback  m_cancelCb   = nullptr;
    void*           m_cancelUser = nullptr;
    DefaultCallback m_defaultCb   = nullptr;
    void*           m_defaultUser = nullptr;
    MainBarIconCallback m_mainBarCb = nullptr;
    void*           m_mainBarUser = nullptr;
    bool           m_bGraphicTabDisabled = false;
    // 1:1 with legacy cTabDialog::curIdx1 / curIdx2 (the per-type
    // insertion cursors that COptionDialog::Add uses to route
    // PUSHBUTTON / DIALOG children into the right tab slot).
    int            m_curIdx1 = 0;
    int            m_curIdx2 = 0;
};

} // namespace mxh::ui
