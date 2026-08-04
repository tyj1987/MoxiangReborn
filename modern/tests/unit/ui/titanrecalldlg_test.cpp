// titanrecalldlg_test.cpp — 1:1 port tests for
// 墨香 CTitanRecallDlg.

#include "titanrecalldlg.hpp"
#include "progressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cObjectGuagen;
using mxh::ui::cProgressBarDlg;
using mxh::ui::cStatic;
using mxh::ui::cTitanRecallDlg;

namespace {

struct LinkedDialog {
    cTitanRecallDlg dlg;
    std::unique_ptr<cObjectGuagen> progressGuagen;
    std::unique_ptr<cStatic> remaintime;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 100, nullptr, 0);
        progressGuagen = std::make_unique<cObjectGuagen>();
        progressGuagen->Init(0, 0, 200, 20, nullptr,
                             cTitanRecallDlg::kIdProgressBarGage);
        auto* gPtr = progressGuagen.get();
        dlg.Add(std::move(progressGuagen));

        remaintime = std::make_unique<cStatic>();
        remaintime->Init(0, 20, 100, 20, nullptr,
                         cTitanRecallDlg::kIdRemaintimeTime);
        auto* sPtr = remaintime.get();
        dlg.Add(std::move(remaintime));

        dlg.Linking();

        gPtr_ = gPtr;
        sPtr_ = sPtr;
    }

    cObjectGuagen* gPtr_ = nullptr;
    cStatic* sPtr_ = nullptr;
};

}  // namespace

TEST(CTitanRecallDlgTest, CtorDoesNotCrash) {
    cTitanRecallDlg dlg;
    SUCCEED();
}

TEST(CTitanRecallDlgTest, DtorDoesNotCrash) {
    cTitanRecallDlg dlg;
    SUCCEED();
}

TEST(CTitanRecallDlgTest, InheritsFromCProgressBarDlg) {
    static_assert(std::is_base_of_v<cProgressBarDlg, cTitanRecallDlg>,
                  "cTitanRecallDlg must inherit from cProgressBarDlg");
    SUCCEED();
}

TEST(CTitanRecallDlgTest, DefaultSuccessRecallIsFalse) {
    cTitanRecallDlg dlg;
    EXPECT_FALSE(dlg.GetSuccessRecall());
}

TEST(CTitanRecallDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cTitanRecallDlg::kIdProgressBarGage, 690);
    EXPECT_EQ(cTitanRecallDlg::kIdRemaintimeTime, 691);
    EXPECT_EQ(cTitanRecallDlg::kIdCancelBtn, 692);
}

TEST(CTitanRecallDlgTest, BaseSuccessTimeIs7000) {
    EXPECT_EQ(cTitanRecallDlg::kBaseSuccessTime, 7000u);
}

TEST(CTitanRecallDlgTest, LinkingResolvesProgressGuagen) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetProgressGuagen(), ld.gPtr_);
}

TEST(CTitanRecallDlgTest, LinkingResolvesRemaintimeStatic) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetRemaintimeStatic(), ld.sPtr_);
}

TEST(CTitanRecallDlgTest, LinkingSetsSuccessTime) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetSuccessTime(), cTitanRecallDlg::kBaseSuccessTime);
}

TEST(CTitanRecallDlgTest, LinkingBeforeInitDoesNotCrash) {
    cTitanRecallDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CTitanRecallDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
    // SuccessTime is still set
    EXPECT_EQ(dlg.GetSuccessTime(), cTitanRecallDlg::kBaseSuccessTime);
}

TEST(CTitanRecallDlgTest, OnActionEventReturnsTrue) {
    LinkedDialog ld;
    EXPECT_TRUE(ld.dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0));
    EXPECT_TRUE(ld.dlg.OnActionEvent(999, nullptr, 0));
}

TEST(CTitanRecallDlgTest, OnActionEventBeforeInitIsSafe) {
    cTitanRecallDlg dlg;
    EXPECT_TRUE(dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0));
}

TEST(CTitanRecallDlgTest, RenderIsNoOp) {
    LinkedDialog ld;
    ld.dlg.Render();
    SUCCEED();
}

TEST(CTitanRecallDlgTest, RenderBeforeInitDoesNotCrash) {
    cTitanRecallDlg dlg;
    dlg.Render();
    SUCCEED();
}

TEST(CTitanRecallDlgTest, SetSuccessRecallToggles) {
    cTitanRecallDlg dlg;
    EXPECT_FALSE(dlg.GetSuccessRecall());
    dlg.SetSuccessRecall(true);
    EXPECT_TRUE(dlg.GetSuccessRecall());
    dlg.SetSuccessRecall(false);
    EXPECT_FALSE(dlg.GetSuccessRecall());
}



// ---------- WeCloseWindow / ObjectStateSociety constants ----------

TEST(CTitanRecallDlgTest, WeCloseWindowIsLegacy1) {
    EXPECT_EQ(cTitanRecallDlg::kWeCloseWindow, 1u);
}

TEST(CTitanRecallDlgTest, ObjectStateSocietyMatchesLegacyEnum) {
    EXPECT_EQ(cTitanRecallDlg::kObjectStateSociety, 24);
}

TEST(CTitanRecallDlgTest, TitanCategoryMatchesLegacy72) {
    EXPECT_EQ(cTitanRecallDlg::kTitanCategory, 72u);
}

TEST(CTitanRecallDlgTest, TitanRecallSynProtocolMatchesLegacy3) {
    EXPECT_EQ(cTitanRecallDlg::kTitanRecallSynProtocol, 3u);
}

TEST(CTitanRecallDlgTest, TitanRecallCancelSynProtocolMatchesLegacy6) {
    EXPECT_EQ(cTitanRecallDlg::kTitanRecallCancelSynProtocol, 6u);
}

// ---------- SetRecallSendCallbacks / Render success path ----------

namespace {

struct RecallSendCapture {
    int synCalls = 0;
    int cancelCalls = 0;
    std::uint32_t lastSynObjectId = 0;
    std::uint32_t lastCancelObjectId = 0;
};

std::uint32_t CaptureHeroObjectId(void* userData) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(userData) & 0xFFFFFFFFu);
}

bool CaptureSendRecallSyn(std::uint32_t objectId, void* userData) {
    auto* c = static_cast<RecallSendCapture*>(userData);
    ++c->synCalls;
    c->lastSynObjectId = objectId;
    return true;
}

bool CaptureSendRecallCancelSyn(std::uint32_t objectId, void* userData) {
    auto* c = static_cast<RecallSendCapture*>(userData);
    ++c->cancelCalls;
    c->lastCancelObjectId = objectId;
    return true;
}

}  // namespace

TEST(CTitanRecallDlgTest, RenderInvokesRecallSynOnSuccess) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    RecallSendCapture cap;
    dlg.SetRecallSendCallbacks(&CaptureHeroObjectId,
                              &CaptureSendRecallSyn,
                              &CaptureSendRecallCancelSyn,
                              &cap);
    dlg.SetSuccessProgress(true);
    EXPECT_TRUE(dlg.GetSuccessProgress());
    dlg.Render();
    EXPECT_EQ(cap.synCalls, 1);
    EXPECT_EQ(cap.cancelCalls, 0);
    EXPECT_EQ(cap.lastSynObjectId,
              static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(&cap) & 0xFFFFFFFFu));
    EXPECT_FALSE(dlg.GetSuccessProgress());
}

TEST(CTitanRecallDlgTest, RenderNoCallbackOnSuccessIsSafeAndStillResets) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    dlg.SetSuccessProgress(true);
    dlg.Render();
    EXPECT_FALSE(dlg.GetSuccessProgress());
}

TEST(CTitanRecallDlgTest, RenderWithoutSuccessDoesNotInvokeSend) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    RecallSendCapture cap;
    dlg.SetRecallSendCallbacks(&CaptureHeroObjectId,
                              &CaptureSendRecallSyn,
                              &CaptureSendRecallCancelSyn,
                              &cap);
    dlg.Render();
    EXPECT_EQ(cap.synCalls, 0);
    EXPECT_EQ(cap.cancelCalls, 0);
}

TEST(CTitanRecallDlgTest, SetRecallSendCallbacksAllowNullUserData) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    auto getId = [](void*) -> std::uint32_t { return 0xDEADBEEFu; };
    auto sendSyn = [](std::uint32_t, void*) { return true; };
    auto sendCancel = [](std::uint32_t, void*) { return true; };
    dlg.SetRecallSendCallbacks(getId, sendSyn, sendCancel);
    dlg.SetSuccessProgress(true);
    dlg.Render();
    SUCCEED();
}

// ---------- SetCloseWindowCallbacks / OnActionEvent WE_CLOSEWINDOW branch ----------

namespace {

struct CloseWindowCapture {
    int endCalls = 0;
    std::uint32_t lastObjectId = 0;
    std::int32_t lastState = -1;
    std::uint32_t heroObjectId = 0x12345678u;
    std::int32_t heroState = cTitanRecallDlg::kObjectStateSociety;
};

std::uint32_t CloseWindowGetHeroObjectId(void* userData) {
    return static_cast<CloseWindowCapture*>(userData)->heroObjectId;
}

std::int32_t CloseWindowGetHeroState(void* userData) {
    return static_cast<CloseWindowCapture*>(userData)->heroState;
}

void CloseWindowEndObjectState(std::uint32_t objectId,
                               std::int32_t stateIdx,
                               void* userData) {
    auto* c = static_cast<CloseWindowCapture*>(userData);
    ++c->endCalls;
    c->lastObjectId = objectId;
    c->lastState = stateIdx;
}

}  // namespace

TEST(CTitanRecallDlgTest, OnActionEventCloseWindowEndsSocietyState) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    CloseWindowCapture cap;
    dlg.SetCloseWindowCallbacks(&CloseWindowGetHeroObjectId,
                                &CloseWindowGetHeroState,
                                &CloseWindowEndObjectState,
                                &cap);
    dlg.OnActionEvent(999, nullptr,
                      cTitanRecallDlg::kWeCloseWindow);
    EXPECT_EQ(cap.endCalls, 1);
    EXPECT_EQ(cap.lastObjectId, cap.heroObjectId);
    EXPECT_EQ(cap.lastState, cTitanRecallDlg::kObjectStateSociety);
}

TEST(CTitanRecallDlgTest, OnActionEventCloseWindowNonSocietyNoEnd) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    CloseWindowCapture cap;
    cap.heroState = 0;
    dlg.SetCloseWindowCallbacks(&CloseWindowGetHeroObjectId,
                                &CloseWindowGetHeroState,
                                &CloseWindowEndObjectState,
                                &cap);
    dlg.OnActionEvent(999, nullptr,
                      cTitanRecallDlg::kWeCloseWindow);
    EXPECT_EQ(cap.endCalls, 0);
}

TEST(CTitanRecallDlgTest, OnActionEventCloseWindowWithoutCallbackIsSafe) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    dlg.OnActionEvent(999, nullptr,
                      cTitanRecallDlg::kWeCloseWindow);
    SUCCEED();
}

TEST(CTitanRecallDlgTest, OnActionEventCloseWindowNullUserDataIsSafe) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    auto getId = [](void*) -> std::uint32_t { return 0xCAFEBABEu; };
    auto getState = [](void*) -> std::int32_t { return 24; };
    auto endState = [](std::uint32_t, std::int32_t, void*) {};
    dlg.SetCloseWindowCallbacks(getId, getState, endState);
    dlg.OnActionEvent(999, nullptr,
                      cTitanRecallDlg::kWeCloseWindow);
    SUCCEED();
}

// ---------- SetRecallSendCallbacks / OnActionEvent kIdCancelBtn branch ----------

TEST(CTitanRecallDlgTest, OnActionEventCancelSendsTitanRecallCancelSyn) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    RecallSendCapture cap;
    dlg.SetRecallSendCallbacks(&CaptureHeroObjectId,
                              &CaptureSendRecallSyn,
                              &CaptureSendRecallCancelSyn,
                              &cap);
    dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0);
    EXPECT_EQ(cap.cancelCalls, 1);
    EXPECT_EQ(cap.synCalls, 0);
    EXPECT_EQ(cap.lastCancelObjectId,
              static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(&cap) & 0xFFFFFFFFu));
}

TEST(CTitanRecallDlgTest, OnActionEventCancelWithoutCallbackIsSafe) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0);
    SUCCEED();
}

TEST(CTitanRecallDlgTest, OnActionEventUnknownIdAndUnknownWeIsNoOp) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    RecallSendCapture cap;
    CloseWindowCapture closeCap;
    dlg.SetRecallSendCallbacks(&CaptureHeroObjectId,
                              &CaptureSendRecallSyn,
                              &CaptureSendRecallCancelSyn,
                              &cap);
    dlg.SetCloseWindowCallbacks(&CloseWindowGetHeroObjectId,
                                &CloseWindowGetHeroState,
                                &CloseWindowEndObjectState,
                                &closeCap);
    dlg.OnActionEvent(999, nullptr, 0);
    EXPECT_EQ(cap.synCalls, 0);
    EXPECT_EQ(cap.cancelCalls, 0);
    EXPECT_EQ(closeCap.endCalls, 0);
}

TEST(CTitanRecallDlgTest, OnActionEventCancelReplacesCallback) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    RecallSendCapture first;
    RecallSendCapture second;
    dlg.SetRecallSendCallbacks(&CaptureHeroObjectId,
                              &CaptureSendRecallSyn,
                              &CaptureSendRecallCancelSyn,
                              &first);
    dlg.SetRecallSendCallbacks(&CaptureHeroObjectId,
                              &CaptureSendRecallSyn,
                              &CaptureSendRecallCancelSyn,
                              &second);
    dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0);
    EXPECT_EQ(first.cancelCalls, 0);
    EXPECT_EQ(second.cancelCalls, 1);
}

