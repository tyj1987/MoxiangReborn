// mxh/client/GameStateStubs.hpp
// Concrete state declarations retained for the legacy eGAMESTATE table.
// Login, character select/create, GameIn, loading and map change expose their
// real modern lifecycle here; only IntroReplay and MurimNet remain lightweight
// compatibility states.
//
// The class names match the legacy naming (CMainTitle, CGameIn, ...)
// 1:1 so a search-and-replace across the legacy source translates
// directly into modern C++.

#pragma once

#include <string>

#include "CGameState.hpp"
#include "CMainTitle.hpp"
#include "CLoginState.hpp"
#include "CCharSelectState.hpp"
#include "CInGameState.hpp"

namespace mxh::client {

struct LoadStateContext {
    std::uint16_t map_num = 0;
    std::uint32_t character_id = 0;
    std::uint32_t completed_steps = 0;
    std::uint32_t total_steps = 10;
    bool cancelled = false;
    bool failed = false;
    const char* error = nullptr;
};

// -------------------------------------------------------------------------
// CIntroReplay (legacy: CIntroReplayDlg) — compatibility state
// -------------------------------------------------------------------------
class CIntroReplay : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CConnecting was the placeholder for eGS_CONNECT (= 2).  Phase B.2.1
// replaces it with CLoginState (see CLoginState.hpp) which drives the
// real login handshake against MoxianLoginServer.  The CConnecting
// class is gone; registrations should use CLoginState.
//
// CCharSelect (eGS_CHARSELECT = 4) is now CCharSelectState (Phase B.2.2,
// see CCharSelectState.hpp) which drives the real character-list +
// character-select handshake against MoxianAgentServer.  The CCharSelect
// stub class is gone.
//
// CCharMake (eGS_CHARMAKE = 5) is now CCharMake (Phase B.4, see
// CCharMake.hpp) which drives the real character-creation handshake
// against MoxianAgentServer.  The CCharMake stub class is gone.
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
// CGameLoading — real map loading screen with progress and cancellation.
// -------------------------------------------------------------------------
class CGameLoading : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
    void Start(CEngine* engine);
    float progress() const noexcept { return m_progress; }
    bool failed() const noexcept { return m_failed; }
    bool cancelled() const noexcept { return m_cancelled; }
    bool completed() const noexcept { return !m_failed && !m_cancelled && m_progress >= 1.0f; }
    const std::string& error() const noexcept { return m_error; }
    ClientUiRuntime& ui_runtime() noexcept { return m_uiRuntime; }
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& ui_dialogs() const noexcept {
        return m_uiRuntime.dialogs();
    }
    void set_context(const LoadStateContext* context) noexcept {
        SetInitParam(const_cast<LoadStateContext*>(context));
    }
private:
    float m_progress = 0.0f;
    bool m_failed = false;
    bool m_cancelled = false;
    std::string m_error;
    ClientUiRuntime m_uiRuntime;
};

// -------------------------------------------------------------------------
// CGameIn (eGS_GAMEIN = 7) was the placeholder until Phase B.2.3
// replaced it with CInGameState (see CInGameState.hpp) which drives
// the GameIn handshake against MoxianMapServer.  The CGameIn stub
// class is gone.
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
// CMapChange — between-map transition state sharing the load pipeline.
// -------------------------------------------------------------------------
class CMapChange : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
    float progress() const noexcept { return m_progress; }
    bool failed() const noexcept { return m_failed; }
    bool cancelled() const noexcept { return m_cancelled; }
    bool completed() const noexcept { return !m_failed && !m_cancelled && m_progress >= 1.0f; }
    const std::string& error() const noexcept { return m_error; }
    void Start(CEngine* engine);
    void set_context(const LoadStateContext* context) noexcept {
        SetInitParam(const_cast<LoadStateContext*>(context));
    }
    ClientUiRuntime& ui_runtime() noexcept { return m_uiRuntime; }
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& ui_dialogs() const noexcept {
        return m_uiRuntime.dialogs();
    }
private:
    float m_progress = 0.0f;
    bool m_failed = false;
    bool m_cancelled = false;
    std::string m_error;
    ClientUiRuntime m_uiRuntime;
};

// -------------------------------------------------------------------------
// CMurimNet — MurimNet PvP lobby.  Lands in D.5 (Phase D).
// -------------------------------------------------------------------------
class CMurimNet : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

} // namespace mxh::client
