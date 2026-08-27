// mxh/client/GameStateStubs.cpp
// Shared implementation for the remaining lightweight eGAMESTATE states.
// Login, character select/create, loading, map change and GameIn have their
// dedicated implementations; only intentionally inert legacy states use the
// small lifecycle helper below.
//
// The 1:1 surface (Init / Release / Process / BeforeRender /
// AfterRender) is preserved so the host's CMainGame driver and the
// CGameState base class don't have to special-case any state.  State
// classes that the legacy engine never called BeforeRender/AfterRender
// on (most of them) inherit the no-op defaults from CGameState and
// don't override them here.

#include "GameStateStubs.hpp"
#include "CEngine.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>

namespace mxh::client {

#define MXH_STATE_STUB_IMPL(klass)                                          \
    void klass::Init(void* /*p*/) {                                         \
        MLOG_DEBUG(#klass "::Init");                                        \
        setInitialized(true);                                               \
    }                                                                       \
    void klass::Release() {                                                 \
        MLOG_DEBUG(#klass "::Release");                                     \
        setInitialized(false);                                              \
    }                                                                       \
    void klass::Process() {                                                 \
        MLOG_DEBUG(#klass "::Process");                                     \
    }

MXH_STATE_STUB_IMPL(CIntroReplay)
// CConnecting (eGS_CONNECT = 2) is now CLoginState (Phase B.2.1) —
// drives the login handshake against MoxianLoginServer.
// CMainTitle is no longer a stub — see CMainTitle.cpp (Phase A.1.8).
// CCharSelect (eGS_CHARSELECT = 4) is now CCharSelectState (Phase B.2.2)
// — drives the character-list + character-select handshake against
// MoxianAgentServer.
// CGameIn (eGS_GAMEIN = 7) is now CInGameState (Phase B.2.3) —
// drives the GameIn handshake against MoxianMapServer.
void CGameLoading::Init(void* param) {
    SetInitParam(param);
    m_progress = 0.0f;
    m_failed = false;
    m_cancelled = false;
    m_error.clear();
    const auto* context = static_cast<const LoadStateContext*>(param);
    if (context) {
        if (context->total_steps == 0) {
            m_failed = true;
            m_error = "loading context has zero steps";
        } else {
            m_progress = std::clamp(static_cast<float>(context->completed_steps) /
                                    static_cast<float>(context->total_steps), 0.0f, 1.0f);
        }
        m_cancelled = context->cancelled;
        if (context->failed) {
            m_failed = true;
            m_error = context->error ? context->error : "map loading failed";
        }
    }
    setInitialized(true);
    MLOG_INFO("CGameLoading::Init progress=%.3f failed=%d", m_progress, m_failed);
}

void CGameLoading::Start(CEngine* engine) {
    if (m_uiRuntime.empty() && (!engine || !engine->playdh_root())) {
        m_failed = true;
        m_error = !engine ? "loading requires a client engine"
                          : "loading requires an explicit PlayDH resource root";
        MLOG_ERROR("CGameLoading: %s", m_error.c_str());
        return;
    }
    if (!m_uiRuntime.empty()) return;
    std::string error;
    if (!m_uiRuntime.load(*engine->playdh_root(), "NewLoadDlg.bin",
                          engine->ui_resolution_mode(), &error)) {
        m_failed = true;
        m_error = "loading UI unavailable: " + error;
        MLOG_ERROR("CGameLoading: %s", m_error.c_str());
        return;
    }
    m_uiRuntime.activateAllLoadedDialogs();
    MLOG_INFO("CGameLoading: loaded real NewLoadDlg.bin UI");
}

void CGameLoading::Release() {
    m_uiRuntime.clear();
    setInitialized(false);
    MLOG_DEBUG("CGameLoading::Release");
}

void CGameLoading::Process() {
    tick();
    const auto* context = static_cast<const LoadStateContext*>(initParam());
    if (!context) return;
    // Loading has a terminal state: cancellation or the first failure must
    // not be overwritten by a late worker callback.
    if (m_failed || m_cancelled) return;
    if (context->total_steps != 0) {
        m_progress = std::clamp(static_cast<float>(context->completed_steps) /
                                static_cast<float>(context->total_steps), 0.0f, 1.0f);
    }
    m_uiRuntime.setProgressValue(m_progress);
    m_cancelled = context->cancelled;
    if (!m_cancelled && context->failed) {
        m_failed = true;
        m_error = context->error ? context->error : "map loading failed";
    }
}

void CMapChange::Init(void* param) {
    SetInitParam(param);
    m_progress = 0.0f;
    m_failed = false;
    m_cancelled = false;
    m_error.clear();
    const auto* context = static_cast<const LoadStateContext*>(param);
    if (context) {
        if (context->total_steps == 0) {
            m_failed = true;
            m_error = "map change context has zero steps";
        } else {
            m_progress = std::clamp(static_cast<float>(context->completed_steps) /
                                    static_cast<float>(context->total_steps), 0.0f, 1.0f);
        }
        m_cancelled = context->cancelled;
        if (context->failed) {
            m_failed = true;
            m_error = context->error ? context->error : "map change failed";
        }
    }
    setInitialized(true);
    MLOG_INFO("CMapChange::Init progress=%.3f", m_progress);
}

void CMapChange::Release() {
    m_uiRuntime.clear();
    setInitialized(false);
    MLOG_DEBUG("CMapChange::Release");
}

void CMapChange::Start(CEngine* engine) {
    if (m_uiRuntime.empty() && (!engine || !engine->playdh_root())) {
        m_failed = true;
        m_error = !engine ? "map change requires a client engine"
                          : "map change requires an explicit PlayDH resource root";
        MLOG_ERROR("CMapChange: %s", m_error.c_str());
        return;
    }
    if (!m_uiRuntime.empty()) return;
    std::string error;
    if (!m_uiRuntime.load(*engine->playdh_root(), "NewLoadDlg.bin",
                          engine->ui_resolution_mode(), &error)) {
        m_failed = true;
        m_error = "map change UI unavailable: " + error;
        MLOG_ERROR("CMapChange: %s", m_error.c_str());
        return;
    }
    m_uiRuntime.activateAllLoadedDialogs();
    MLOG_INFO("CMapChange: loaded real NewLoadDlg.bin UI");
}

void CMapChange::Process() {
    tick();
    const auto* context = static_cast<const LoadStateContext*>(initParam());
    if (!context) return;
    // MapChange shares the same terminal-state contract as GameLoading:
    // once cancelled or failed, late asynchronous results are ignored.
    if (m_failed || m_cancelled) return;
    if (context->total_steps != 0) {
        m_progress = std::clamp(static_cast<float>(context->completed_steps) /
                                static_cast<float>(context->total_steps), 0.0f, 1.0f);
    } else {
        m_failed = true;
        m_error = "map change context has zero steps";
    }
    m_uiRuntime.setProgressValue(m_progress);
    m_cancelled = context->cancelled;
    if (!m_cancelled && context->failed) {
        m_failed = true;
        m_error = context->error ? context->error : "map change failed";
    }
}
MXH_STATE_STUB_IMPL(CMurimNet)

} // namespace mxh::client
