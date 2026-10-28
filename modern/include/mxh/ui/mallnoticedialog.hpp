// mallnoticedialog.hpp — modern port of 墨香 CMallNoticeDialog (mall notice).
//
// 1:1 port of legacy `CMallNoticeDialog` from
//   `墨香【源码】\[Client]MH\MallNoticeDialog.{h,cpp}`.
//
// CMallNoticeDialog is a cTabDialog subclass that adds the
// "open mall" web-launch button to the tab UI. The dialog
// dispatches `Add` based on window type (WT_PUSHUPBUTTON →
// tab btn, WT_DIALOG → tab sheet, else → cTabDialog::Add)
// and routes ITEM_MALLBTN clicks to ShellExecute with a
// locale-specific URL (TAIWAN/HK/JP/else).
//
// 1:1 contract preserved:
//   - Add(cWindow* window) override — branches on legacy
//     GetType() (WT_PUSHUPBUTTON → AddTabBtn(curIdx1++),
//     WT_DIALOG → AddTabSheet(curIdx2++), else → base Add).
//     Modern port: cWindow has no m_type/GetType() (Phase 6
//     removed). Pragmatic replacement via dynamic_cast
//     (cPushupButton → tab btn, cDialog → tab sheet, else
//     → base Add). Same end-state, slightly different
//     dispatch key.
//   - OnActionEvent(lId, p, we) — branches on
//     `we & WE_BTNCLICK`, then on lId == ITEM_MALLBTN.
//     Launches ShellExecute with a locale-specific URL
//     (TAIWAN/HK/JP/else). Modern port: ShellExecute is
//     stubbed no-op (Phase 6.x deferred). The URL is
//     preserved as a test-injectable constant for
//     production wiring.
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy cWindow::GetType() returns WT_*
//     from cWindow::m_type. Modern cWindow has no m_type
//     (Phase 6 removed as a 1:1 quirk, same pattern as
//     cTipBrowserDlg/cPetStateMiniDlg). Modern port: type
//     check is replaced by `dynamic_cast` (cPushupButton*
//     for tab btn, cDialog* for tab sheet, else base Add).
//     Same end-state (3-way branch), different dispatch key.
//   - 1:1 quirk: legacy curIdx1/curIdx2 are inherited from
//     cTabDialog. Modern port: uses the same cTabDialog
//     curIdx1_/curIdx2_ fields (1:1 fidelity). Add
//     increments them per-tab-add. After the dialog is
//     built, the counters stop incrementing (legacy
//     behavior: they wrap back to 0 only on InitTab).
//   - 1:1 quirk: legacy ITEM_MALLBTN comes from
//     WindowIDEnum.h. Modern port uses local
//     kItemMallBtnId=2200.
//   - 1:1 quirk: legacy ShellExecute uses
//     `<shellapi.h.>` (typo — extra `.`). Modern port does
//     NOT include the typo header (Phase 6 ShellExecute
//     stubbed no-op per modern port pattern).
//   - 1:1 quirk: legacy `#ifdef TAIWAN_LOCAL /
//     #elif _JAPAN_LOCAL_ / #elif _HK_LOCAL_ / #else`
//     locale branches with 3 distinct URLs (TAIWAN + else).
//     JP and HK branches are empty (no ShellExecute call).
//     Modern port: the URL is test-injectable via
//     `SetMallUrlForTesting(std::string)`. Default
//     URL is the legacy else-branch URL (wldhmx.com).
//     Production code would link a real locale-detection
//     function before the dialog lib.
//   - 1:1 quirk: legacy ctor + dtor are empty bodies.
//     Modern port uses `= default`.
//   - 1:1 quirk: legacy OnActionEvent has a 1:1 commented-
//     out ShellExecute call (`http://mall.darkstoryonline.com/`)
//     for the default locale. Modern port: documented
//     as a 1:1 quirk note, not ported (we use the
//     else-branch URL by default).
//   - 1:1 quirk: legacy OnActionEvent first checks
//     `we & WE_BTNCLICK` (legacy == 64). Modern port:
//     `we == WindowEvent::LButtonClick` (modern == 4)
//     per R-12 (same as cPetStateMiniDlg/cCharStateDialog).
//   - 1:1 quirk: legacy OnActionEvent has no default
//     branch in the inner if. Modern port: preserves
//     the implicit default (no log, no error).
//   - 1:1 quirk: legacy ctor + dtor are NOT virtual in
//     header but the dtor IS `virtual ~CMallNoticeDialog()`.
//     Modern port preserves `~cMallNoticeDialog() override`
//     (matches cDialog's virtual dtor).
//   - 1:1 quirk: legacy class does not override
//     `InitTab` / `SetActive` / `SetAbsXY` / `SelectTab` —
//     it inherits cTabDialog's behavior 1:1. Modern port
//     preserves the lack of overrides (no extra method
//     definitions).

#pragma once

#include "ctabdialog.hpp"

#include <cstdint>
#include <string>

namespace mxh::ui {

class cWindow;
class cPushupButton;

// 1:1 with legacy WindowIDEnum.h. Local id range to avoid
// coupling to the legacy header.
constexpr int kItemMallBtnId = 2200;

// 1:1 with legacy URL constants (from legacy #ifdef TAIWAN_LOCAL
// / #elif _JAPAN_LOCAL_ / #elif _HK_LOCAL_ / #else branches).
// Production code would link a real locale-detection function
// before the dialog lib (or replace these constants at compile
// time).
namespace mallUrls {
constexpr const char* kTaiwan = "https://secure.tengwu.com.cn/ItemMall/web_product_main.asp";
constexpr const char* kJapan  = "";  // 1:1 quirk: legacy #elif _JAPAN_LOCAL_ branch is empty
constexpr const char* kHk     = "";  // 1:1 quirk: legacy #elif _HK_LOCAL_ branch is empty
constexpr const char* kElse   = "http://www.wldhmx.com/webshop.aspx";
// 1:1 quirk: legacy `//ShellExecute(..., "http://mall.darkstoryonline.com/", ...)` is
// the default-else commented-out URL. Modern port does not
// preserve it (we use kElse by default).
}  // namespace mallUrls

class cMallNoticeDialog : public cTabDialog {
public:
    cMallNoticeDialog();
    ~cMallNoticeDialog() override;

    cMallNoticeDialog(const cMallNoticeDialog&) = delete;
    cMallNoticeDialog& operator=(const cMallNoticeDialog&) = delete;

    // 1:1 with legacy surface.
    void Add(cWindow* window);  // 1:1 quirk: legacy `virtual void Add(cWindow*)`
                                // override. Modern port: cWindow::Add is non-virtual
                                // (per cWindow.hpp line 110-111), so the override is
                                // a "new" rather than an override. Same end-state
                                // (3-way branch) — see the cpp for the dispatch key
                                // (dynamic_cast vs legacy GetType).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // Test accessors.
    std::uint32_t addCallCount()    const noexcept { return s_addCalls; }
    std::uint32_t onActionEventCallCount() const noexcept { return s_onActionEventCalls; }
    std::uint32_t shellExecuteCount() const noexcept { return s_shellExecuteCount; }
    const std::string& lastShellUrl() const noexcept { return s_lastShellUrl; }

    // Test-injectable mall URL (default: legacy else-branch URL).
    // Production code would link a real locale-detection function.
    static void SetMallUrlForTesting(const std::string& url) noexcept { s_mallUrl = url; }
    static const std::string& mallUrlForTesting() noexcept { return s_mallUrl; }
    static void ClearTestInjections() noexcept;

private:
    // Test-injectable state.
    static inline std::uint32_t s_addCalls = 0;
    static inline std::uint32_t s_onActionEventCalls = 0;
    static inline std::uint32_t s_shellExecuteCount = 0;
    static inline std::string s_lastShellUrl;
    static inline std::string s_mallUrl = mallUrls::kElse;
};

} // namespace mxh::ui
