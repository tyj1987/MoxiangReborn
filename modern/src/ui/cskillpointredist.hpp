// cskillpointredist.hpp -- modern port of Moxiang
//   CSkillPointRedist (skill-point redistribution
//   dialog: 3 tabs of ability icons).
//
// 1:1 port of legacy CSkillPointRedist from
//   [Client]MH/SkillPointRedist.{h,cpp}.
//
// Surface (legacy):
//   - 3 cButton children (Up, Down, Ok) resolved
//     in Linking via GetWindowForID(SK_UPBTN /
//     SK_DOWNBTN / SK_OKBTN).
//   - 3 cStatic children (RePointst, UsePointst,
//     OgPointst) resolved in Linking via
//     GetWindowForID(SK_POINTSTATIC /
//     SK_USESTATIC / SK_ORIGINALSTATIC).
//   - 3 cPushupButton + 3 cIconGridDialog children
//     indexed by tab (0=War, 1=KyungGong,
//     2=Character) resolved in Linking via
//     GetWindowForID(SK_POINTAGAIN{1,2,3}BTN) +
//     GetWindowForID(SK_ICONGRID{1,2,3}).
//   - SetActive override: forwards to
//     cDialog::SetActive(val); on deactivation,
//     disables self + SuryunDialog and ends the
//     hero deal-state via OBJECTSTATEMGR; on
//     activation calls RefreshAbilityIcons().
//   - RefreshAbilityIcons(): iterates the 40
//     ability icons in the current tab, populating
//     the cIconGridDialog via MakeNewAbilityIcon.
//   - MakeNewAbilityIcon(CAbilityInfo*): produces a
//     CAbilityIcon for the given ability info.
//   - SetAbilityToolTip(CAbilityIcon*): attaches the
//     ability tooltip to the icon.
//   - SetAbilitySyn(BOOL bDown): sends the
//     ability-point add / remove MSG to the server.
//   - SetAbilityExp(DWORD Exp): updates the cached
//     current ability exp.
//   - RefreshAbilityPoint(): refreshes the 3
//     cStatic point readouts (RePointst / UsePointst /
//     OgPointst).
//   - GetCurAbilityName(): returns the name of the
//     currently focused ability.
//   - GetCurAbilityLevel(): returns the level of the
//     currently focused ability.
//   - GetCurItemIdx() / GetCurItemPos(): return the
//     cached m_ItemIdx / m_ItemPos.
//   - SetCurItem(Idx, Pos): sets m_ItemIdx / m_ItemPos.
//   - SetTabNumber(dwTab) / GetTabNumber(): switch
//     and read the current tab.
//   - GetCurAbilityInfo(): returns the CAbilityInfo*
//     for the currently focused ability.
//
// Modern port:
//   - Inherits cDialog (1:1 with legacy).
//   - enum SkillRedistTab { War=0, KyungGong=1,
//     Character=2, Max=3 } (1:1 with legacy eTab_*).
//   - AbilityInfo / AbilityIcon are R-12.x deferred
//     (not yet ported to modern); modern port uses
//     void* for the deferred members and forwards
//     all R-12.x operations through host-injected
//     callbacks.
//   - 3 cButton (Up / Down / Ok) + 3 cStatic
//     (Re / Use / Og) + 3 cPushupButton (tab buttons)
//     + 3 cIconGridDialog (tab grids): all storage
//     slots are preserved 1:1 with legacy.
//   - Linking() resolves the 12 child windows via
//     the host-injected window-resolver callback
//     (modern port preserves the 1:1 lookup contract
//     even though the actual GetWindowForID is
//     deferred to the host).
//   - SetActive override: forwards to base; on
//     deactivation calls the host-injected close
//     callback; on activation calls the host-
//     injected refresh-icons callback.
//   - All R-12.x deferred surfaces (RefreshAbilityIcons,
//     MakeNewAbilityIcon, SetAbilityToolTip,
//     SetAbilitySyn, SetAbilityExp, RefreshAbilityPoint,
//     GetCurAbilityName, GetCurAbilityLevel,
//     GetCurAbilityInfo, SetTabNumber) are routed
//     through host-injected callbacks.
//   - GetCurItemIdx / GetCurItemPos / SetCurItem /
//     GetTabNumber are pure 1:1 storage wrappers.
//   - No m_type assignment (modern cWindow has no
//     m_type field, removed in Phase 6).
//
// 1:1 quirks:
//   - 1:1 with legacy 12 child windows (3 buttons +
//     3 statics + 3 tab buttons + 3 grids).
//   - 1:1 with legacy 4-value tab enum.
//   - 1:1 with legacy Linking order (buttons,
//     statics, then tab loop).
//   - 1:1 with legacy SetActive override that
//     forwards to cDialog::SetActive(val).
//   - 1:1 with legacy m_ItemIdx / m_ItemPos /
//     m_dwCurTabNum storage.
//   - 1:1 with legacy SkillIdx[100/200/400] array
//     (1:1 with legacy AbilityIdx[eTab_Max]).
//   - 1:1 with legacy default tab = eTab_War.
//

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;
class cStatic;
class cPushupButton;
class cIconGridDialog;

// 1:1 with legacy enum { eTab_War=0, eTab_KyungGong=1,
// eTab_Character=2, eTab_Max=3 }.
enum class SkillRedistTab : std::uint8_t {
    War       = 0,
    KyungGong = 1,
    Character = 2,
    Max       = 3,
};

class cSkillPointRedist : public cDialog {
public:
    // 1:1 with legacy kTabCount (= eTab_Max = 3).
    static constexpr std::size_t kTabCount = 3;

    cSkillPointRedist();
    ~cSkillPointRedist() override;

    cSkillPointRedist(const cSkillPointRedist&) = delete;
    cSkillPointRedist& operator=(const cSkillPointRedist&) = delete;

    // 1:1 with legacy SetActive override.  Forwards
    // to cDialog::SetActive(val); on deactivation
    // calls the host close callback; on activation
    // calls the host refresh-icons callback.
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy Linking.  Resolves the 12
    // child windows via the host-injected window-
    void Linking();

    // 1:1 with legacy RefreshAbilityIcons.
    // Forwards to host-injected callback (R-12.x).
    void RefreshAbilityIcons();

    // 1:1 with legacy MakeNewAbilityIcon.
    // Forwards to host-injected callback (R-12.x).
    void* MakeNewAbilityIcon(void* pInfo);

    // 1:1 with legacy SetAbilityToolTip.
    // Forwards to host-injected callback (R-12.x).
    void SetAbilityToolTip(void* pIcon);

    // 1:1 with legacy SetAbilitySyn.
    // Forwards to host-injected callback (R-12.x).
    void SetAbilitySyn(bool bDown);

    // 1:1 with legacy SetAbilityExp.
    void SetAbilityExp(std::uint32_t exp) noexcept;

    // 1:1 with legacy RefreshAbilityPoint.
    // Forwards to host-injected callback (R-12.x).
    void RefreshAbilityPoint();

    // 1:1 with legacy GetCurAbilityName.  Returns
    // the name from the host-injected callback.
    const char* GetCurAbilityName();

    // 1:1 with legacy GetCurAbilityLevel.  Returns
    // the level from the host-injected callback.
    int GetCurAbilityLevel();

    // 1:1 with legacy GetCurItemIdx.
    std::uint32_t GetCurItemIdx() const noexcept;

    // 1:1 with legacy GetCurItemPos.
    std::uint32_t GetCurItemPos() const noexcept;

    // 1:1 with legacy SetCurItem.
    void SetCurItem(std::uint32_t idx, std::uint32_t pos) noexcept;

    // 1:1 with legacy SetTabNumber.
    void SetTabNumber(std::uint32_t dwTab) noexcept;

    // 1:1 with legacy GetTabNumber.
    std::uint32_t GetTabNumber() const noexcept;

    // 1:1 with legacy GetCurAbilityInfo.  Returns
    // the CAbilityInfo* from the host-injected
    // callback.
    void* GetCurAbilityInfo();

    // ---- 1:1 id constants (legacy WindowIDs.h) ----
    // 1:1 with legacy SK_POINTDLG.
    static constexpr std::int32_t kIdDialog      = 1288;
    // 1:1 with legacy SK_POINTAGAIN{1,2,3}BTN.
    static constexpr std::int32_t kIdTabBtn0     = 1289;
    static constexpr std::int32_t kIdTabBtn1     = 1290;
    static constexpr std::int32_t kIdTabBtn2     = 1291;
    // 1:1 with legacy SK_ICONGRID{1,2,3}.
    static constexpr std::int32_t kIdIconGrid0   = 1292;
    static constexpr std::int32_t kIdIconGrid1   = 1293;
    static constexpr std::int32_t kIdIconGrid2   = 1294;
    // 1:1 with legacy SK_POINTSTATIC /
    // SK_USESTATIC / SK_ORIGINALSTATIC.
    static constexpr std::int32_t kIdRePoint     = 1295;
    static constexpr std::int32_t kIdUsePoint    = 1296;
    static constexpr std::int32_t kIdOgPoint     = 1297;
    // 1:1 with legacy SK_UPBTN / SK_DOWNBTN / SK_OKBTN.
    static constexpr std::int32_t kIdUpBtn       = 1298;
    static constexpr std::int32_t kIdDownBtn     = 1299;
    static constexpr std::int32_t kIdOkBtn       = 1300;

    // 1:1 with legacy AbilityIdx[eTab_Max] = {100,200,400}.
    static constexpr std::uint32_t kAbilityIdx[kTabCount] = {100, 200, 400};

    // ---- Test hooks ----
    // 1:1 with legacy m_UpBtn / m_DownBtn / m_OkBtn.
    void SetUpBtnForTest(cButton* b) noexcept;
    void SetDownBtnForTest(cButton* b) noexcept;
    void SetOkBtnForTest(cButton* b) noexcept;
    cButton* GetUpBtnForTest() const noexcept;
    cButton* GetDownBtnForTest() const noexcept;
    cButton* GetOkBtnForTest() const noexcept;

    // 1:1 with legacy m_RePointst / m_UsePointst / m_OgPointst.
    void SetRePointForTest(cStatic* s) noexcept;
    void SetUsePointForTest(cStatic* s) noexcept;
    void SetOgPointForTest(cStatic* s) noexcept;
    cStatic* GetRePointForTest() const noexcept;
    cStatic* GetUsePointForTest() const noexcept;
    cStatic* GetOgPointForTest() const noexcept;

    // 1:1 with legacy m_GridButton[eTab_Max] +
    // m_IconGrid[eTab_Max].
    void SetGridButtonForTest(std::size_t tab, cPushupButton* b) noexcept;
    void SetIconGridForTest(std::size_t tab, cIconGridDialog* g) noexcept;
    cPushupButton* GetGridButtonForTest(std::size_t tab) const noexcept;
    cIconGridDialog* GetIconGridForTest(std::size_t tab) const noexcept;

    // ---- Host-injected callbacks (R-12.x deferred surfaces) ----
    // 1:1 with legacy Linking window-resolver.
    using WindowResolver = void*(*)(std::int32_t id, void* user);
    void SetWindowResolverForTest(WindowResolver cb, void* user) noexcept;

    // 1:1 with legacy RefreshAbilityIcons loop.
    using RefreshIconsCallback = void(*)(void* user);
    void SetRefreshIconsCallbackForTest(RefreshIconsCallback cb, void* user) noexcept;

    // 1:1 with legacy MakeNewAbilityIcon.
    using MakeIconCallback = void*(*)(void* pInfo, void* user);
    void SetMakeIconCallbackForTest(MakeIconCallback cb, void* user) noexcept;

    // 1:1 with legacy SetAbilityToolTip.
    using SetToolTipCallback = void(*)(void* pIcon, void* user);
    void SetToolTipCallbackForTest(SetToolTipCallback cb, void* user) noexcept;

    // 1:1 with legacy SetAbilitySyn.
    using SetAbilitySynCallback = void(*)(bool bDown, void* user);
    void SetAbilitySynCallbackForTest(SetAbilitySynCallback cb, void* user) noexcept;

    // 1:1 with legacy RefreshAbilityPoint.
    using RefreshPointCallback = void(*)(void* user);
    void SetRefreshPointCallbackForTest(RefreshPointCallback cb, void* user) noexcept;

    // 1:1 with legacy GetCurAbilityName.
    using CurAbilityNameCallback = const char*(*)(void* user);
    void SetCurAbilityNameCallbackForTest(CurAbilityNameCallback cb, void* user) noexcept;

    // 1:1 with legacy GetCurAbilityLevel.
    using CurAbilityLevelCallback = int(*)(void* user);
    void SetCurAbilityLevelCallbackForTest(CurAbilityLevelCallback cb, void* user) noexcept;

    // 1:1 with legacy GetCurAbilityInfo.
    using CurAbilityInfoCallback = void*(*)(void* user);
    void SetCurAbilityInfoCallbackForTest(CurAbilityInfoCallback cb, void* user) noexcept;

    // 1:1 with legacy SetActive on-deactivate hook.
    using DeactivateCallback = void(*)(void* user);
    void SetDeactivateCallbackForTest(DeactivateCallback cb, void* user) noexcept;

private:
    // 1:1 with legacy child window members.
    cButton* m_upBtn = nullptr;
    cButton* m_downBtn = nullptr;
    cButton* m_okBtn = nullptr;
    cStatic* m_rePoint = nullptr;
    cStatic* m_usePoint = nullptr;
    cStatic* m_ogPoint = nullptr;
    cPushupButton* m_gridButton[kTabCount] = {nullptr};
    cIconGridDialog* m_iconGrid[kTabCount] = {nullptr};

    // 1:1 with legacy m_ItemIdx / m_ItemPos / m_dwCurTabNum.
    std::uint32_t m_itemIdx = 0;
    std::uint32_t m_itemPos = 0;
    std::uint32_t m_curTabNum = 0;

    // 1:1 with legacy m_AbilityExp cached field.
    std::uint32_t m_abilityExp = 0;

    // Host-injected callbacks + user pointers.
    WindowResolver m_windowResolverCb = nullptr;
    void* m_windowResolverUser = nullptr;
    RefreshIconsCallback m_refreshIconsCb = nullptr;
    void* m_refreshIconsUser = nullptr;
    MakeIconCallback m_makeIconCb = nullptr;
    void* m_makeIconUser = nullptr;
    SetToolTipCallback m_setToolTipCb = nullptr;
    void* m_setToolTipUser = nullptr;
    SetAbilitySynCallback m_setAbilitySynCb = nullptr;
    void* m_setAbilitySynUser = nullptr;
    RefreshPointCallback m_refreshPointCb = nullptr;
    void* m_refreshPointUser = nullptr;
    CurAbilityNameCallback m_curAbilityNameCb = nullptr;
    void* m_curAbilityNameUser = nullptr;
    CurAbilityLevelCallback m_curAbilityLevelCb = nullptr;
    void* m_curAbilityLevelUser = nullptr;
    CurAbilityInfoCallback m_curAbilityInfoCb = nullptr;
    void* m_curAbilityInfoUser = nullptr;
    DeactivateCallback m_deactivateCb = nullptr;
    void* m_deactivateUser = nullptr;
};

}  // namespace mxh::ui
