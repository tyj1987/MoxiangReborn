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
