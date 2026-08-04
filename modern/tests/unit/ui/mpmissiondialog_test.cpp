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

// ===========================================================================

namespace {

struct MPMissionHostCalls {
    int showMPNoticeCalls = 0;
    int chatMsgCalls      = 0;
    std::int32_t lastChatMsgId = -1;

    static const char* GetChatMsg(std::int32_t msgId, void* ud) {
        auto* hc = static_cast<MPMissionHostCalls*>(ud);
        ++hc->chatMsgCalls;
        hc->lastChatMsgId = msgId;
        if (msgId == cMPMissionDialog::kChatMsgMission) {
            return "MP_MISSION_CUSTOM";
        }
        if (msgId == cMPMissionDialog::kChatMsgCaution) {
            return "MP_CAUTION_CUSTOM";
        }
        return "MP_OTHER";
    }

    static void ShowMPNoticeDialog(void* ud) {
        ++static_cast<MPMissionHostCalls*>(ud)->showMPNoticeCalls;
    }
};

}  // namespace

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

TEST(CMPMissionDialogTest, LinkingWithoutCallbacksUsesPlaceholderText) {
    // 1:1 with legacy CHATMGR placeholder:
    // when the host GetChatMessage callback
    // is absent, Linking falls back to the
    // kMissionText / kCautionText constants.
    LinkedDialog ld;
    EXPECT_EQ(ld.missionPtr_->GetScriptText(), cMPMissionDialog::kMissionText);
    EXPECT_EQ(ld.cautionPtr_->GetScriptText(), cMPMissionDialog::kCautionText);
}

TEST(CMPMissionDialogTest, LinkingWithChatCallbackOverridesPlaceholder) {
    // The host callback returns a custom
    // chat message per legacy id (665/666).
    LinkedDialog ld;
    MPMissionHostCalls hc;
    ld.dlg.SetCallbacks(&MPMissionHostCalls::GetChatMsg,
                         nullptr, &hc);
    ld.dlg.Linking();
    EXPECT_EQ(ld.missionPtr_->GetScriptText(), "MP_MISSION_CUSTOM");
    EXPECT_EQ(ld.cautionPtr_->GetScriptText(), "MP_CAUTION_CUSTOM");
}

TEST(CMPMissionDialogTest, LinkingRecalledUsesLatestCallback) {
    // Linking is idempotent: re-calling it
    // with a different chat-message callback
    // overrides the previous text.
    LinkedDialog ld;
    ld.dlg.SetCallbacks(nullptr, nullptr, nullptr);
    ld.dlg.Linking();
    EXPECT_EQ(ld.missionPtr_->GetScriptText(), cMPMissionDialog::kMissionText);
    MPMissionHostCalls hc;
    ld.dlg.SetCallbacks(&MPMissionHostCalls::GetChatMsg,
                         nullptr, &hc);
    ld.dlg.Linking();
    EXPECT_EQ(ld.missionPtr_->GetScriptText(), "MP_MISSION_CUSTOM");
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

TEST(CMPMissionDialogTest, SetActiveFalseClosesAndDispatchesShowMPNotice) {
    // 1:1 with legacy SetActive(false):
    // GAMEIN->GetMPNoticeDialog()->SetActive(TRUE)
    // dispatch runs BEFORE cDialog::SetActive
    // (matches legacy byte-for-byte). Verify
    // both the dispatch + the base close.
    LinkedDialog ld;
    MPMissionHostCalls hc;
    ld.dlg.SetCallbacks(nullptr,
                         &MPMissionHostCalls::ShowMPNoticeDialog,
                         &hc);
    ld.dlg.SetActive(true);
    ASSERT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_EQ(hc.showMPNoticeCalls, 1);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CMPMissionDialogTest, SetActiveFalseWithoutCallbackClosesDialog) {
    // No host callback -> null singleton
    // (GAMEIN not yet ported) -- the dialog
    // still closes (1:1 with legacy base
    // SetActive).
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    ASSERT_TRUE(ld.dlg.isActive());
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
// ===========================================================================
// Callback fixtures + new host-dispatch tests

TEST(CMPMissionDialogTest, LegacyChatMessageIdsAndAccessorsMatchSource) {
    // 1:1 with legacy CHATMGR->GetChatMsg ids.
    EXPECT_EQ(cMPMissionDialog::kChatMsgMission, 665);
    EXPECT_EQ(cMPMissionDialog::kChatMsgCaution, 666);
}

TEST(CMPMissionDialogTest, LinkingWithPartialChatCallbackFallsBackForMissingId) {
    // The host callback returns nullptr -- the
    // fallback (kMissionText) is used.
    LinkedDialog ld;
    struct AlwaysNull {
        static const char* Get(std::int32_t, void*) { return nullptr; }
    };
    ld.dlg.SetCallbacks(&AlwaysNull::Get, nullptr, nullptr);
    ld.dlg.Linking();
    EXPECT_EQ(ld.missionPtr_->GetScriptText(), cMPMissionDialog::kMissionText);
    EXPECT_EQ(ld.cautionPtr_->GetScriptText(), cMPMissionDialog::kCautionText);
}

TEST(CMPMissionDialogTest, SetActiveTrueDoesNotDispatchShowMPNotice) {
    // 1:1 with legacy: val==TRUE only stamps
    // m_dwStartTime -- the ShowMPNoticeDialog
    // dispatch runs only on val==FALSE.
    LinkedDialog ld;
    MPMissionHostCalls hc;
    ld.dlg.SetCallbacks(nullptr,
                         &MPMissionHostCalls::ShowMPNoticeDialog,
                         &hc);
    ld.dlg.SetActive(true);
    EXPECT_EQ(hc.showMPNoticeCalls, 0);
}

TEST(CMPMissionDialogTest, SetCallbacksReplacesExistingHostDispatch) {
    // First context then second context. Verify
    // the second context receives the dispatch.
    LinkedDialog ld;
    MPMissionHostCalls firstCtx;
    MPMissionHostCalls secondCtx;
    ld.dlg.SetCallbacks(nullptr,
                         &MPMissionHostCalls::ShowMPNoticeDialog,
                         &firstCtx);
    ld.dlg.SetActive(true);
    ld.dlg.SetActive(false);
    EXPECT_EQ(firstCtx.showMPNoticeCalls, 1);

    ld.dlg.SetActive(true);
    ld.dlg.SetCallbacks(nullptr,
                         &MPMissionHostCalls::ShowMPNoticeDialog,
                         &secondCtx);
    ld.dlg.SetActive(false);
    EXPECT_EQ(firstCtx.showMPNoticeCalls, 1);  // NOT incremented again
    EXPECT_EQ(secondCtx.showMPNoticeCalls, 1);
}

