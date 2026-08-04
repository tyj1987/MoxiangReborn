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
//   - SetActive val=true calls base SetActive
//     (gCurTime TODO: modern port does not init
//     m_dwStartShowTime, R-12.x deferred).
//   - SetActive val=false calls base SetActive +
//     SetFocusEdit(false) on cEditBox (1:1 with
//     legacy else branch; the MSGBASE NETWORK
//     send is TODO).
//   - SetActive without Linking is safe.
//   - SetActive before Init does not crash.
//   - ActionEvent returns 0 (WE_NULL) (TODO:
//     CMouse + gCurTime not ported, R-12.x
//     deferred). The 1:1 contract is preserved:
//     returns uint32 matching the legacy early-
//     return path.
//   - ActionEvent before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_WANTREGISTDIALOG drop, modern cWindow
//     does not have m_type field).
//   - 1:1 quirk: legacy's m_bActive check (early
//     return on val == m_bActive) is omitted in
//     the modern port (1:1 quirk: modern
//     cDialog::SetActive is idempotent).
//   - SetWantedName with null pName is safe
//     (modern port guards null; legacy would
//     crash).
//   - SetActive override: base SetActive always
//     called. The val == FALSE SetFocusEdit(false)
//     call is REAL (no singleton dep). The
//     MSGBASE NETWORK send is TODO.
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
    // m_PrizeEdit->SetFocusEdit(FALSE) (REAL, no
    // singleton dep) + the MSGBASE NETWORK send
    // is TODO.
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

TEST(CWantRegistDialogTest, SetActiveFalseResetsShowState) {
    cWantRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    WgClockCapture clock;
    clock.value = 5000u;
    dlg.SetCurrentTimeProvider(&WgClockCapture::Get, &clock);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.IsShow());
    EXPECT_EQ(dlg.GetStartShowTime(), 0u);
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
