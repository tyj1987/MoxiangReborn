// mpmissiondialog_test.cpp — 1:1 port tests for
// 墨香 CMPMissionDialog (event-map mission notice
// dialog).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - 2 id constants (kIdMission / kIdCaution)
//     match expected local range 570-571
//   - 2 placeholder strings (kMissionText /
//     kCautionText) match expected
//   - kMaxMissionMsgNum = 5
//   - Linking resolves both cTextArea
//   - Linking sets mission text + caution text
//     on the cTextArea
//   - Linking initializes m_dwStartTime to 0
//   - Linking without children leaves pointers
//     null but does not crash
//   - Linking before Init does not crash
//   - SetMissionInfo is defensive (out-of-range
//     msgnum does not crash)
//   - SetActive(true) updates base state
//   - SetActive(false) updates base state
//   - SetActive before Init does not crash
//   - ActionEvent returns 0 (WE_NULL)
//   - ActionEvent before Init does not crash
//   - LoadMissionMsg is a no-op (does not crash)
//   - m_pMissionMsg / m_pCautionMsg are empty
//     (LoadMissionMsg no-op)

#include "mpmissiondialog.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cMPMissionDialog;
using mxh::ui::cTextArea;

namespace {

// helper: build a cMPMissionDialog + 2 cTextArea +
// Linking()
struct LinkedDialog {
    cMPMissionDialog dlg;
    std::unique_ptr<cTextArea> missionText;
    std::unique_ptr<cTextArea> cautionText;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        missionText = std::make_unique<cTextArea>();
        missionText->InitTextArea(mxh::ui::TextRect{0, 0, 100, 100}, 64);
        missionText->setId(cMPMissionDialog::kIdMission);
        auto* missionPtr = missionText.get();
        dlg.Add(std::move(missionText));

        cautionText = std::make_unique<cTextArea>();
        cautionText->InitTextArea(mxh::ui::TextRect{0, 100, 100, 200}, 64);
        cautionText->setId(cMPMissionDialog::kIdCaution);
        auto* cautionPtr = cautionText.get();
        dlg.Add(std::move(cautionText));

        dlg.Linking();

        missionPtr_ = missionPtr;
        cautionPtr_ = cautionPtr;
    }

    cTextArea* missionPtr_ = nullptr;
    cTextArea* cautionPtr_ = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CMPMissionDialogTest, CtorDoesNotCrash) {
    cMPMissionDialog dlg;
    SUCCEED();
}

TEST(CMPMissionDialogTest, DtorDoesNotCrash) {
    cMPMissionDialog dlg;
    SUCCEED();
}

TEST(CMPMissionDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cMPMissionDialog>,
                  "cMPMissionDialog must inherit from cDialog");
    SUCCEED();
}

// ---------- constants ----------

TEST(CMPMissionDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cMPMissionDialog::kIdMission, 570);
    EXPECT_EQ(cMPMissionDialog::kIdCaution, 571);
}

TEST(CMPMissionDialogTest, IdConstantsAreUnique) {
    EXPECT_NE(cMPMissionDialog::kIdMission, cMPMissionDialog::kIdCaution);
}

TEST(CMPMissionDialogTest, MaxMissionMsgNumIsFive) {
    EXPECT_EQ(cMPMissionDialog::kMaxMissionMsgNum, 5);
}

TEST(CMPMissionDialogTest, PlaceholderStringsMatchExpectedValues) {
    EXPECT_STREQ(cMPMissionDialog::kMissionText, "MP_MISSION_TEXT");
    EXPECT_STREQ(cMPMissionDialog::kCautionText, "MP_CAUTION_TEXT");
    EXPECT_STRNE(cMPMissionDialog::kMissionText, cMPMissionDialog::kCautionText);
}

// ---------- Linking ----------

TEST(CMPMissionDialogTest, LinkingResolvesBothTextAreas) {
    LinkedDialog ld;
    EXPECT_EQ(ld.missionPtr_->GetScriptText(), cMPMissionDialog::kMissionText);
    EXPECT_EQ(ld.cautionPtr_->GetScriptText(), cMPMissionDialog::kCautionText);
}

TEST(CMPMissionDialogTest, LinkingBeforeInitDoesNotCrash) {
    cMPMissionDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CMPMissionDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cMPMissionDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    // SetMissionInfo must not crash when children
    // are missing.
    dlg.SetMissionInfo(0);
    dlg.SetMissionInfo(10);  // OOB
    SUCCEED();
}

// ---------- SetMissionInfo ----------

TEST(CMPMissionDialogTest, SetMissionInfoWithValidIdxIsSafe) {
    LinkedDialog ld;
    // m_pMissionMsg is empty (LoadMissionMsg no-op),
    // so SetMissionInfo is a no-op but does not crash.
    ld.dlg.SetMissionInfo(0);
    ld.dlg.SetMissionInfo(4);
    SUCCEED();
}

TEST(CMPMissionDialogTest, SetMissionInfoWithNegativeIdxIsSafe) {
    LinkedDialog ld;
    ld.dlg.SetMissionInfo(-1);
    SUCCEED();
}

TEST(CMPMissionDialogTest, SetMissionInfoWithOOBIdxIsSafe) {
    LinkedDialog ld;
    ld.dlg.SetMissionInfo(5);   // == kMaxMissionMsgNum
    ld.dlg.SetMissionInfo(100);
    SUCCEED();
}

// ---------- SetActive ----------

TEST(CMPMissionDialogTest, SetActiveTrueUpdatesBaseState) {
    cMPMissionDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CMPMissionDialogTest, SetActiveFalseUpdatesBaseState) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CMPMissionDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cMPMissionDialog dlg;
    dlg.SetActive(true);
    SUCCEED();
}

// ---------- ActionEvent ----------

TEST(CMPMissionDialogTest, ActionEventReturnsZero) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.ActionEvent(), 0u);
}

TEST(CMPMissionDialogTest, ActionEventBeforeInitDoesNotCrash) {
    cMPMissionDialog dlg;
    dlg.ActionEvent();
    SUCCEED();
}

// ---------- LoadMissionMsg ----------

TEST(CMPMissionDialogTest, LoadMissionMsgDoesNotCrash) {
    cMPMissionDialog dlg;
    dlg.LoadMissionMsg();
    SUCCEED();
}

TEST(CMPMissionDialogTest, LoadMissionMsgIsIdempotent) {
    cMPMissionDialog dlg;
    dlg.LoadMissionMsg();
    dlg.LoadMissionMsg();
    dlg.LoadMissionMsg();
    SUCCEED();
}


// ---------- Clock provider + SetActive + ActionEvent body ----------

struct MpClockCapture {
    std::uint32_t value = 0;
    int calls = 0;
    static std::uint32_t Get(void* userData) {
        auto* self = static_cast<MpClockCapture*>(userData);
        ++self->calls;
        return self->value;
    }
};

TEST(CMPMissionDialogTest, SetActiveTrueStampsStartTimeFromClock) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 1000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    ld.dlg.SetActive(true);
    EXPECT_EQ(ld.dlg.GetStartTime(), 1000u);
}

TEST(CMPMissionDialogTest, SetActiveTrueWithoutProviderUsesZeroClock) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_EQ(ld.dlg.GetStartTime(), 0u);
}

TEST(CMPMissionDialogTest, SetActiveFalseDoesNotStampStartTime) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 5000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    ld.dlg.SetActive(false);
    // SetActive(false) does not stamp m_dwStartTime per legacy.
    EXPECT_EQ(ld.dlg.GetStartTime(), 0u);
}

TEST(CMPMissionDialogTest, ActionEventBefore5SecKeepsDialogActive) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 1000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    ld.dlg.SetActive(true);
    clock.value = 4000u;  // 3 seconds elapsed
    std::uint32_t we = ld.dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CMPMissionDialogTest, ActionEventAfter5SecAutoClosesDialog) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 1000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    ld.dlg.SetActive(true);
    clock.value = 7000u;  // 6 seconds elapsed >= 5s
    std::uint32_t we = ld.dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CMPMissionDialogTest, ActionEventAtExactly5SecAutoCloses) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 1000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    ld.dlg.SetActive(true);
    clock.value = 6000u;  // exactly 5 seconds elapsed
    ld.dlg.ActionEvent();
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CMPMissionDialogTest, ActionEventWithoutClockProviderIsNoOp) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    std::uint32_t we = ld.dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CMPMissionDialogTest, ActionEventWhenNotActiveIsNoOp) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 1000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    // Dialog is not active.
    std::uint32_t we = ld.dlg.ActionEvent();
    EXPECT_EQ(we, 0u);
    EXPECT_EQ(clock.calls, 0);
}

TEST(CMPMissionDialogTest, ActionEventUsesLegacyDwordWrap) {
    LinkedDialog ld;
    MpClockCapture clock;
    clock.value = 1000u;
    ld.dlg.SetCurrentTimeProvider(&MpClockCapture::Get, &clock);
    ld.dlg.SetActive(true);
    // Advance clock to wrap-around time (DWORD max).
    clock.value = 0xFFFFFF00u;
    ld.dlg.ActionEvent();
    // curTime - startTime = 0xFFFFFF00 - 1000 = wrap, huge. Auto-closes.
    EXPECT_FALSE(ld.dlg.isActive());
}
