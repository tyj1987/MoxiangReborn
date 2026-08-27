// mxh/tests/unit/client/game_state_stubs_test.cpp
// Unit tests for the 9 eGAMESTATE concrete stubs (Phase A.1.7).
//
// Each stub's Init / Release / Process should:
//   * Set isInitialized() correctly.
//   * Be safe to call multiple times (idempotent-ish — the second
//     Init call after Release will reset isInitialized to true).
//   * Not crash even if Process() runs without an explicit Init.

#include "GameStateStubs.hpp"
#include "CGameState.hpp"
#include "GameLoadingCoordinator.hpp"
#include "CEngine.hpp"

#include <gtest/gtest.h>

using namespace mxh::client;

namespace {

template <typename T>
void CheckStubLifecycle() {
    T s;
    EXPECT_FALSE(s.isInitialized());
    s.Init(nullptr);
    EXPECT_TRUE(s.isInitialized());
    s.Process();           // no-op must not crash
    s.Release();
    EXPECT_FALSE(s.isInitialized());
}

} // namespace

TEST(CIntroReplay, Lifecycle) { CheckStubLifecycle<CIntroReplay>(); }
TEST(CMainTitle, Lifecycle)   { CheckStubLifecycle<CMainTitle>(); }
TEST(CLoginState, Lifecycle)  { CheckStubLifecycle<CLoginState>(); }
TEST(CCharSelectState, Lifecycle)  { CheckStubLifecycle<CCharSelectState>(); }
TEST(CGameLoading, Lifecycle) { CheckStubLifecycle<CGameLoading>(); }
TEST(CInGameState, Lifecycle) { CheckStubLifecycle<CInGameState>(); }
TEST(CMapChange, Lifecycle)   { CheckStubLifecycle<CMapChange>(); }
TEST(CMurimNet, Lifecycle)    { CheckStubLifecycle<CMurimNet>(); }

TEST(CGameLoading, TracksCoordinatorContext) {
    LoadStateContext context;
    context.completed_steps = 3;
    context.total_steps = 10;
    CGameLoading state;
    state.Init(&context);
    EXPECT_FLOAT_EQ(state.progress(), 0.3f);
    context.completed_steps = 8;
    state.Process();
    EXPECT_FLOAT_EQ(state.progress(), 0.8f);
    context.failed = true;
    context.error = "missing map asset";
    state.Process();
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "missing map asset");
    state.Release();
}

TEST(CGameLoading, ReportsCompletionOnlyAfterAllSteps) {
    LoadStateContext context{};
    context.total_steps = 4;
    context.completed_steps = 3;
    CGameLoading state;
    state.Init(&context);
    EXPECT_FALSE(state.completed());
    context.completed_steps = 4;
    state.Process();
    EXPECT_TRUE(state.completed());
}

TEST(CGameLoading, RejectsInvalidContext) {
    LoadStateContext context;
    context.total_steps = 0;
    CGameLoading state;
    state.Init(&context);
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "loading context has zero steps");
}

TEST(CGameLoading, StartFailsClosedWithoutEngineOrResourceRoot) {
    LoadStateContext context{};
    context.total_steps = 1;
    CGameLoading state;
    state.Init(&context);
    state.Start(nullptr);
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "loading requires a client engine");
}

TEST(CMapChange, TracksProgressCancellationAndFailure) {
    LoadStateContext context;
    context.completed_steps = 4;
    context.total_steps = 10;
    CMapChange state;
    state.Init(&context);
    EXPECT_FLOAT_EQ(state.progress(), 0.4f);

    context.completed_steps = 7;
    context.cancelled = true;
    state.Process();
    EXPECT_FLOAT_EQ(state.progress(), 0.7f);
    EXPECT_TRUE(state.cancelled());

    context.failed = true;
    context.error = "target map unavailable";
    state.Process();
    EXPECT_FALSE(state.failed());
    EXPECT_TRUE(state.cancelled());
    EXPECT_TRUE(state.error().empty());
}

TEST(CMapChange, FirstFailureWinsOverLateProgress) {
    LoadStateContext context{};
    context.total_steps = 10;
    CMapChange state;
    state.Init(&context);
    context.failed = true;
    context.error = "target map unavailable";
    state.Process();
    ASSERT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "target map unavailable");
    context.failed = false;
    context.completed_steps = 10;
    state.Process();
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "target map unavailable");
    EXPECT_LT(state.progress(), 1.0f);
}

TEST(CMapChange, FailureAndCancellationNeverReportCompletion) {
    LoadStateContext context{};
    context.total_steps = 1;
    context.completed_steps = 1;
    context.cancelled = true;
    CMapChange state;
    state.Init(&context);
    EXPECT_FALSE(state.completed());
}

TEST(CMapChange, RejectsInvalidContext) {
    LoadStateContext context;
    context.total_steps = 0;
    CMapChange state;
    state.Init(&context);
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "map change context has zero steps");
}

TEST(CMapChange, StartFailsClosedWithoutEngineOrResourceRoot) {
    LoadStateContext context{};
    context.total_steps = 1;
    CMapChange state;
    state.Init(&context);
    state.Start(nullptr);
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "map change requires a client engine");
}

TEST(GameLoadingCoordinator, ConsumesOnlyValidEntryTransfer) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{42, 10});
    std::string error;
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine, &error)) << error;
    EXPECT_EQ(coordinator.request().character_id, 42u);
    EXPECT_EQ(coordinator.request().map_num, 10u);
    EXPECT_EQ(coordinator.context().completed_steps, 0u);
    coordinator.mark_completed(100);
    EXPECT_EQ(coordinator.context().completed_steps, 10u);
}

TEST(GameLoadingCoordinator, AcceptsShippedMapZero) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{77u, 0u});
    std::string error;
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine, &error)) << error;
    EXPECT_EQ(coordinator.request().character_id, 77u);
    EXPECT_EQ(coordinator.request().map_num, 0u);
    EXPECT_FALSE(coordinator.context().failed);
}

TEST(GameLoadingCoordinator, ClearsPreviousRequestWhenTransferIsInvalid) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{55u, 10u});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    // A transfer of the wrong variant is an invalid next request; the old
    // request must not remain observable after the failed consumption attempt.
    engine.SetPendingTransfer(LoginResult{});
    std::string error;
    EXPECT_FALSE(coordinator.consume_pending_transfer(engine, &error));
    EXPECT_FALSE(coordinator.has_request());
    EXPECT_NE(error.find("invalid"), std::string::npos);
}

TEST(GameLoadingCoordinator, ClearsPreviousRequestWhenNoTransferIsPending) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{56u, 10u});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    std::string error;
    EXPECT_FALSE(coordinator.consume_pending_transfer(engine, &error));
    EXPECT_FALSE(coordinator.has_request());
    EXPECT_NE(error.find("waiting"), std::string::npos);
}

TEST(GameLoadingCoordinator, ProgressDoesNotRegressFromLateStageCallback) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{7, 10});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    coordinator.mark_completed(6);
    coordinator.mark_completed(3);
    EXPECT_EQ(coordinator.context().completed_steps, 6u);
    coordinator.mark_completed(10);
    coordinator.mark_completed(8);
    EXPECT_EQ(coordinator.context().completed_steps, 10u);
}

TEST(GameLoadingCoordinator, CancellationIsTerminalAndIgnoresLateCompletion) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{7, 10});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    coordinator.mark_completed(4);
    coordinator.cancel();
    EXPECT_TRUE(coordinator.terminal());
    EXPECT_TRUE(coordinator.context().cancelled);
    coordinator.mark_completed(10);
    EXPECT_EQ(coordinator.context().completed_steps, 4u);
}

TEST(GameLoadingCoordinator, FirstFailureWinsAgainstLateWorkerError) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    engine.SetPendingTransfer(GameEntryRequest{7, 10});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    coordinator.mark_failed("texture decode failed");
    coordinator.mark_failed("late worker error");
    EXPECT_TRUE(coordinator.context().failed);
    ASSERT_NE(coordinator.context().error, nullptr);
    EXPECT_STREQ(coordinator.context().error, "texture decode failed");
}

TEST(GameLoadingCoordinator, ConsumesASecondEntryAfterFirstRequestCompletes) {
    mxh::client::CEngine engine;
    mxh::client::GameLoadingCoordinator coordinator;

    engine.SetPendingTransfer(mxh::client::GameEntryRequest{101u, 10u});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    EXPECT_EQ(coordinator.request().character_id, 101u);
    EXPECT_EQ(coordinator.request().map_num, 10u);

    // The first request remains available for evidence while the engine's
    // transfer slot is empty; a new transfer must still be consumable.
    engine.SetPendingTransfer(mxh::client::GameEntryRequest{202u, 12u});
    ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
    EXPECT_EQ(coordinator.request().character_id, 202u);
    EXPECT_EQ(coordinator.request().map_num, 12u);
    EXPECT_EQ(coordinator.context().completed_steps, 0u);
    EXPECT_FALSE(coordinator.context().failed);
}

TEST(GameLoadingCoordinator, RestoresPreviousGameOnlyWithCompleteScene) {
    EXPECT_TRUE(can_restore_previous_game_after_map_change(true, true, true, true));
    EXPECT_FALSE(can_restore_previous_game_after_map_change(false, true, true, true));
    EXPECT_FALSE(can_restore_previous_game_after_map_change(true, false, true, true));
    EXPECT_FALSE(can_restore_previous_game_after_map_change(true, true, false, true));
    EXPECT_FALSE(can_restore_previous_game_after_map_change(true, true, true, false));
}

TEST(GameLoadingCoordinator, RepeatedMapRequestsResetProgressForSoakLoop) {
    CEngine engine;
    GameLoadingCoordinator coordinator;
    for (std::uint32_t iteration = 0; iteration < 50; ++iteration) {
        engine.SetPendingTransfer(GameEntryRequest{1000u + iteration,
                                                   static_cast<std::uint16_t>(
                                                       iteration % 3 == 0 ? 10 : 12)});
        ASSERT_TRUE(coordinator.consume_pending_transfer(engine));
        coordinator.mark_completed(10);
        EXPECT_EQ(coordinator.context().completed_steps, 10u);
        EXPECT_FALSE(coordinator.context().failed);
        EXPECT_FALSE(coordinator.context().cancelled);
    }
}
