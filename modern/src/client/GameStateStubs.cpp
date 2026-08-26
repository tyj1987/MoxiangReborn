// mxh/client/GameStateStubs.cpp
// Phase A.1.7 — implementation of the 9 eGAMESTATE concrete state
// stubs.  Each method is intentionally a no-op for A.1.7; the real
// bodies land in A.1.8+ as the corresponding legacy state is ported
// (CMainTitle first, since the boot-to-login flow is the next thing
// the user sees after the bootscreen).
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
    if (!engine || !engine->playdh_root() || !m_uiRuntime.empty()) return;
    std::string error;
    if (!m_uiRuntime.load(*engine->playdh_root(), "NewLoadDlg.bin",
                          engine->ui_resolution_mode(), &error)) {
        MLOG_WARN("CGameLoading: NewLoadDlg.bin unavailable: %s", error.c_str());
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
    if (context->total_steps != 0) {
        m_progress = std::clamp(static_cast<float>(context->completed_steps) /
                                static_cast<float>(context->total_steps), 0.0f, 1.0f);
    }
    m_uiRuntime.setProgressValue(m_progress);
    m_cancelled = context->cancelled;
    if (context->failed) {
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
    if (!engine || !engine->playdh_root() || !m_uiRuntime.empty()) return;
    std::string error;
    if (!m_uiRuntime.load(*engine->playdh_root(), "NewLoadDlg.bin",
                          engine->ui_resolution_mode(), &error)) {
        MLOG_WARN("CMapChange: NewLoadDlg.bin unavailable: %s", error.c_str());
        return;
    }
    m_uiRuntime.activateAllLoadedDialogs();
    MLOG_INFO("CMapChange: loaded real NewLoadDlg.bin UI");
}

void CMapChange::Process() {
    tick();
    const auto* context = static_cast<const LoadStateContext*>(initParam());
    if (!context) return;
    if (context->total_steps != 0) {
        m_progress = std::clamp(static_cast<float>(context->completed_steps) /
                                static_cast<float>(context->total_steps), 0.0f, 1.0f);
    } else {
        m_failed = true;
        m_error = "map change context has zero steps";
    }
    m_uiRuntime.setProgressValue(m_progress);
    m_cancelled = context->cancelled;
    if (context->failed) {
        m_failed = true;
        m_error = context->error ? context->error : "map change failed";
    }
}
MXH_STATE_STUB_IMPL(CMurimNet)

} // namespace mxh::client
