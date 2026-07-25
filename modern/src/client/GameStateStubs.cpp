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

#include "mxh/log/mlog.hpp"

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
MXH_STATE_STUB_IMPL(CCharMake)
MXH_STATE_STUB_IMPL(CGameLoading)
MXH_STATE_STUB_IMPL(CGameIn)
MXH_STATE_STUB_IMPL(CMapChange)
MXH_STATE_STUB_IMPL(CMurimNet)

} // namespace mxh::client
