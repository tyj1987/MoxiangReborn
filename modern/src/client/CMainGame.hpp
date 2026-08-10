// mxh/client/CMainGame.hpp
// Phase A.1.6 — modern C++17 1:1 port of legacy CMainGame.
//
// The legacy CMainGame (墨香【源码】\[Client]MH\MainGame.h) drives the
// client state machine: 9 eGAMESTATE slots, a pCurrentGameState pointer
// that gets swapped on SetGameState(), and a per-frame Process / Before
// / After triple that delegates to the active state.
//
// Modern port notes:
//   * We use std::array<std::unique_ptr<CGameState>, kStateCount> instead
//     of the legacy CGameState** + manual new/delete.  RAII handles
//     destruction at CMainGame's own dtor.
//   * m_pEngine (the legacy BaseNetwork + 4Dyuchi engine) is
//     deliberately NOT included in A.1.6 — the renderer is created by
//     MoxianClient directly today (it owns the HWND).  A.1.6 keeps
//     m_pEngine as a forward-declared pointer that the host fills in.
//   * GetUserLevel/SetUserLevel and the "user is admin / GM" flag are
//     preserved verbatim so the GM tool path can plug in unchanged
//     later.
//   * PauseRender() is included; the engine pauses the renderer while
//     the legacy main loop is blocked on a modal dialog.
#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "CGameState.hpp"

namespace mxh::client {

// 1:1 port of the legacy eGAMESTATE enum.  Order matches the legacy
// (END=0, INTRO=1, ...).  Do NOT renumber — Protocol.h, save files,
// and the dist server's state machine all depend on the values.
enum class GameStateId : int {
    End        = 0,
    Intro      = 1,  // magi82 - Intro(070712)
    Connect    = 2,
    Title      = 3,
    CharSelect = 4,
    CharMake   = 5,
    GameLoading = 6,
    GameIn     = 7,
    MapChange  = 8,
    MurimNet   = 9,
};
constexpr std::size_t kStateCount = 10;

class CEngine;  // forward — the host (MoxianClient) fills this in.

class CMainGame {
public:
    CMainGame();
    ~CMainGame();

    CMainGame(const CMainGame&)            = delete;
    CMainGame& operator=(const CMainGame&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle.  Init takes the HWND so states that need to know
    // "where am I drawing" can pass it through to dialogs.
    // -------------------------------------------------------------------------
    void Init(void* hMainWnd);
    void Release();

    // -------------------------------------------------------------------------
    // State transitions.  SetGameState triggers:
    //   1. current->Release()
    //   2. next->Init(pStateInitParam)
    //   3. swap current pointer
    // The legacy engine accepted a NULL ParamLen; we accept it as a
    // default 0.  When transitioning to End the engine is left alone
    // (the caller closes the window).
    // -------------------------------------------------------------------------
    void SetGameState(GameStateId state, void* pStateInitParam = nullptr,
                      std::uint32_t paramLen = 0);

    // -------------------------------------------------------------------------
    // Per-frame.  Process / BeforeRender / AfterRender are called in
    // that order by the host's main loop.  They no-op if no state is
    // current (e.g. before the first SetGameState()).
    // -------------------------------------------------------------------------
    void Process();
    void BeforeRender();
    void AfterRender();

    // -------------------------------------------------------------------------
    // State registration.  CMainGame owns the slots; callers (e.g. the
    // legacy-equivalent main.cpp that constructs CMainTitle, CGameIn,
    // ...) hand CMainGame a heap-allocated CGameState* during their
    // own Init.  Take ownership is true (default) — CMainGame deletes
    // the state in its dtor.
    //
    // Idempotency: if a state is already registered for a slot, the
    // existing one is replaced.  This matches the legacy engine which
    // rebuilt the state table on every launch.
    // -------------------------------------------------------------------------
    void RegisterState(GameStateId id, std::unique_ptr<CGameState> state);

    CGameState* GetGameState(GameStateId id) const noexcept;
    CGameState* GetCurGameState() const noexcept          { return m_pCurrentGameState; }

    bool IsChangeState() const noexcept                    { return m_bChangeState; }
    GameStateId GetCurStateNum() const noexcept            { return m_nCurStateNum; }

    int  GetUserLevel() const noexcept                     { return m_nUserLevel; }
    void SetUserLevel(int n) noexcept                      { m_nUserLevel = n; }

    void PauseRender(bool bPause) noexcept                 { m_bPauseRender = bPause; }
    bool isPaused() const noexcept                        { return m_bPauseRender; }

    CEngine* GetEngine() const noexcept                    { return m_pEngine.get(); }
    void SetEngine(std::unique_ptr<CEngine> e) noexcept;

private:
    // Forward-declared CEngine held via unique_ptr to keep the include
    // out of the header.  The concrete CEngine definition lands in
    // A.1.6+ when the engine class is ported (BaseNetwork + 4Dyuchi
    // glue + the DX11 renderer handle).
    std::unique_ptr<CEngine> m_pEngine;

    // State table.  m_ppGameState[index] is heap-owned and is freed
    // in our dtor (unique_ptr).  Index matches GameStateId.
    std::array<std::unique_ptr<CGameState>, kStateCount> m_ppGameState{};

    // Non-owning observer into the state table.  m_pCurrentGameState
    // always equals m_ppGameState[m_nCurStateNum].get() when active
    // (or nullptr after a transition to End).  We use a raw pointer
    // so there's no ambiguity about ownership — the slot owns the
    // state, this is just a "which slot is active" cache.
    CGameState*                   m_pCurrentGameState = nullptr;

    bool        m_bEndGame         = false;
    bool        m_bChangeState     = false;
    bool        m_bPauseRender     = false;
    GameStateId m_nCurStateNum      = GameStateId::End;
    GameStateId m_nNextStateNum     = GameStateId::End;
    int         m_nUserLevel       = 0;  // 0 = normal user; >0 = GM
};

} // namespace mxh::client
