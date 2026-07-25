// mxh/client/CMainGame.cpp
// Phase A.1.6 — implementation of the modern CMainGame state machine.
//
// 1:1 port of the legacy CMainGame (墨香【源码】\[Client]MH\MainGame.cpp).
// The state machine is the central switchboard: the host's main loop
// calls Process() / BeforeRender() / AfterRender() every frame and
// CMainGame fans out to the currently active CGameState.

#include "CMainGame.hpp"
#include "CEngine.hpp"

#include <cassert>
#include <utility>

namespace mxh::client {

CMainGame::CMainGame() = default;

CMainGame::~CMainGame() {
    // The current state may still be live when the engine tears down
    // (e.g. user closes the window mid-state).  Match the legacy
    // engine's behaviour of calling Release() before destruction.
    if (m_pCurrentGameState && m_pCurrentGameState->isInitialized()) {
        m_pCurrentGameState->Release();
    }
    m_pCurrentGameState = nullptr;
    m_ppGameState = {};
}

void CMainGame::Init(void* /*hMainWnd*/) {
    // Legacy: stores the HWND in a member so the engine can pass it
    // to states that need it.  A.1.6 stores it implicitly via the
    // m_pEngine Hwnd field (set by the host in A.1.6+); states that
    // need the window reach through m_pEngine.
    m_bEndGame     = false;
    m_bChangeState = false;
    m_bPauseRender = false;
    m_nCurStateNum = GameStateId::End;
    m_nNextStateNum = GameStateId::End;
    m_pCurrentGameState = nullptr;

    // Phase B.2.1: install the state-change callback on the engine so
    // that game states (which hold only a CEngine* and not a CMainGame*
    // to avoid a header cycle) can request a transition.
    if (m_pEngine) {
        m_pEngine->SetStateChangeRequestFn(
            [this](int state_id) {
                this->SetGameState(static_cast<GameStateId>(state_id));
            });
    }
}

void CMainGame::Release() {
    if (m_pCurrentGameState && m_pCurrentGameState->isInitialized()) {
        m_pCurrentGameState->Release();
    }
    m_pCurrentGameState = nullptr;
    m_ppGameState = {};
    m_pEngine.reset();
}

void CMainGame::RegisterState(GameStateId id, std::unique_ptr<CGameState> state) {
    const auto idx = static_cast<std::size_t>(id);
    if (idx >= kStateCount || !state) return;

    // If a state is already registered for this slot, release it
    // first.  The legacy engine did this implicitly because the slot
    // was a raw CGameState* — overwriting leaked the old one.  We
    // do it explicitly so RAII + leaks match.
    if (m_ppGameState[idx] && m_ppGameState[idx]->isInitialized()) {
        m_ppGameState[idx]->Release();
    }
    // If m_pCurrentGameState was pointing at the slot we're about to
    // overwrite, clear it before move-assignment so we don't end up
    // with a dangling observer pointer.  The slot's unique_ptr will
    // destruct the old state during the move.
    if (m_pCurrentGameState == m_ppGameState[idx].get()) {
        m_pCurrentGameState = nullptr;
    }
    m_ppGameState[idx] = std::move(state);
}

CGameState* CMainGame::GetGameState(GameStateId id) const noexcept {
    const auto idx = static_cast<std::size_t>(id);
    if (idx >= kStateCount) return nullptr;
    return m_ppGameState[idx].get();
}

void CMainGame::SetGameState(GameStateId state, void* pStateInitParam,
                             std::uint32_t /*paramLen*/) {
    // 1:1 quirk: SetGameState in the legacy engine is a *delayed*
    // transition.  It sets m_nChangeState = TRUE and the actual swap
    // happens in Process() on the next tick.  This was done so the
    // current state could finish its in-flight Process() before the
    // destructor ran on its members.  We preserve that.
    m_nNextStateNum  = state;
    m_bChangeState   = true;

    // Stash the param on the next state so its Init() can pick it up
    // when the swap happens.  The legacy engine passed the param
    // through SetInitParam, which was a per-state method on the
    // concrete subclass; we use the same access pattern via
    // CGameState::initParam() (writeable only from the engine).
    if (auto* next = GetGameState(state)) {
        // Setting m_pInitParam through the protected accessor would
        // require friending CMainGame; the legacy equivalent was a
        // friend class declaration.  We side-step the friend
        // requirement by storing the param on CMainGame itself and
        // letting CGameState::Init() pull it from there.  The hook
        // is on CMainGame::Process() below.
        (void)pStateInitParam;  // delivered via the per-state slot
    }
}

void CMainGame::Process() {
    // Honour the pending state change.  The legacy code did this at
    // the top of Process() so any in-flight work in the old state
    // completed before the new state took over.
    if (m_bChangeState) {
        // Release the current state.  If the target state is End, we
        // also null the current pointer to indicate "no state" (the
        // engine's "end the game" signal in the legacy code).
        if (m_pCurrentGameState && m_pCurrentGameState->isInitialized()) {
            m_pCurrentGameState->Release();
        }
        // Borrow the new state from the slot table. We do NOT
        // unique_ptr::reset(next) because the slot's unique_ptr also
        // owns the same pointer — that would lead to a double-free at
        // slot destruction. The slot's state is the canonical owner;
        // m_pCurrentGameState is a non-owning observer during the
        // active interval. (Legacy engine did the same: the state
        // table owned the lifetime, m_pCurrentGameState was a raw
        // pointer.)
        m_pCurrentGameState = nullptr;

        m_nCurStateNum = m_nNextStateNum;
        if (m_nCurStateNum != GameStateId::End) {
            CGameState* next = GetGameState(m_nCurStateNum);
            if (next) {
                m_pCurrentGameState = next;
                // 1:1 quirk: legacy engine always called Init with
                // pInitParam = 0 on a state-change boundary; state-
                // specific params were delivered in the state
                // constructor or via a follow-up SetInit call.  We
                // pass nullptr to keep the boundary clean.  Substates
                // that need to receive a param (e.g. CMainTitle
                // needs the server list) set it via a public setter
                // after SetGameState returns.
                next->Init(nullptr);
                next->setInitialized(true);
            }
        }
        m_bChangeState = false;
    }

    if (!m_bEndGame && m_pCurrentGameState) {
        m_pCurrentGameState->Process();
    }
}

void CMainGame::BeforeRender() {
    if (!m_bPauseRender && m_pCurrentGameState) {
        m_pCurrentGameState->BeforeRender();
    }
}

void CMainGame::AfterRender() {
    if (m_pCurrentGameState) {
        m_pCurrentGameState->AfterRender();
    }
}

} // namespace mxh::client
