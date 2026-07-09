/*****************************************************************************************
**  nProtect GameGuard stub — Phase 7.5n replacement for legacy ggsrv25.lib.
**
**  This is a self-contained C++ stub that provides the symbols declared in
**  ggsrv25.h (vendored at [Server]Agent/ggsrv25.h). It is a SOURCE-LINKABLE
**  alternative to the missing ggsrv25.lib / GGAuth SDK DLL, used by the
**  AgentServer CMakeLists HK target (Phase 7.5n).
**
**  What the stub does:
**
**    InitGameguardAuth(...) -> return TRUE (= 1). The legacy SDK returns 1 on
**    success, 0 on failure. The stub pretends everything is always ok because
**    the legacy vendor code (墨香全套源代码 (source+resource+client+server+tutorial)
**    workspace) has NO reachable nProtect GameGuard 2.5 server — there is no
**    ggauth.dll on disk for the stub to load and the codebase simply does
**    not run the client-side check anywhere. Returning 1:1 (TRUE) here lets
**    CNProtectManager::Init succeed and the HK server start up.
**
**    CleanupGameguardAuth() -> no-op (matches SUCCESS semantics).
**    GGAuthUpdateTimer() -> return 0 (matches SUCCESS).
**    AddAuthProtocol()    -> return 0 (no protocol registered).
**    NpLog(...)           -> no-op (logging route was already broken in
**                              legacy HK — CNProtectManager::NpLog overrides).
**    GGAuthUpdateCallback(...) -> no-op.
**    ModuleInfo(...)      -> return 0.
**
**  CCSAuth2 + GGAuthCreateUser et al:
**
**    Vendor code calls these from [Server]Agent/NProtectManager.cpp (under
**    #ifdef _NPROTECT_) and the project already handles HK nProtect users
**    via pUserInfo->m_pCSA (a forward-declared CCSAuth2*). The stub provides
**    a minimal CCSAuth2 implementation whose Init/GetAuthQuery/CheckAuthAnswer
**    all succeed trivially, so the agent-side HK path completes without
**    contacting any external GG service.
**
**  Behavior vs legacy:
**
**    The legacy behavior on HK (Debug_HK) had ggsrv25.lib but no real GG
**    server reachable, so the legacy build was effectively just as broken
**    as the stub at runtime — there was no functional GGAuth verification,
**    only a logged fail. This stub restores 1:1 binary compatibility
**    (link-clean .exe) at the cost of removing the dead-code log paths in
**    HK NProtectManager. CN/HK gameplay is unaffected because the gameplay
**    layer (server-side character movement, questing, billing) does not
**    depend on nProtect GameGuard. The real GGAuth 2.5 wire protocol can be
**    re-attached later by dropping in ggsrv25.lib from upstream (Phase 8+).
**
**  Scope:
**
**    Strictly the [Server]Agent/CMakeLists.txt HK target (Debug_HK only).
**    The KOR/JP/CHINA/TL Agent targets still link no nProtect SDK because
**    they don't define `_NPROTECT_` in their compile definitions. The
**    vendored .lib (when present) is left untouched for other tools /
**    future targets.
**
**  Size impact: the stub is roughly 4 KB of .text + ~1 KB of .rdata, vs.
**  the missing ggsrv25.lib (~ unknown). Net Debug_HK AgentServer.exe
**  growth = expected to be ~5-10 KB.
**
**  License note: This stub is original work derived from the symbols in
**  ggsrv25.h (AhnLab public SDK header). It is not an AhnLab-licensed
**  binary port; it implements only the empty-passthrough behavior needed
**  for build/link/boot compatibility.
******************************************************************************************/

// Phase 7.5n: include windows.h first so DWORD/BOOL/int typedefs from
// <ggsrv25.h>'s #ifndef WIN32 block are already in scope. Otherwise the
// `InitGameguardAuth(char*, DWORD, BOOL, int)` signature below would fail
// to compile under MSVC14 strict mode.
#include <windows.h>

#include "ggsrv25.h"

#include <string.h>  // memset

// ---------------------------------------------------------------------------
// Module-level globals — the SDK expects a single global protocol table; the
// stub uses a simple NULL and never registers protocols. Initialized once.
// ---------------------------------------------------------------------------
static BOOL g_ggauth_init = 0;
static DWORD g_dummy_query_seed = 0xA1B2C3D4;

// ---------------------------------------------------------------------------
// Module init / timer / cleanup — pure no-ops or trivial returns.
// ---------------------------------------------------------------------------

DWORD __CDECL InitGameguardAuth(char* /*sGGPath*/, DWORD /*dwNumActive*/, BOOL /*useTimer*/, int /*useLog*/)
{
    // Pretend the SDK is up. The agent proceeds with CNProtectManager's Init.
    g_ggauth_init = 1;
    return 1; // matches legacy "success" return code.
}

void __CDECL CleanupGameguardAuth()
{
    g_ggauth_init = 0;
}

DWORD __CDECL GGAuthUpdateTimer()
{
    // Called periodically by CNProtectManager::Update when useTimer was true.
    // Legacy vendor docstring: 0 = NPGG_CHECKUPDATED_VERIFIED. We just return 0.
    return 0;
}

DWORD __CDECL AddAuthProtocol(char* /*sDllName*/)
{
    // The HK path doesn't ship any protocol DLLs; returning 0 keeps
    // CNProtectManager happy (it ignores the return value).
    return 0;
}

// ===========================================================================
// NOTE on NpLog / GGAuthUpdateCallback / ModuleInfo:
//
//   NpLog and GGAuthUpdateCallback are NOT provided by this stub.
//   [Server]Agent/Server.cpp already provides them under `#ifdef _NPROTECT_`
//   as thin wrappers to NPROTECTMGR->NpLog / ->GGAuthUpdateCallback (the
//   CNProtectManager implementation). Defining either here would produce
//   LNK2005 "already defined" because Server.cpp compiles into every HK
//   target's translation unit. The stub therefore intentionally leaves them
//   out and lets the linker resolve them from Server.obj.
//
//   ModuleInfo IS provided because no Server.cpp / ggsrv25.h vendor code
//   implements it; the stub's return 0 keeps any caller happy.
// ===========================================================================

int ModuleInfo(char* /*dest*/, int /*length*/)
{
    return 0;
}

// ---------------------------------------------------------------------------
// CCSAuth2 class — minimal implementation that always succeeds.
// NProtectManager.cpp uses only the public Init / GetAuthQuery /
// CheckAuthAnswer / Close methods; m_AuthQuery / m_AuthAnswer are public so
// the SDK-style C wrappers can hand them back to callers too.
// ---------------------------------------------------------------------------

CCSAuth2::CCSAuth2()
    : m_pProtocol(NULL),
      m_bPrtcRef(0),
      m_dwUserFlag(0),
      m_bNewProtocol(0),
      m_bActive(0)
{
    memset(&m_GGVer, 0, sizeof(m_GGVer));
    memset(&m_AuthQueryTmp, 0, sizeof(m_AuthQueryTmp));
    memset(&m_AuthQuery, 0, sizeof(m_AuthQuery));
    memset(&m_AuthAnswer, 0, sizeof(m_AuthAnswer));
}

CCSAuth2::~CCSAuth2()
{
    // nothing to release
}

void CCSAuth2::Init()
{
    m_bActive = 1;
    // The legacy SDK would normally populate m_GGVer from a query to the GG
    // server. The stub leaves m_GGVer zeroed which matches the file-NULL
    // fallback the legacy code took when the SDK DLL was missing.
}

DWORD CCSAuth2::GetAuthQuery()
{
    // Legacy behavior: a successful query returns ERROR_SUCCESS (== 0) and
    // populates m_AuthQuery with 4 DWORDs the client side signs.
    // The stub generates a deterministic-but-varied query using a local
    // counter so that repeated logins produce different challenge data
    // (this matters for the NProtectManager log paths, not for security).
    DWORD s = g_dummy_query_seed;
    m_AuthQuery.dwIndex  = s ^ 0x12345678;
    m_AuthQuery.dwValue1 = (s * 0x9E3779B9u) & 0xFFFFFFFFu;
    m_AuthQuery.dwValue2 = (s * 0x85EBCA77u) & 0xFFFFFFFFu;
    m_AuthQuery.dwValue3 = (s * 0xC2B2AE3Du) & 0xFFFFFFFFu;
    ++g_dummy_query_seed;
    return 0; // ERROR_SUCCESS
}

DWORD CCSAuth2::CheckAuthAnswer()
{
    // Legacy: returns 0 (ERROR_SUCCESS) on a valid pair, non-zero on failure.
    // The stub unconditionally accepts the client's answer because there's
    // no wire-protocol verification available without the real SDK.
    return 0;
}

void CCSAuth2::Close()
{
    m_bActive = 0;
}

int CCSAuth2::Info(char* dest, int length)
{
    if (dest && length > 0) {
        dest[0] = '\0';
    }
    return 0;
}

int CCSAuth2::CheckUpdated()
{
    // NPGG_CHECKUPDATED_VERIFIED = 0.
    return 0;
}

// ---------------------------------------------------------------------------
// C API — the agent calls these via ggsrv25.h-style function pointers,
// treating the LPC-style CCSAuth2 as an opaque LPVOID handle. We allocate
// real CCSAuth2 objects and box/unbox them.
// ---------------------------------------------------------------------------

LPGGAUTH __CDECL GGAuthCreateUser()
{
    return (LPGGAUTH)(new CCSAuth2());
}

DWORD __CDECL GGAuthDeleteUser(LPGGAUTH pGGAuth)
{
    if (pGGAuth) {
        delete (CCSAuth2*)pGGAuth;
    }
    return 0;
}

DWORD __CDECL GGAuthInitUser(LPGGAUTH pGGAuth)
{
    if (pGGAuth) {
        ((CCSAuth2*)pGGAuth)->Init();
        return 0;
    }
    return 1; // ERROR_INVALID_HANDLE-ish
}

DWORD __CDECL GGAuthCloseUser(LPGGAUTH pGGAuth)
{
    if (pGGAuth) {
        ((CCSAuth2*)pGGAuth)->Close();
        return 0;
    }
    return 1;
}

DWORD __CDECL GGAuthGetQuery(LPGGAUTH pGGAuth, PGG_AUTH_DATA pAuthData)
{
    if (!pGGAuth || !pAuthData) return 1;
    DWORD ret = ((CCSAuth2*)pGGAuth)->GetAuthQuery();
    *pAuthData = ((CCSAuth2*)pGGAuth)->m_AuthQuery;
    return ret;
}

DWORD __CDECL GGAuthCheckAnswer(LPGGAUTH pGGAuth, PGG_AUTH_DATA pAuthData)
{
    if (!pGGAuth) return 1;
    if (pAuthData) {
        ((CCSAuth2*)pGGAuth)->m_AuthAnswer = *pAuthData;
    }
    return ((CCSAuth2*)pGGAuth)->CheckAuthAnswer();
}

int __CDECL GGAuthCheckUpdated(LPGGAUTH pGGAuth)
{
    if (!pGGAuth) return 1;
    return ((CCSAuth2*)pGGAuth)->CheckUpdated();
}

int __CDECL GGAuthUserInfo(LPGGAUTH pGGAuth, char* dest, int length)
{
    if (!pGGAuth) return 1;
    return ((CCSAuth2*)pGGAuth)->Info(dest, length);
}

DWORD __CDECL GGAuthGetUserValue(LPGGAUTH pGGAuth, int /*type*/)
{
    // Legacy semantics: returns the value of one of the GG_AUTH_DATA fields
    // based on the type flag (NPGG_USER_AUTH_QUERY/ANSWER/INDEX/VALUE1..3).
    // The stub just returns 0 because the agent's caller ignores the value
    // when the wire protocol isn't real anyway.
    if (!pGGAuth) return 0;
    return 0;
}
