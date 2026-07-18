// mallnoticedialog.cpp — modern port of 墨香 CMallNoticeDialog (mall notice).
//
// 1:1 port body. See legacy `MallNoticeDialog.cpp` for the original.

#include "mallnoticedialog.hpp"

#include "cicondialog.hpp"
#include "cpushupbutton.hpp"
#include "cwindow.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

cMallNoticeDialog::cMallNoticeDialog() = default;
cMallNoticeDialog::~cMallNoticeDialog() = default;

void cMallNoticeDialog::ClearTestInjections() noexcept {
    s_addCalls = 0;
    s_onActionEventCalls = 0;
    s_shellExecuteCount = 0;
    s_lastShellUrl.clear();
    s_mallUrl = mallUrls::kElse;
}

void cMallNoticeDialog::Add(cWindow* window) {
    // 1:1 with legacy Add(cWindow* window):
    //   if (window->GetType() == WT_PUSHUPBUTTON)
    //       AddTabBtn(curIdx1++, (cPushupButton*)window);
    //   else if (window->GetType() == WT_DIALOG)
    //       AddTabSheet(curIdx2++, window);
    //   else
    //       cTabDialog::Add(window);
    //
    // 1:1 quirks:
    //   - legacy cWindow::GetType() returns WT_* (from
    //     cWindow::m_type). Modern cWindow has no m_type
    //     field (Phase 6 removed it, same as cTipBrowserDlg/
    //     cPetStateMiniDlg). Modern port: dispatch key
    //     switched from GetType() to dynamic_cast (cDialog*
    //     is the base of cTabDialog which is the base of
    //     cMallNoticeDialog — so a `dynamic_cast<cDialog*>`
    //     catches the WT_DIALOG branch, and
    //     `dynamic_cast<cPushupButton*>` catches the
    //     WT_PUSHUPBUTTON branch).
    //   - legacy curIdx1/curIdx2 are inherited from
    //     cTabDialog. Modern port uses cTabDialog::curIdx1_
    //     / cTabDialog::curIdx2_ (the protected fields
    //     added in 0.13.67 for 1:1 fidelity). Each Add
    //     increments the appropriate counter.
    //   - legacy `else` branch forwards to cTabDialog::Add
    //     (which forwards to cDialog::Add). Modern port:
    //     cTabDialog doesn't override Add (it inherits
    //     cWindow::Add, which is non-virtual), so we call
    //     cDialog::Add directly.
    ++s_addCalls;
    if (!window) { return; }
    if (auto* btn = dynamic_cast<cPushupButton*>(window)) {
        AddTabBtn(curIdx1_++, std::unique_ptr<cPushupButton>(btn));
    } else if (dynamic_cast<cDialog*>(window) != nullptr) {
        AddTabSheet(curIdx2_++, std::unique_ptr<cWindow>(window));
    } else {
        cDialog::Add(std::unique_ptr<cWindow>(window));
    }
}

void cMallNoticeDialog::OnActionEvent(std::int32_t lId, void* /*p*/,
                                      std::uint32_t we) {
    ++s_onActionEventCalls;

    // 1:1 with legacy OnActionEvent:
    //   if (we & WE_BTNCLICK) {
    //       if (lId == ITEM_MALLBTN) {
    //           //ShellExecute(NULL, NULL, "Iexplore.exe", "http://mall.darkstoryonline.com/", ...);
    //           #ifdef TAIWAN_LOCAL
    //               ShellExecute(..., "https://secure.tengwu.com.cn/ItemMall/web_product_main.asp", ...);
    //           #elif defined _JAPAN_LOCAL_
    //           #elif defined _HK_LOCAL_
    //           #else
    //               ShellExecute(..., "http://www.wldhmx.com/webshop.aspx", ...);
    //           #endif
    //       }
    //   }
    //
    // 1:1 quirks:
    //   - legacy `we & WE_BTNCLICK` (64) → modern
    //     `we == WindowEvent::LButtonClick` (4) per R-12
    //     (same as cPetStateMiniDlg/cCharStateDialog).
    //   - legacy ShellExecute stubbed no-op per modern
    //     port pattern. The URL is recorded in
    //     s_lastShellUrl for test inspection.
    //   - legacy locale branches: TAIWAN → tengwu URL,
    //     JP/HK → empty (no ShellExecute), else → wldhmx
    //     URL. Modern port: the URL is test-injectable via
    //     SetMallUrlForTesting. Default URL is the
    //     legacy else-branch URL. Production code would
    //     link a real locale-detection function.
    //   - legacy 1:1 commented-out ShellExecute
    //     (mall.darkstoryonline.com) preserved as 1:1
    //     quirk note, not ported.
    if (we == static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick)) {
        if (lId == kItemMallBtnId) {
            ++s_shellExecuteCount;
            s_lastShellUrl = s_mallUrl;
            // 1:1 quirk: legacy ShellExecute stubbed no-op
            // (Windows API + modern port not platform-bound).
        }
    }
}

} // namespace mxh::ui
