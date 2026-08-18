// tests/unit/ui/cWindowManager_focus_test.cpp
// M-R6.2 focus chain 1:1 验证 — hand-rolled main-style 跟
// cResourceManager_test / cDialogLoader_test 一样避免跟 gtest_main 冲突
//
// 验证 cWindowManager::SetFocus / TabFocusNext / TabFocusPrev
// 1:1 跟老版 cWindowManager::SetFocus / TabFocusNext / TabFocusPrev
// (老版: cWindowManager.cpp::SetFocus + TabFocusNext 走 m_pFocusedDialogList)

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cWindow.hpp"
#include "mxh/ui/cWindowManager.hpp"

#include <iostream>
#include <memory>

int g_failures = 0;
int g_passes = 0;

#define EXPECT(cond, msg) do { \
    if (cond) { ++g_passes; } \
    else { ++g_failures; std::cerr << "FAIL: " << msg << " @ " << __FILE__ << ":" << __LINE__ << "\n"; } \
} while (0)

#define EXPECT_EQ(a, b, msg) do { \
    auto _a = (a); auto _b = (b); \
    if (_a == _b) { ++g_passes; } \
    else { ++g_failures; std::cerr << "FAIL: " << msg << " (got=" << _a << " want=" << _b << ") @ " << __FILE__ << ":" << __LINE__ << "\n"; } \
} while (0)

int main() {
    std::cout << "[cWindowManager_focus_test] M-R6.2 focus chain 1:1 verification\n";

    // Test 1: SetFocus marks focused + clears previous
    {
        mxh::ui::cWindowManager wm;
        auto dlg = std::make_unique<mxh::ui::cDialog>();
        dlg->Init(0, 0, 200, 100, nullptr, /*id=*/0);
        dlg->SetActive(true);  // 1:1 with legacy
        auto eb1 = std::make_unique<mxh::ui::cEditBox>();
        eb1->Init(10, 10, 80, 20, nullptr, nullptr, /*id=*/1);
        auto eb2 = std::make_unique<mxh::ui::cEditBox>();
        eb2->Init(100, 10, 80, 20, nullptr, nullptr, /*id=*/2);
        mxh::ui::cEditBox* raw_eb1 = eb1.get();
        mxh::ui::cEditBox* raw_eb2 = eb2.get();
        dlg->Add(std::move(eb1));
        dlg->Add(std::move(eb2));
        wm.AddDialog(std::move(dlg));

        wm.SetFocus(raw_eb1);
        EXPECT(raw_eb1->hasFocus(), "eb1 focused after SetFocus(eb1)");
        EXPECT(!raw_eb2->hasFocus(), "eb2 NOT focused");
        EXPECT_EQ(wm.focusedId(), 1, "focusedId == 1");

        wm.SetFocus(raw_eb2);
        EXPECT(!raw_eb1->hasFocus(), "eb1 NOT focused after SetFocus(eb2)");
        EXPECT(raw_eb2->hasFocus(), "eb2 focused");
        EXPECT_EQ(wm.focusedId(), 2, "focusedId == 2");
    }

    // Test 2: SetFocus same window is no-op (1:1 legacy)
    {
        mxh::ui::cWindowManager wm;
        auto dlg = std::make_unique<mxh::ui::cDialog>();
        dlg->Init(0, 0, 200, 100, nullptr, /*id=*/0);
        dlg->SetActive(true);  // 1:1 with legacy
        auto eb = std::make_unique<mxh::ui::cEditBox>();
        eb->Init(10, 10, 80, 20, nullptr, nullptr, /*id=*/7);
        mxh::ui::cEditBox* raw_eb = eb.get();
        dlg->Add(std::move(eb));
        wm.AddDialog(std::move(dlg));

        wm.SetFocus(raw_eb);
        EXPECT(raw_eb->hasFocus(), "eb focused");
        wm.SetFocus(raw_eb);
        EXPECT(raw_eb->hasFocus(), "eb still focused after same-window SetFocus");
        EXPECT_EQ(wm.focusedId(), 7, "focusedId still 7");
    }

    // Test 3: TabFocusNext cycles through edit boxes
    {
        mxh::ui::cWindowManager wm;
        auto dlg = std::make_unique<mxh::ui::cDialog>();
        dlg->Init(0, 0, 200, 100, nullptr, /*id=*/0);
        dlg->SetActive(true);  // 1:1 with legacy
        auto eb1 = std::make_unique<mxh::ui::cEditBox>();
        eb1->Init(10, 10, 80, 20, nullptr, nullptr, /*id=*/1);
        auto eb2 = std::make_unique<mxh::ui::cEditBox>();
        eb2->Init(100, 10, 80, 20, nullptr, nullptr, /*id=*/2);
        dlg->Add(std::move(eb1));
        dlg->Add(std::move(eb2));
        wm.AddDialog(std::move(dlg));

        wm.TabFocusNext();
        EXPECT_EQ(wm.focusedId(), 1, "Tab → first focusable (id=1)");
        wm.TabFocusNext();
        EXPECT_EQ(wm.focusedId(), 2, "Tab → second (id=2)");
        wm.TabFocusNext();
        EXPECT_EQ(wm.focusedId(), 2, "Tab past end stays (no wrap)");
    }

    // Test 4: TabFocusPrev reverses order
    {
        mxh::ui::cWindowManager wm;
        auto dlg = std::make_unique<mxh::ui::cDialog>();
        dlg->Init(0, 0, 200, 100, nullptr, /*id=*/0);
        dlg->SetActive(true);  // 1:1 with legacy
        auto eb1 = std::make_unique<mxh::ui::cEditBox>();
        eb1->Init(10, 10, 80, 20, nullptr, nullptr, /*id=*/1);
        auto eb2 = std::make_unique<mxh::ui::cEditBox>();
        eb2->Init(100, 10, 80, 20, nullptr, nullptr, /*id=*/2);
        // NOTE: cache raw_eb2 BEFORE AddDialog so the pointer survives the dlg move.
        // (The dlg unique_ptr becomes nullptr after the move; using dlg->childAt(1)
        // after AddDialog would deref nullptr and crash.)
        mxh::ui::cEditBox* raw_eb2 = eb2.get();
        dlg->Add(std::move(eb1));
        dlg->Add(std::move(eb2));
        wm.AddDialog(std::move(dlg));
        wm.SetFocus(raw_eb2);

        wm.TabFocusPrev();
        EXPECT_EQ(wm.focusedId(), 1, "Shift+Tab → first (id=1)");
        wm.TabFocusPrev();
        EXPECT_EQ(wm.focusedId(), 1, "Shift+Tab before start stays (no wrap)");
    }

    // Test 5: cButton + cEditBox are focusable
    {
        mxh::ui::cWindowManager wm;
        auto dlg = std::make_unique<mxh::ui::cDialog>();
        dlg->Init(0, 0, 300, 100, nullptr, /*id=*/0);
        dlg->SetActive(true);  // 1:1 with legacy
        auto btn = std::make_unique<mxh::ui::cButton>();
        btn->Init(10, 10, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr, /*id=*/10);
        auto eb = std::make_unique<mxh::ui::cEditBox>();
        eb->Init(50, 10, 80, 20, nullptr, nullptr, /*id=*/11);
        dlg->Add(std::move(btn));
        dlg->Add(std::move(eb));
        wm.AddDialog(std::move(dlg));

        wm.TabFocusNext();
        EXPECT_EQ(wm.focusedId(), 10, "Tab → button (id=10)");
        wm.TabFocusNext();
        EXPECT_EQ(wm.focusedId(), 11, "Tab → editbox (id=11)");
    }

    std::cout << "\n[cWindowManager_focus_test] PASS " << g_passes
              << " / FAIL " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
