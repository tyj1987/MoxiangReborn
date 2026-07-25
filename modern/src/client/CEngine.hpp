// mxh/client/CEngine.hpp
// Phase A.1.6 — minimal CEngine stub.
//
// 1:1 port of the legacy CEngine class (墨香【源码】\[Client]MH\Engine.h)
// which glues the renderer, network, audio, and input layers.  A.1.6
// ships an empty CEngine so CMainGame's unique_ptr<CEngine> compiles;
// A.1.6+ fills it in with the real BaseNetwork + 4Dyuchi renderer +
// Miles Sound + FreeImage handles.
//
// Modern port notes:
//   * The legacy CEngine owned the HWND and was the place every
//     subsystem looked up "where am I drawing".  A.1.6 stores the
//     HWND in the same field; the MoxianClient host fills it in
//     during WinMain after CreateWindow() returns.
//   * m_pRenderer (I4DyuchiGXRenderer*) is borrowed — the host owns
//     the renderer.  We don't take a refcount.
//   * m_pNetwork is a forward-declared pointer to the future
//     BaseNetwork wrapper; A.1.6 leaves it nullptr until the network
//     layer is wired in (A.3).
//   * Phase B.2.1: RequestStateChange() is the indirect hook that
//     game states (e.g. CLoginState) use to trigger a state transition
//     without holding a back-pointer to CMainGame (which would create
//     a header cycle).  The callback is installed by CMainGame::Init
//     and forwards to SetGameState() with the int -> GameStateId cast.
#pragma once

#include <cstdint>
#include <functional>

#include "mxh/render/IRenderer.hpp"

namespace mxh::client {

class CEngine {
public:
    CEngine() = default;
    ~CEngine() = default;

    CEngine(const CEngine&)            = delete;
    CEngine& operator=(const CEngine&) = default;

    // HWND is set once by the host after CreateWindowW().
    void  SetHwnd(void* h) noexcept      { m_hWnd = h; }
    void* GetHwnd() const noexcept       { return m_hWnd; }

    // The host sets the renderer once CreateGXRendererInstance + Create
    // have been called.  CEngine doesn't own it.
    void SetRenderer(mxh::gx::I4DyuchiGXRenderer* r) noexcept { m_pRenderer = r; }
    mxh::gx::I4DyuchiGXRenderer* GetRenderer() const noexcept { return m_pRenderer; }

    // Lifecycle.  Init is called once at startup; Release once at
    // shutdown.  Both are stubs in A.1.6.
    void Init()    { m_bInitialized = true; }
    void Release() { m_bInitialized = false; m_pRenderer = nullptr; }
    bool isInitialized() const noexcept { return m_bInitialized; }

    // ---------------------------------------------------------------------
    // State-change request (Phase B.2.1).  CMainGame::Init installs the
    // callback that actually calls SetGameState().  Game states call
    // RequestStateChange(GameStateId::X) to request a transition.  The
    // int encoding avoids a header cycle with CMainGame.hpp.
    // ---------------------------------------------------------------------
    using StateChangeFn = std::function<void(int /*GameStateId*/)>;
    void SetStateChangeRequestFn(StateChangeFn fn) noexcept {
        m_stateChangeFn = std::move(fn);
    }
    void RequestStateChange(int state_id) const {
        if (m_stateChangeFn) m_stateChangeFn(state_id);
    }

private:
    void*                           m_hWnd         = nullptr;
    mxh::gx::I4DyuchiGXRenderer*    m_pRenderer    = nullptr;
    bool                            m_bInitialized = false;
    StateChangeFn                   m_stateChangeFn;
    // m_pNetwork, m_pAudio, m_pInput land in A.1.6+ when those layers
    // are wired in.  Kept out of A.1.6 to keep the surface minimal.
};

} // namespace mxh::client
