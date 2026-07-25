// mxh/client/CGameState.hpp
// Phase A.1.7 — base class for Moxian game states.
//
// 1:1 port of the legacy abstract interface from
// 墨香【源码】\[Client]MH\GameState.h.  The legacy contract is small:
// each state owns Init / Release / Process / BeforeRender / AfterRender
// and the engine drives them in lockstep with the Win32 message pump.
//
// Modern port notes:
//   * `m_pInitParam` is a borrowed pointer to a state-specific init blob
//     (matches the legacy void* pInitParam).  Each subclass is
//     responsible for casting it to its own concrete type.
//   * `m_dwDialogProcessTickCount` is a single tick counter incremented
//     per Process() call.  The legacy engine uses it to drive timing-
//     sensitive UI animations (cDialog fade-in, "Loading..." progress
//     bar).  We keep the same field name + semantics.
//   * The m_bStateInitialized flag replaces the legacy "have we called
//     Init on this state" bookkeeping the engine did with m_ppGameState
//     indirection.
#pragma once

#include <cstdint>

namespace mxh::client {

class CGameState {
public:
    CGameState() = default;
    virtual ~CGameState() = default;

    CGameState(const CGameState&) = delete;
    CGameState& operator=(const CGameState&) = default;

    // -------------------------------------------------------------------------
    // Lifecycle.  Init() is called once when the engine transitions INTO
    // this state.  Release() is called once when the engine transitions
    // OUT (the next state's Init() has not been called yet at that
    // point).  Both take a borrowed void* for state-specific init
    // parameters; the concrete subclass casts to its own type.
    // -------------------------------------------------------------------------
    virtual void Init(void* pInitParam) = 0;
    virtual void Release() = 0;

    // -------------------------------------------------------------------------
    // Per-frame steps.  Process() is the main tick (input, network
    // pump, game logic).  BeforeRender() is the prepass (matrices,
    // animations).  AfterRender() is the postpresent hook.  The legacy
    // engine called them in this order on every frame.
    // -------------------------------------------------------------------------
    virtual void Process() {}
    virtual void BeforeRender() {}
    virtual void AfterRender() {}

    // -------------------------------------------------------------------------
    // Accessors used by CMainGame to keep per-state bookkeeping.
    // -------------------------------------------------------------------------
    bool isInitialized() const noexcept       { return m_bStateInitialized; }
    void setInitialized(bool v) noexcept     { m_bStateInitialized = v; }

    void* initParam() const noexcept          { return m_pInitParam; }

    // Per-state tick counter.  Wraps at 32 bits; matches the legacy
    // DWORD semantics.  Subclasses may use this to drive periodic UI
    // updates (e.g. progress bar increments every N ticks).
    std::uint32_t tickCount() const noexcept  { return m_dwDialogProcessTickCount; }

protected:
    // Default-initialised.  CMainGame calls SetInitParam() before Init()
    // so subclasses can fetch the borrowed blob from initParam().
    void SetInitParam(void* p) noexcept       { m_pInitParam = p; }

    // Subclasses bump the tick counter in their Process() override.
    // CMainGame also calls this from its own driver so the counter
    // increments even on a no-op state.
    void tick() noexcept {
        ++m_dwDialogProcessTickCount;
    }

private:
    void*           m_pInitParam                = nullptr;
    bool            m_bStateInitialized         = false;
    std::uint32_t   m_dwDialogProcessTickCount  = 0;
};

} // namespace mxh::client
