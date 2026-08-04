// fortwartimedialog_test.cpp — 1:1 port verification tests for FortWar dialogs.

#include "fortwartimedialog.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cFWEngraveDialog;
using mxh::ui::cFWTimeDialog;

namespace {

std::unique_ptr<cFWEngraveDialog> MakeEngrave() {
    auto d = std::make_unique<cFWEngraveDialog>();
    d->Init(0, 0, 200, 100, nullptr, 779);
    return d;
}

std::unique_ptr<cFWTimeDialog> MakeTime() {
    auto d = std::make_unique<cFWTimeDialog>();
    d->Init(0, 0, 200, 100, nullptr, 778);
    return d;
}

// Captures every clock tick + call count for SetCurrentTimeProvider.
struct ClockCapture {
    std::uint32_t value = 0;
    int calls = 0;
    static std::uint32_t Get(void* userData) {
        auto* self = static_cast<ClockCapture*>(userData);
        ++self->calls;
        return self->value;
    }
};

// Captures every chat lookup + canned template.
struct ChatCapture {
    std::string template_str = "Remaining %d sec";
    int last_msg_id = -1;
    int calls = 0;
    static const char* Get(int msgId, void* userData) {
        auto* self = static_cast<ChatCapture*>(userData);
        ++self->calls;
        self->last_msg_id = msgId;
        return self->template_str.c_str();
    }
};


// Captures every send-engrave-cancel dispatch.
struct EngraveHostCalls {
    int callCount = 0;
    static void SendEngraveCancel(void* userData) {
        auto* self = static_cast<EngraveHostCalls*>(userData);
        ++self->callCount;
    }
};
}  // namespace


// ===========================================================================
// cFWEngraveDialog
// ===========================================================================

// ---------------------------------------------------------------------------
// Construction + constants
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, DefaultProcessTimeIsZero) {
    auto d = MakeEngrave();
    EXPECT_EQ(d->GetProcessTime(), 0u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 1.0f);
}

TEST(CFWEngraveDialog, ChildrenNullBeforeLinking) {
    auto d = MakeEngrave();
    EXPECT_EQ(d->GetEngraveGuage(), nullptr);
    EXPECT_EQ(d->GetRemaintimeStatic(), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, LinkingMaterializesBothChildren) {
    auto d = MakeEngrave();
    d->Linking();
    EXPECT_NE(d->GetEngraveGuage(), nullptr);
    EXPECT_NE(d->GetRemaintimeStatic(), nullptr);
}

TEST(CFWEngraveDialog, LinkingSetsChildIds) {
    auto d = MakeEngrave();
    d->Linking();
    EXPECT_EQ(d->GetEngraveGuage()->id(),      780);
    EXPECT_EQ(d->GetRemaintimeStatic()->id(), 781);
}

TEST(CFWEngraveDialog, LinkingIdempotent) {
    auto d = MakeEngrave();
    d->Linking();
    d->Linking();
    EXPECT_NE(d->GetEngraveGuage(), nullptr);
    EXPECT_NE(d->GetRemaintimeStatic(), nullptr);
}

// ---------------------------------------------------------------------------
// SetActiveWithTime
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, SetActiveWithTimeTrueStoresProcessTime) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 30);  // 30 seconds
    EXPECT_EQ(d->GetProcessTime(), 30u * 1000u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 30.0f);
    EXPECT_TRUE(d->isActive());
}

TEST(CFWEngraveDialog, SetActiveWithTimeFalseResetsState) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 10);
    d->SetActiveWithTime(false, 0);
    EXPECT_EQ(d->GetProcessTime(), 0u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 1.0f);
    EXPECT_FALSE(d->isActive());
}

TEST(CFWEngraveDialog, SetActiveWithTimeTrueThenTrue) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 5);
    d->SetActiveWithTime(true, 60);
    EXPECT_EQ(d->GetProcessTime(), 60u * 1000u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 60.0f);
}

// ---------------------------------------------------------------------------
// ActionEvent
// ---------------------------------------------------------------------------

TEST(CFWEngraveDialog, ActionEventOnDisabledDialogReturnsZero) {
    auto d = MakeEngrave();
    d->Linking();
    // Dialog starts disabled.
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    EXPECT_EQ(we, 0u);  // WE_NULL
}

TEST(CFWEngraveDialog, ActionEventOnEnabledDialogDelegatesToBase) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActive(true);
    // Stubbed in modern port: no time refresh (gCurTime unported), but
    // base cDialog::ActionEvent still gets called for hit-test.
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    // Hit test on (0,0) which is inside the dialog at (0,0,200,100) →
    // topmost child hit → returns some non-zero we bits (likely
    // WE_MOUSEOVER from base). We only assert "did not crash".
    (void)we;
    SUCCEED();
}

// ---------------------------------------------------------------------------
// OnActionEvent
// ---------------------------------------------------------------------------

// New tests for OnActionEvent host dispatch

TEST(CFWEngraveDialog, SendEngraveCancelCallbackInitiallyNull) {
    auto d = MakeEngrave();
    EXPECT_EQ(d->GetSendEngraveCancelForTest(), nullptr);
    EXPECT_EQ(d->GetCallbackUserDataForTest(), nullptr);
    EXPECT_EQ(d->GetEngraveCancelIdForTest(), 784);
    EXPECT_EQ(d->GetWeBtnClickForTest(), 64u);
}

TEST(CFWEngraveDialog, SetFwEngraveCancelCallbackStoresPointer) {
    auto d = MakeEngrave();
    EngraveHostCalls hc;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc);
    EXPECT_EQ(d->GetSendEngraveCancelForTest(), &EngraveHostCalls::SendEngraveCancel);
    EXPECT_EQ(d->GetCallbackUserDataForTest(), &hc);
}

TEST(CFWEngraveDialog, SetFwEngraveCancelCallbackReplaces) {
    auto d = MakeEngrave();
    EngraveHostCalls hc1;
    EngraveHostCalls hc2;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc1);
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc2);
    EXPECT_EQ(d->GetCallbackUserDataForTest(), &hc2);
}

TEST(CFWEngraveDialog, SetFwEngraveCancelCallbackWithNullFnClears) {
    auto d = MakeEngrave();
    EngraveHostCalls hc;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc);
    d->SetFwEngraveCancelCallback(nullptr, nullptr);
    EXPECT_EQ(d->GetSendEngraveCancelForTest(), nullptr);
    EXPECT_EQ(d->GetCallbackUserDataForTest(), nullptr);
}


// ============================================================================
// cFWEngraveDialog -- OnActionEvent host dispatch
// ============================================================================

TEST(CFWEngraveDialog, OnActionEventWithoutCallbackIsNoOp) {
    auto d = MakeEngrave();
    d->Linking();
    // No callback installed; must not crash.
    d->OnActionEvent(d->GetEngraveCancelIdForTest(), nullptr, d->GetWeBtnClickForTest());
    SUCCEED();
}

TEST(CFWEngraveDialog, OnActionEventWithoutBtnClickFlagIsNoOp) {
    auto d = MakeEngrave();
    d->Linking();
    EngraveHostCalls hc;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc);
    // we=0 (no WE_BTNCLICK); even with lId == kEngraveCancelId, no dispatch.
    d->OnActionEvent(d->GetEngraveCancelIdForTest(), nullptr, 0);
    EXPECT_EQ(hc.callCount, 0);
}

TEST(CFWEngraveDialog, OnActionEventWithWrongIdIsNoOp) {
    auto d = MakeEngrave();
    d->Linking();
    EngraveHostCalls hc;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc);
    // we=WE_BTNCLICK but lId != kEngraveCancelId; no dispatch.
    d->OnActionEvent(999, nullptr, d->GetWeBtnClickForTest());
    EXPECT_EQ(hc.callCount, 0);
}

TEST(CFWEngraveDialog, OnActionEventFwEngraveCancelDispatchesCallback) {
    auto d = MakeEngrave();
    d->Linking();
    EngraveHostCalls hc;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc);
    // we=WE_BTNCLICK + lId=kEngraveCancelId -> dispatch.
    d->OnActionEvent(d->GetEngraveCancelIdForTest(), nullptr, d->GetWeBtnClickForTest());
    EXPECT_EQ(hc.callCount, 1);
    // Replaces check: set null callback, dispatch again -- no inc.
    d->SetFwEngraveCancelCallback(nullptr, nullptr);
    d->OnActionEvent(d->GetEngraveCancelIdForTest(), nullptr, d->GetWeBtnClickForTest());
    EXPECT_EQ(hc.callCount, 1);
}

TEST(CFWEngraveDialog, OnActionEventFwEngraveCancelWithExtraFlagsStillDispatches) {
    auto d = MakeEngrave();
    d->Linking();
    EngraveHostCalls hc;
    d->SetFwEngraveCancelCallback(&EngraveHostCalls::SendEngraveCancel, &hc);
    // Legacy 1:1: legacy guard is (we & WE_BTNCLICK) -- any extra flags are
    // ignored. Modern port keeps the same semantics.
    const std::uint32_t we = d->GetWeBtnClickForTest() | 0x100u | 0x200u;
    d->OnActionEvent(d->GetEngraveCancelIdForTest(), nullptr, we);
    EXPECT_EQ(hc.callCount, 1);
}

// ============================================================================
// cFWTimeDialog

// ===========================================================================

// ---------------------------------------------------------------------------
// Construction + constants
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, DefaultWarTimeIsZero) {
    auto d = MakeTime();
    EXPECT_EQ(d->GetWarTime(), 0u);
}

TEST(CFWTimeDialog, ChildrenNullBeforeLinking) {
    auto d = MakeTime();
    EXPECT_EQ(d->GetTimeStatic(), nullptr);
    EXPECT_EQ(d->GetCharacterName(), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, LinkingMaterializesBothStatics) {
    auto d = MakeTime();
    d->Linking();
    EXPECT_NE(d->GetTimeStatic(), nullptr);
    EXPECT_NE(d->GetCharacterName(), nullptr);
}

TEST(CFWTimeDialog, LinkingSetsChildIds) {
    auto d = MakeTime();
    d->Linking();
    EXPECT_EQ(d->GetTimeStatic()->id(),    782);
    EXPECT_EQ(d->GetCharacterName()->id(), 783);
}

// ---------------------------------------------------------------------------
// SetActiveWithTimeName
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, SetActiveWithTimeNameTrueStoresWarTime) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 60, "Alice");
    EXPECT_EQ(d->GetWarTime(), 60u * 1000u);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Alice");
}

TEST(CFWTimeDialog, SetActiveWithTimeNameFalseClearsName) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 30, "Bob");
    d->SetActiveWithTimeName(false, 0, nullptr);
    EXPECT_EQ(d->GetWarTime(), 0u);
    EXPECT_FALSE(d->isActive());
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "");
}

TEST(CFWTimeDialog, SetActiveWithTimeNameNullNameIsNoOpForText) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 10, nullptr);
    EXPECT_TRUE(d->isActive());
    // Text not modified since pName was null.
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "");
}

// ---------------------------------------------------------------------------
// SetCharacterName
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, SetCharacterNameUpdatesText) {
    auto d = MakeTime();
    d->Linking();
    d->SetCharacterName("Carol");
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Carol");
}

TEST(CFWTimeDialog, SetCharacterNameNullIsDefensiveNoOp) {
    auto d = MakeTime();
    d->Linking();
    d->SetCharacterName("Eve");
    d->SetCharacterName(nullptr);  // defensive
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Eve");
}

TEST(CFWTimeDialog, SetCharacterNameBeforeLinkingIsNoOp) {
    auto d = MakeTime();
    d->SetCharacterName("X");  // null m_pCharacterName, no crash
    EXPECT_EQ(d->GetCharacterName(), nullptr);
}

// ---------------------------------------------------------------------------
// ActionEvent
// ---------------------------------------------------------------------------

TEST(CFWTimeDialog, ActionEventOnDisabledDialogReturnsZero) {
    auto d = MakeTime();
    d->Linking();
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    EXPECT_EQ(we, 0u);
}

TEST(CFWTimeDialog, ActionEventOnEnabledDialogDoesNotCrash) {
    auto d = MakeTime();
    d->Linking();
    d->SetActive(true);
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    (void)we;
    SUCCEED();
}

// ===========================================================================
// cFWEngraveDialog -- clock provider + chat msg provider + ActionEvent body
// ===========================================================================

TEST(CFWEngraveDialog, SetActiveWithTimeStampsProcessTimeFromClock) {
    ClockCapture clock;
    clock.value = 5000u;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTime(true, 30);  // 30 seconds
    EXPECT_EQ(d->GetProcessTime(), 5000u + 30u * 1000u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 30.0f);
    EXPECT_TRUE(d->isActive());
}

TEST(CFWEngraveDialog, SetActiveWithTimeWithoutProviderUsesZeroClock) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActiveWithTime(true, 30);
    // No clock provider: m_dwProcessTime = 0 + dwTime*1000.
    EXPECT_EQ(d->GetProcessTime(), 30u * 1000u);
}

TEST(CFWEngraveDialog, SetActiveWithTimeFalseResetsLastTickAndProcessTime) {
    ClockCapture clock;
    clock.value = 5000u;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTime(true, 30);
    d->SetActiveWithTime(false, 0);
    EXPECT_EQ(d->GetProcessTime(), 0u);
    EXPECT_FLOAT_EQ(d->GetBasicTime(), 1.0f);
    EXPECT_FALSE(d->isActive());
}

TEST(CFWEngraveDialog, ActionEventRefreshesStaticTextWithDefaultFormat) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTime(true, 30);
    // m_dwProcessTime = 1000 + 30000 = 31000; nLimitTime = 30
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetRemaintimeStatic()->GetStaticText(), "Engrave time: 30");
}

TEST(CFWEngraveDialog, ActionEventRefreshesStaticTextWithInjectedChatMsg) {
    ClockCapture clock;
    clock.value = 1000u;
    ChatCapture chat;
    chat.template_str = "[%d sec]";
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetChatMessageFn(&ChatCapture::Get, &chat);
    d->SetActiveWithTime(true, 30);
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetRemaintimeStatic()->GetStaticText(), "[30 sec]");
    EXPECT_EQ(chat.calls, 1);
    EXPECT_EQ(chat.last_msg_id, 1043);
}

TEST(CFWEngraveDialog, ActionEventThrottlesByLastTick) {
    ClockCapture clock;
    clock.value = 1000u;
    ChatCapture chat;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetChatMessageFn(&ChatCapture::Get, &chat);
    d->SetActiveWithTime(true, 30);
    // First tick: m_dwProcessTime = 1000 + 30000 = 31000; nLimitTime = 30
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(chat.calls, 1);
    // Second tick same nLimitTime: throttled.
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(chat.calls, 1);
}

TEST(CFWEngraveDialog, ActionEventUpdatesGuageValue) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTime(true, 30);
    d->ActionEvent(0, 0, 0);
    // m_fBasicTime=30, nLimitTime=30, value=30/30=1.0
    EXPECT_FLOAT_EQ(d->GetEngraveGuage()->GetValue(), 1.0f);
}

TEST(CFWEngraveDialog, ActionEventClampsToZeroOnExpiry) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    // m_dwProcessTime = 1000 + 30000 = 31000; nLimitTime = 30.
    d->SetActiveWithTime(true, 30);
    d->ActionEvent(0, 0, 0);
    // Advance clock past process time → nLimitTime = -19 → clamped to 0.
    clock.value = 50000u;
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetRemaintimeStatic()->GetStaticText(), "Engrave time: 0");
}

TEST(CFWEngraveDialog, ActionEventAdvancesOnSecondTick) {
    ClockCapture clock;
    clock.value = 1000u;
    ChatCapture chat;
    chat.template_str = "Remaining %d sec";
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetChatMessageFn(&ChatCapture::Get, &chat);
    d->SetActiveWithTime(true, 30);
    // Tick 1: nLimitTime = (31000-1000)/1000 = 30.
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(chat.calls, 1);
    // Advance clock by 1 second -> nLimitTime = 29.
    clock.value = 2000u;
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(chat.calls, 2);
    EXPECT_EQ(d->GetRemaintimeStatic()->GetStaticText(), "Remaining 29 sec");
}

TEST(CFWEngraveDialog, ActionEventWithoutClockProviderIsNoOp) {
    auto d = MakeEngrave();
    d->Linking();
    d->SetActive(true);
    d->SetActiveWithTime(true, 30);
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    (void)we;
    // Static text unchanged because clock provider is null.
    EXPECT_EQ(d->GetRemaintimeStatic()->GetStaticText(), "");
}

TEST(CFWEngraveDialog, ActionEventHandlesZeroBasicTimeGracefully) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeEngrave();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTime(true, 0);  // 0 second -> m_fBasicTime=0
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    (void)we;
    // Must not crash on division-by-zero. Static text refreshed.
    EXPECT_EQ(d->GetRemaintimeStatic()->GetStaticText(), "Engrave time: 0");
}

// ===========================================================================
// cFWTimeDialog -- clock provider + ActionEvent body
// ===========================================================================

TEST(CFWTimeDialog, SetActiveWithTimeNameStampsWarTimeFromClock) {
    ClockCapture clock;
    clock.value = 5000u;
    auto d = MakeTime();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTimeName(true, 60, "Alice");
    EXPECT_EQ(d->GetWarTime(), 5000u + 60u * 1000u);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->GetCharacterName()->GetStaticText(), "Alice");
}

TEST(CFWTimeDialog, SetActiveWithTimeNameWithoutProviderUsesZeroClock) {
    auto d = MakeTime();
    d->Linking();
    d->SetActiveWithTimeName(true, 60, "Alice");
    // No clock provider: m_dwWarTime = 0 + 60000.
    EXPECT_EQ(d->GetWarTime(), 60u * 1000u);
}

TEST(CFWTimeDialog, ActionEventRefreshesStaticTextAsMmSs) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeTime();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTimeName(true, 130, "Bob");  // 2:10
    // m_dwWarTime = 1000 + 130000 = 131000; nLimitTime = 130
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "02:10");
}

TEST(CFWTimeDialog, ActionEventThrottlesByLastTick) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeTime();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTimeName(true, 60, "C");
    // warTime = 1000 + 60000 = 61000; nLimitTime = (61000-1000)/1000 = 60; fmt = 01:00.
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "01:00");
    // Same nLimitTime throttled.
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "01:00");
}

TEST(CFWTimeDialog, ActionEventClampsToZeroOnExpiry) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeTime();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTimeName(true, 30, "D");
    // warTime = 1000 + 30000 = 31000; nLimitTime = 30; fmt = 00:30.
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "00:30");
    // Advance clock past process time → nLimitTime = -19 → clamped to 0.
    clock.value = 50000u;
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "00:00");
}

TEST(CFWTimeDialog, ActionEventAdvancesOnSecondTick) {
    ClockCapture clock;
    clock.value = 1000u;
    auto d = MakeTime();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTimeName(true, 65, "E");  // 1:05
    // warTime = 1000 + 65000 = 66000; nLimitTime = 65 -> 01:05
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "01:05");
    // Advance clock by 5 seconds -> nLimitTime = 60 -> 01:00
    clock.value = 6000u;
    d->ActionEvent(0, 0, 0);
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "01:00");
}

TEST(CFWTimeDialog, ActionEventWithoutClockProviderIsNoOp) {
    auto d = MakeTime();
    d->Linking();
    d->SetActive(true);
    d->SetActiveWithTimeName(true, 60, "F");
    std::uint32_t we = d->ActionEvent(0, 0, 0);
    (void)we;
    // Static text unchanged because clock provider is null.
    EXPECT_EQ(d->GetTimeStatic()->GetStaticText(), "");
}

TEST(CFWTimeDialog, SetActiveWithTimeNameFalseResetsLastTick) {
    ClockCapture clock;
    clock.value = 5000u;
    auto d = MakeTime();
    d->Linking();
    d->SetCurrentTimeProvider(&ClockCapture::Get, &clock);
    d->SetActiveWithTimeName(true, 30, "G");
    d->ActionEvent(0, 0, 0);
    d->SetActiveWithTimeName(false, 0, nullptr);
    EXPECT_EQ(d->GetWarTime(), 0u);
    EXPECT_FALSE(d->isActive());
}
