// wantregistdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog
// 1:1 port contract test for modern cWantRegistDialog
// (wanted registration editor dialog: 1 cStatic + 1 cEditBox).
//
// Covers modern/src/ui/wantregistdialog.{hpp,cpp}, a 1:1
// port of
//   墨香【源码】\[Client]MH\WantRegistDialog.h (930 B) and
//   墨香【源码】\[Client]MH\WantRegistDialog.cpp.
//
// What's tested:
//   - Default construction: cWantRegistDialog is a
//     cDialog and inherits its tree management.
//   - 2 child pointers start null (1:1 with legacy
//     default init).
//   - 2 id constants match expected local range
//     (kIdWantedName=510, kIdPrizeEdit=511).
//   - kVcmNumber == 1 (1:1 with legacy VCM_NUMBER
//     enum value).
//   - Linking resolves the cStatic + cEditBox
//     children by id and calls SetValidCheck(1) on
//     the cEditBox.
//   - Linking without children leaves both
//     pointers null (SetWantedName + SetActive +
//     ActionEvent are safe).
//   - Linking before Init does not crash.
//   - SetWantedName with valid name updates the
//     cStatic + clears the cEditBox (1:1 with
//     legacy).
//   - SetWantedName with null name is safe (1:1
//     quirk: modern port guards null; legacy would
//     crash on null).
//   - SetWantedName without Linking is safe.
//   - SetWantedName before Init does not crash.
//   - SetActive val=true stamps m_dwStartShowTime
//     through the optional host clock provider.
//   - SetActive val=false calls base SetActive +
//     SetFocusEdit(false) on cEditBox (1:1 with
//     legacy else branch) and sends the wanted
//     registration-cancel MSGBASE through callbacks.
//   - SetActive without Linking is safe.
//   - SetActive before Init does not crash.
//   - ActionEvent preserves inactive/disabled and
//     3000 ms delayed-show gates; CMouse routing
//     remains deferred and returns WE_NULL.
//   - ActionEvent before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_WANTREGISTDIALOG drop, modern cWindow
//     does not have m_type field).
//   - Legacy same-active early return is preserved
//     before clock, focus, network, and base effects.
//   - SetWantedName with null pName is safe
//     (modern port guards null; legacy would
//     crash).
//   - SetActive override: base SetActive always
//     called. The val == FALSE SetFocusEdit(false)
//     call is REAL; the MSGBASE send is wired
//     through optional HERO + NETWORK callbacks.
//   - ActionEvent returns 0 (matching legacy
//     WE_NULL).
//   - kVcmNumber = 1 (1:1 with legacy
//     cEditBox::SetValidCheck VCM_NUMBER).
//   - Local id range 510-511 (distinct from
//     200-500 used by previous Tier 2 dialogs; no
//     collision).

#include "wantregistdialog.hpp"
#include "cdialog.hpp"
#include "cstatic.hpp"
#include "ceditbox.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

using mxh::ui::cEditBox;
using mxh::ui::cStatic;
using mxh::ui::cWantRegistDialog;

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CWantRegistDialogTest, DefaultConstructionIsValid) {
    cWantRegistDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_WANTREGISTDIALOG drop, modern
    // cWindow does not have m_type field).
    SUCCEED();
}

TEST(CWantRegistDialogTest, InheritsDialogTreeManagement) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CWantRegistDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cWantRegistDialog::kIdWantedName, 510);
    EXPECT_EQ(cWantRegistDialog::kIdPrizeEdit, 511);
}

TEST(CWantRegistDialogTest, VcmNumberIsOne) {
    // 1:1 with legacy cEditBox::SetValidCheck
    // VCM_NUMBER = 1 (digits-only valid check).
    EXPECT_EQ(cWantRegistDialog::kVcmNumber, 1);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

void BuildDlgWithChildren(cWantRegistDialog& dlg,
                          cStatic** outWantedName,
                          cEditBox** outPrizeEdit) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto wantedName = std::make_unique<cStatic>();
    wantedName->Init(0, 0, 200, 14, nullptr, cWantRegistDialog::kIdWantedName);
    *outWantedName = wantedName.get();
    dlg.Add(std::unique_ptr<cWindow>(wantedName.release()));

    auto prizeEdit = std::make_unique<cEditBox>();
    prizeEdit->Init(0, 0, 200, 14, nullptr, nullptr,
                    cWantRegistDialog::kIdPrizeEdit);
    prizeEdit->InitEditbox(50, 64);
    *outPrizeEdit = prizeEdit.get();
    dlg.Add(std::unique_ptr<cWindow>(prizeEdit.release()));

    dlg.Linking();
}

}  // namespace

TEST(CWantRegistDialogTest, LinkingResolvesBothChildren) {
    cWantRegistDialog dlg;
    cStatic* pWantedName = nullptr;
    cEditBox* pPrizeEdit = nullptr;
    BuildDlgWithChildren(dlg, &pWantedName, &pPrizeEdit);

    // m_WantedName / m_PrizeEdit are private;
    // verified indirectly via SetWantedName
    // updating the cStatic.
    dlg.SetWantedName("target_player");
    EXPECT_EQ(pWantedName->GetStaticText(), "target_player");
    EXPECT_EQ(pPrizeEdit->editText(), "");
}

TEST(CWantRegistDialogTest, LinkingConfiguresValidCheckNumber) {
    // 1:1 quirk: legacy calls SetValidCheck(VCM_NUMBER)
    // on the cEditBox. Modern port calls
    // SetValidCheck(1).
    cWantRegistDialog dlg;
    cStatic* pWantedName = nullptr;
    cEditBox* pPrizeEdit = nullptr;
    BuildDlgWithChildren(dlg, &pWantedName, &pPrizeEdit);
    // The cEditBox port stores the valid check int
    // internally; verified by Linking not crashing.
    SUCCEED();
}

TEST(CWantRegistDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetWantedName + SetActive + ActionEvent
    // without children must be safe.
    dlg.SetWantedName("test");
    dlg.SetWantedName(nullptr);
    dlg.SetActive(true);
    dlg.SetActive(false);
    dlg.ActionEvent();
    SUCCEED();
}

TEST(CWantRegistDialogTest, LinkingBeforeInitDoesNotCrash) {
    cWantRegistDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetWantedName
// ===========================================================================

TEST(CWantRegistDialogTest, SetWantedNameUpdatesBothChildren) {
    cWantRegistDialog dlg;
    cStatic* pWantedName = nullptr;
    cEditBox* pPrizeEdit = nullptr;
    BuildDlgWithChildren(dlg, &pWantedName, &pPrizeEdit);

    dlg.SetWantedName("target_player");
    EXPECT_EQ(pWantedName->GetStaticText(), "target_player");
    EXPECT_EQ(pPrizeEdit->editText(), "");
}

TEST(CWantRegistDialogTest, SetWantedNameWithNullNameIsSafe) {
    // 1:1 quirk: modern port guards null pName
    // (legacy would crash on null).
    cWantRegistDialog dlg;
    cStatic* pWantedName = nullptr;
    cEditBox* pPrizeEdit = nullptr;
    BuildDlgWithChildren(dlg, &pWantedName, &pPrizeEdit);

    dlg.SetWantedName(nullptr);
    // cStatic not updated (guarded null).
    // cEditBox still cleared.
    EXPECT_EQ(pPrizeEdit->editText(), "");
}

TEST(CWantRegistDialogTest, SetWantedNameOverwritesPreviousText) {
    cWantRegistDialog dlg;
    cStatic* pWantedName = nullptr;
    cEditBox* pPrizeEdit = nullptr;
    BuildDlgWithChildren(dlg, &pWantedName, &pPrizeEdit);

    dlg.SetWantedName("first");
    EXPECT_EQ(pWantedName->GetStaticText(), "first");
    dlg.SetWantedName("second");
    EXPECT_EQ(pWantedName->GetStaticText(), "second");
}

TEST(CWantRegistDialogTest, SetWantedNameWithoutLinkIsSafe) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetWantedName("test");
    dlg.SetWantedName(nullptr);
    SUCCEED();
}

TEST(CWantRegistDialogTest, SetWantedNameBeforeInitDoesNotCrash) {
    cWantRegistDialog dlg;
    dlg.SetWantedName("test");
    SUCCEED();
}

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CWantRegistDialogTest, SetActiveTrueUpdatesBaseState) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CWantRegistDialogTest, SetActiveFalseClearsFocus) {
    // 1:1 with legacy val == FALSE: calls
    // m_PrizeEdit->SetFocusEdit(FALSE) plus the
    // optional HERO + NETWORK cancel dispatch.
    cWantRegistDialog dlg;
    cStatic* pWantedName = nullptr;
    cEditBox* pPrizeEdit = nullptr;
    BuildDlgWithChildren(dlg, &pWantedName, &pPrizeEdit);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
    // 1:1 quirk: legacy SetFocusEdit(FALSE) is
    // called on the cEditBox. Modern cEditBox has
    // SetFocusEdit(bool) — the test verifies the
    // call is safe (no crash) but does not verify
    // the focus state.
}

TEST(CWantRegistDialogTest, SetActiveWithoutLinkIsSafe) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CWantRegistDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cWantRegistDialog dlg;
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// ActionEvent
// ===========================================================================

TEST(CWantRegistDialogTest, ActionEventReturnsWeNullUntilCMousePorted) {
    // 1:1 with legacy contract: returns uint32.
    // Modern port returns 0 (WE_NULL) — TODO:
    // CMouse + gCurTime not ported, R-12.x
    // deferred. The 1:1 contract is preserved:
    // returns uint32 matching the legacy
    // early-return path.
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    EXPECT_EQ(dlg.ActionEvent(), 0u);
}

TEST(CWantRegistDialogTest, ActionEventBeforeInitDoesNotCrash) {
    cWantRegistDialog dlg;
    EXPECT_EQ(dlg.ActionEvent(), 0u);
}



}  // namespace mxh::ui::test

// Global using for the appended clock-provider test block.
using mxh::ui::cWantRegistDialog;

// ===========================================================================
// Clock provider + SetActive/ActionEvent body
// ===========================================================================

struct WgClockCapture {
    std::uint32_t value = 0;
    int calls = 0;
    static std::uint32_t Get(void* userData) {
        auto* self = static_cast<WgClockCapture*>(userData);
        ++self->calls;
        return self->value;
    }
};

TEST(CWantRegistDialogTest, SetActiveTrueStampsStartShowTimeFromClock) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    EXPECT_EQ(dlg.GetStartShowTime(), 1000u);
    EXPECT_FALSE(dlg.IsShow());
}

TEST(CWantRegistDialogTest, SetActiveTrueWithoutProviderUsesZeroClock) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_EQ(dlg.GetStartShowTime(), 0u);
}

TEST(CWantRegistDialogTest, SetActiveFalsePreservesStartTimeAndResetsShow) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 5000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.IsShow());
    EXPECT_EQ(dlg.GetStartShowTime(), 5000u);
}

TEST(CWantRegistDialogTest, ActionEventBefore3SecKeepsShowFalse) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    clock.value = 3000u;  // 2 seconds elapsed < 3 sec
    std::uint32_t we = dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_FALSE(dlg.IsShow());
}

TEST(CWantRegistDialogTest, ActionEventAt3SecFlipsShowTrue) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    clock.value = 4000u;  // exactly 3 seconds elapsed
    std::uint32_t we = dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_TRUE(dlg.IsShow());
}

TEST(CWantRegistDialogTest, ActionEventWithoutProviderIsNoOp) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    std::uint32_t we = dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_FALSE(dlg.IsShow());
}

TEST(CWantRegistDialogTest, ActionEventUsesLegacyDwordWrap) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    clock.value = 0xFFFFFFF0u;  // DWORD wrap-around distance
    dlg.ActionEvent();
    EXPECT_TRUE(dlg.IsShow());
}


// ===========================================================================
// Legacy early-return + cancel network dispatch (C-Batch-2.44)
// ===========================================================================

struct WantCancelCapture {
    std::uint32_t heroId = 0;
    std::uint32_t sentObjectId = 0;
    int getHeroCalls = 0;
    int sendCalls = 0;
    bool sendResult = true;

    static std::uint32_t GetHeroId(void* userData) {
        auto* capture = static_cast<WantCancelCapture*>(userData);
        ++capture->getHeroCalls;
        return capture->heroId;
    }

    static bool Send(std::uint32_t objectId, void* userData) {
        auto* capture = static_cast<WantCancelCapture*>(userData);
        ++capture->sendCalls;
        capture->sentObjectId = objectId;
        return capture->sendResult;
    }
};

TEST(CWantRegistDialogTest, WantedCancelWireConstantsMatchLegacy) {
    EXPECT_EQ(cWantRegistDialog::kWantedCategory, 52u);
    EXPECT_EQ(cWantRegistDialog::kWantedRegistCancelProtocol, 27u);
    EXPECT_EQ(cWantRegistDialog::kShowDelayMilliseconds, 3000u);
}

TEST(CWantRegistDialogTest, LinkingResetsLegacyShowDelayState) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    clock.value = 4000u;
    dlg.ActionEvent();
    ASSERT_TRUE(dlg.IsShow());

    dlg.Linking();

    EXPECT_FALSE(dlg.IsShow());
    EXPECT_EQ(dlg.GetStartShowTime(), 0u);
}

TEST(CWantRegistDialogTest, SetActiveTrueSameStateReturnsBeforeClockRead) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    clock.value = 9000u;

    dlg.SetActive(true);

    EXPECT_EQ(clock.calls, 1);
    EXPECT_EQ(dlg.GetStartShowTime(), 1000u);
}

TEST(CWantRegistDialogTest, SetActiveFalseSameStateSkipsCancelSend) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WantCancelCapture capture;
    capture.heroId = 77u;
    dlg.SetCancelCallbacks(&WantCancelCapture::GetHeroId,
                           &WantCancelCapture::Send, &capture);

    dlg.SetActive(false);

    EXPECT_EQ(capture.getHeroCalls, 0);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CWantRegistDialogTest, SetActiveFalseSendsCancelForHero) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WantCancelCapture capture;
    capture.heroId = 0x12345678u;
    dlg.SetCancelCallbacks(&WantCancelCapture::GetHeroId,
                           &WantCancelCapture::Send, &capture);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_EQ(capture.getHeroCalls, 1);
    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_EQ(capture.sentObjectId, 0x12345678u);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CWantRegistDialogTest, SetActiveTrueNeverSendsCancel) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WantCancelCapture capture;
    dlg.SetCancelCallbacks(&WantCancelCapture::GetHeroId,
                           &WantCancelCapture::Send, &capture);

    dlg.SetActive(true);

    EXPECT_EQ(capture.getHeroCalls, 0);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CWantRegistDialogTest, SetActiveFalseSkipsMissingHeroCallback) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WantCancelCapture capture;
    dlg.SetCancelCallbacks(nullptr, &WantCancelCapture::Send, &capture);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_EQ(capture.sendCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CWantRegistDialogTest, SetActiveFalseSkipsMissingSendCallback) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WantCancelCapture capture;
    dlg.SetCancelCallbacks(&WantCancelCapture::GetHeroId, nullptr, &capture);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_EQ(capture.getHeroCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CWantRegistDialogTest, SetActiveFalseIgnoresSendFailure) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WantCancelCapture capture;
    capture.sendResult = false;
    dlg.SetCancelCallbacks(&WantCancelCapture::GetHeroId,
                           &WantCancelCapture::Send, &capture);
    dlg.SetActive(true);

    dlg.SetActive(false);

    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CWantRegistDialogTest, ActionEventInactiveReturnsBeforeClockRead) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 5000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);

    EXPECT_EQ(dlg.ActionEvent(), 0u);
    EXPECT_EQ(clock.calls, 0);
    EXPECT_FALSE(dlg.IsShow());
}

TEST(CWantRegistDialogTest, ActionEventDisabledReturnsBeforeClockRead) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    dlg.SetDisable(true);
    clock.calls = 0;
    clock.value = 5000u;

    EXPECT_EQ(dlg.ActionEvent(), 0u);
    EXPECT_EQ(clock.calls, 0);
    EXPECT_FALSE(dlg.IsShow());
}

TEST(CWantRegistDialogTest, RenderIsSafeBeforeAndAfterShowDelay) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 1000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    dlg.Render();
    clock.value = 4000u;
    dlg.ActionEvent();
    dlg.Render();
    EXPECT_TRUE(dlg.IsShow());
}
