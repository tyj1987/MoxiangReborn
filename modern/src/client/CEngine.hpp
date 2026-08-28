// mxh/client/CEngine.hpp
// Modern CEngine runtime coordinator.
//
// 1:1 port of the legacy CEngine class (墨香【源码】\[Client]MH\Engine.h)
// which glues the renderer, network, audio, and input layers.  The modern
// implementation keeps ownership explicit: the host owns the renderer while
// CEngine owns the persistent AgentSession and state-transfer boundary.
//
// Modern port notes:
//   * The legacy CEngine owned the HWND and was the place every
//     subsystem looked up "where am I drawing".  A.1.6 stores the
//     HWND in the same field; the MoxianClient host fills it in
//     during WinMain after CreateWindow() returns.
//   * m_pRenderer (I4DyuchiGXRenderer*) is borrowed — the host owns
//     the renderer.  We don't take a refcount.
//   * Network lifetime is represented by AgentSession, which survives
//     Title -> CharSelect -> GameLoading -> GameIn transitions.
//   * Phase B.2.1: RequestStateChange() is the indirect hook that
//     game states (e.g. CLoginState) use to trigger a state transition
//     without holding a back-pointer to CMainGame (which would create
//     a header cycle).  The callback is installed by CMainGame::Init
//     and forwards to SetGameState() with the int -> GameStateId cast.
//   * Phase B.2.2: SetPendingTransfer / TakePendingTransfer is a
//     single-slot typed variant that lets an outgoing state hand a payload
//     (e.g. LoginResult) to the next state without a back-reference.
#pragma once

#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <optional>
#include <variant>
#include <utility>

#include "mxh/render/IRenderer.hpp"
#include "mxh/ui/resolution_mode.hpp"
#include "AgentSession.hpp"
#include "StateTransfer.hpp"

namespace mxh::client {

class CEngine {
public:
    enum class AudioCue : std::uint8_t { Attack, Skill, UiClick, Pickup };
    using AudioEventFn = std::function<void(AudioCue)>;
    using SpatialAudioEventFn = std::function<void(AudioCue, float)>;
    using SoundEventFn = std::function<void(std::uint32_t, float)>;
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

    // M-R7.1 (G3 bug fix 2026-08-20): host sets the PlayDH resource
    // root once.  Game states (CCharSelectState / CCharMake) read it
    // to load their cDialog trees from <root>/Image/InterfaceScript/.
    void SetPlaydhRoot(std::filesystem::path p) noexcept { m_playdhRoot = std::move(p); }
    const std::optional<std::filesystem::path>& playdh_root() const noexcept {
        return m_playdhRoot;
    }

    void SetUiResolutionMode(mxh::ui::ResolutionMode mode) noexcept {
        m_uiResolutionMode = mode;
    }
    mxh::ui::ResolutionMode ui_resolution_mode() const noexcept {
        return m_uiResolutionMode;
    }

    // Lifecycle. Init is called once at startup; Release once at shutdown.
    void Init()    { m_bInitialized = true; }
    void Release() {
        m_agentSession.disconnect();
        m_bInitialized = false;
        m_pRenderer = nullptr;
    }
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

    // ---------------------------------------------------------------------
    // Transfer slot (Phase B.2.2).  One typed payload pending handoff to
    // the next state.  Set overwrites; Take returns and clears.  Both
    // sides type-check with std::any_cast.
    // ---------------------------------------------------------------------
    void SetPendingTransfer(StateTransfer value) { m_pendingTransfer = std::move(value); }
    StateTransfer TakePendingTransfer() {
        StateTransfer out = std::move(m_pendingTransfer);
        m_pendingTransfer = std::monostate{};
        return out;
    }
    bool has_pending_transfer() const noexcept {
        return !std::holds_alternative<std::monostate>(m_pendingTransfer);
    }
    template <class T>
    bool pending_transfer_is() const noexcept {
        return std::holds_alternative<T>(m_pendingTransfer);
    }

    AgentSession& agent_session() noexcept { return m_agentSession; }
    const AgentSession& agent_session() const noexcept { return m_agentSession; }
    void SetAudioEventFn(AudioEventFn fn) noexcept { m_audioEventFn = std::move(fn); }
    void EmitAudio(AudioCue cue) const { if (m_audioEventFn) m_audioEventFn(cue); }
    void SetSpatialAudioEventFn(SpatialAudioEventFn fn) noexcept { m_spatialAudioEventFn = std::move(fn); }
    void EmitAudioAt(AudioCue cue, float distance) const {
        if (m_spatialAudioEventFn) m_spatialAudioEventFn(cue, std::max(0.0f, distance));
        else EmitAudio(cue);
    }
    void SetSoundEventFn(SoundEventFn fn) noexcept { m_soundEventFn = std::move(fn); }
    void EmitSoundAt(std::uint32_t sound_id, float distance) const {
        if (m_soundEventFn) m_soundEventFn(sound_id, std::max(0.0f, distance));
    }

private:
    void*                           m_hWnd         = nullptr;
    mxh::gx::I4DyuchiGXRenderer*    m_pRenderer    = nullptr;
    bool                            m_bInitialized = false;
    std::optional<std::filesystem::path> m_playdhRoot;
    mxh::ui::ResolutionMode m_uiResolutionMode =
        mxh::ui::ResolutionMode::Low800x600;
    StateChangeFn                   m_stateChangeFn;
    StateTransfer                   m_pendingTransfer;
    AgentSession                    m_agentSession;
    AudioEventFn                    m_audioEventFn;
    SpatialAudioEventFn             m_spatialAudioEventFn;
    SoundEventFn                    m_soundEventFn;
    // m_pNetwork, m_pAudio, m_pInput land in A.1.6+ when those layers
    // are wired in.  Kept out of A.1.6 to keep the surface minimal.
};

} // namespace mxh::client
