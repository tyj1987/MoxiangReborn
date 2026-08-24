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

TEST(CGameLoading, RejectsInvalidContext) {
    LoadStateContext context;
    context.total_steps = 0;
    CGameLoading state;
    state.Init(&context);
    EXPECT_TRUE(state.failed());
    EXPECT_EQ(state.error(), "loading context has zero steps");
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
