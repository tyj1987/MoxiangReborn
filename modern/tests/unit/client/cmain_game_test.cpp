// mxh/tests/unit/client/cmain_game_test.cpp
// Unit tests for mxh::client::CMainGame state machine (Phase A.1.6).
//
// Locks down the 1:1 port's behaviour:
//   * State IDs match the legacy eGAMESTATE values byte-for-byte.
//   * State registration is idempotent (re-Register replaces the
//     previous state for the same slot).
//   * SetGameState is a *delayed* transition: the swap happens on the
//     next Process() call, not synchronously.
//   * Setting state to End leaves m_pCurrentGameState as nullptr and
//     is a clean "kill switch".
//   * GetGameState / GetCurGameState return the registered objects.
//   * SetUserLevel / GetUserLevel round-trip the GM flag.

#include "CMainGame.hpp"
#include "CEngine.hpp"
#include "CGameState.hpp"
#include "GameStateStubs.hpp"

#include <gtest/gtest.h>

#include <memory>

using mxh::client::CMainGame;
using mxh::client::CGameState;
using mxh::client::GameStateId;
using mxh::client::kStateCount;
using mxh::client::CMainTitle;

namespace {

// Helper: a state subclass that records Init/Release calls and bumps
// an internal counter on Process so tests can assert that the driver
// called into the state.
struct SpyState : public CGameState {
    int initCalls    = 0;
    int releaseCalls = 0;
    int processCalls = 0;

    void Init(void* p) override    { ++initCalls;    (void)p; setInitialized(true); }
    void Release() override        { ++releaseCalls; setInitialized(false); }
    void Process() override        { ++processCalls; }
};

// SpyStateWithGlobalCounter — a variant of SpyState that bumps a
// process-wide counter on Release().  Used to verify that
// RegisterState called Release() on the old state even when the
// test doesn't retain a handle to that state (because the slot's
// unique_ptr will destruct it during move-assignment).
struct SpyStateWithGlobalCounter : public CGameState {
    static int g_releaseCount;
    int         initCalls       = 0;

    void Init(void*) override    { ++initCalls; setInitialized(true); }
    void Release() override      { ++g_releaseCount; setInitialized(false); }
};
int SpyStateWithGlobalCounter::g_releaseCount = 0;

} // namespace

TEST(CMainGameStateIds, MatchesLegacyEnum) {
    EXPECT_EQ(static_cast<int>(GameStateId::End),        0);
    EXPECT_EQ(static_cast<int>(GameStateId::Intro),      1);
    EXPECT_EQ(static_cast<int>(GameStateId::Connect),    2);
    EXPECT_EQ(static_cast<int>(GameStateId::Title),      3);
    EXPECT_EQ(static_cast<int>(GameStateId::CharSelect), 4);
    EXPECT_EQ(static_cast<int>(GameStateId::CharMake),   5);
    EXPECT_EQ(static_cast<int>(GameStateId::GameLoading), 6);
    EXPECT_EQ(static_cast<int>(GameStateId::GameIn),     7);
    EXPECT_EQ(static_cast<int>(GameStateId::MapChange),  8);
    EXPECT_EQ(static_cast<int>(GameStateId::MurimNet),   9);
    EXPECT_EQ(kStateCount, 10u);
}

TEST(CMainGameStateIds, IsDelayedTransition) {
    CMainGame game;
    game.Init(nullptr);

    auto title = std::make_unique<SpyState>();
    auto* rawTitle = title.get();
    game.RegisterState(GameStateId::Title, std::move(title));
    EXPECT_EQ(game.GetGameState(GameStateId::Title), rawTitle);
    EXPECT_EQ(game.GetCurGameState(), nullptr);
    EXPECT_FALSE(game.IsChangeState());
    EXPECT_EQ(game.GetCurStateNum(), GameStateId::End);

    game.SetGameState(GameStateId::Title);
    EXPECT_TRUE(game.IsChangeState());
    EXPECT_EQ(rawTitle->initCalls, 0);
    EXPECT_EQ(game.GetCurGameState(), nullptr);

    game.Process();
    EXPECT_FALSE(game.IsChangeState());
    EXPECT_EQ(rawTitle->initCalls, 1);
    EXPECT_EQ(game.GetCurGameState(), rawTitle);
    EXPECT_EQ(game.GetCurStateNum(), GameStateId::Title);

    game.Release();
}

TEST(CMainGameStateIds, ProcessDelegatesToCurrent) {
    CMainGame game;
    game.Init(nullptr);
    auto title = std::make_unique<SpyState>();
    auto* rawTitle = title.get();
    game.RegisterState(GameStateId::Title, std::move(title));
    game.SetGameState(GameStateId::Title);
    // First Process() does the swap AND drives the state.
    // 3 Process() calls in total => 3 state ticks.
    game.Process();
    game.Process();
    game.Process();
    EXPECT_EQ(rawTitle->processCalls, 3);
    game.Release();
}

TEST(CMainGameStateIds, SetGameStateEndClearsCurrent) {
    CMainGame game;
    game.Init(nullptr);
    auto title = std::make_unique<SpyState>();
    auto* rawTitle = title.get();
    game.RegisterState(GameStateId::Title, std::move(title));
    game.SetGameState(GameStateId::Title);
    game.Process();
    ASSERT_EQ(game.GetCurGameState(), rawTitle);

    game.SetGameState(GameStateId::End);
    game.Process();
    EXPECT_EQ(game.GetCurGameState(), nullptr);
    EXPECT_EQ(rawTitle->releaseCalls, 1);
    game.Release();
}

TEST(CMainGameStateIds, RegisterReplacesExisting) {
    CMainGame game;
    game.Init(nullptr);

    auto a = std::make_unique<SpyState>();
    auto b = std::make_unique<SpyState>();
    auto* rawA = a.get();
    auto* rawB = b.get();
    game.RegisterState(GameStateId::Title, std::move(a));
    EXPECT_EQ(game.GetGameState(GameStateId::Title), rawA);

    // Re-registering the same slot replaces the old state.  The old
    // state is released via the standard destructor (RAII) — we don't
    // observe a Release() callback here because the old state was
    // never Initialised.
    game.RegisterState(GameStateId::Title, std::move(b));
    EXPECT_EQ(game.GetGameState(GameStateId::Title), rawB);

    // If the old state had been Initialised, RegisterState should
    // call Release() on it before swapping.  We use
    // SpyStateWithGlobalCounter so the assertion survives the slot
    // overwrite (the local unique_ptr is gone after RegisterState).
    {
        const int before = SpyStateWithGlobalCounter::g_releaseCount;
        auto c = std::make_unique<SpyStateWithGlobalCounter>();
        game.RegisterState(GameStateId::GameIn, std::move(c));
        // c is now owned by m_ppGameState[GameIn] but is not yet
        // Initialised.  Initialise it through the state-change path.
        game.SetGameState(GameStateId::GameIn);
        game.Process();
        // Re-register with a fresh state.  The old one (c) should
        // be Released by RegisterState.  We can't read c anymore
        // (its unique_ptr was moved into the slot, then destroyed
        // by the second RegisterState's move-assignment), but the
        // global counter picks up the Release() call.
        auto d = std::make_unique<SpyStateWithGlobalCounter>();
        auto* rawD = d.get();
        game.RegisterState(GameStateId::GameIn, std::move(d));
        EXPECT_EQ(game.GetGameState(GameStateId::GameIn), rawD);
        EXPECT_EQ(SpyStateWithGlobalCounter::g_releaseCount, before + 1)
            << "RegisterState should call Release() on the old, "
               "Initialised state before swapping";
    }

    game.Release();
}

TEST(CMainGameStateIds, ReleaseOnDtorCallsInitializedState) {
    SpyStateWithGlobalCounter::g_releaseCount = 0;
    auto title = std::make_unique<SpyStateWithGlobalCounter>();
    {
        CMainGame game;
        game.Init(nullptr);
        game.RegisterState(GameStateId::Title, std::move(title));
        game.SetGameState(GameStateId::Title);
        game.Process();
        // dtor should call Release() on the current state because
        // it's still Initialised.
    }
    EXPECT_GE(SpyStateWithGlobalCounter::g_releaseCount, 1)
        << "CMainGame dtor should Release() the current state";
}

TEST(CMainGameStateIds, UserLevelRoundTrip) {
    CMainGame game;
    EXPECT_EQ(game.GetUserLevel(), 0);
    game.SetUserLevel(7);
    EXPECT_EQ(game.GetUserLevel(), 7);
    game.SetUserLevel(0);
    EXPECT_EQ(game.GetUserLevel(), 0);
}

TEST(CMainGameStateIds, PauseRenderRoundTrip) {
    CMainGame game;
    EXPECT_FALSE(game.isPaused());
    game.PauseRender(true);
    EXPECT_TRUE(game.isPaused());
    game.PauseRender(false);
    EXPECT_FALSE(game.isPaused());
}

TEST(CMainGameStateIds, BeforeRenderHonorsPause) {
    CMainGame game;
    game.Init(nullptr);

    struct BeforeSpy : public CGameState {
        int beforeCalls = 0, afterCalls = 0;
        void Init(void*) override { setInitialized(true); }
        void Release() override { setInitialized(false); }
        void BeforeRender() override { ++beforeCalls; }
        void AfterRender()  override { ++afterCalls; }
    };
    auto spy = std::make_unique<BeforeSpy>();
    auto* rawSpy = spy.get();
    game.RegisterState(GameStateId::Title, std::move(spy));
    game.SetGameState(GameStateId::Title);
    game.Process();

    game.PauseRender(true);
    game.BeforeRender();
    game.AfterRender();
    EXPECT_EQ(rawSpy->beforeCalls, 0);
    EXPECT_EQ(rawSpy->afterCalls, 1);

    game.PauseRender(false);
    game.BeforeRender();
    game.AfterRender();
    EXPECT_EQ(rawSpy->beforeCalls, 1);
    EXPECT_EQ(rawSpy->afterCalls, 2);

    game.Release();
}

TEST(CMainGameStateIds, StubsRegister) {
    CMainGame game;
    game.Init(nullptr);
    game.RegisterState(GameStateId::Intro,      std::make_unique<mxh::client::CIntroReplay>());
    game.RegisterState(GameStateId::Connect,    std::make_unique<mxh::client::CLoginState>());
    game.RegisterState(GameStateId::Title,      std::make_unique<mxh::client::CMainTitle>());
    game.RegisterState(GameStateId::CharSelect, std::make_unique<mxh::client::CCharSelectState>());
    game.RegisterState(GameStateId::CharMake,   std::make_unique<mxh::client::CCharMake>());
    game.RegisterState(GameStateId::GameLoading,std::make_unique<mxh::client::CGameLoading>());
    game.RegisterState(GameStateId::GameIn,     std::make_unique<mxh::client::CInGameState>());
    game.RegisterState(GameStateId::MapChange,  std::make_unique<mxh::client::CMapChange>());
    game.RegisterState(GameStateId::MurimNet,   std::make_unique<mxh::client::CMurimNet>());

    for (int i = 1; i <= 9; ++i) {
        EXPECT_NE(game.GetGameState(static_cast<GameStateId>(i)), nullptr)
            << "state " << i << " not registered";
    }
    game.Release();
}
