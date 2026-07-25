// mxh/client/CMainTitle.cpp
// Phase A.1.8 — minimal CMainTitle implementation.  Reads MHVerInfo.ver
// (client version string) and stubs out the rest of the boot flow
// (logo window, server list, Distribute connect, agent connect).
//
// The legacy CMainTitle is a 1860-line file with 50+ members — a
// full 1:1 port of all the body code (chat manager init, item
// manager init, resource manager init, mouse/keyboard init, the
// camera, ...) is multi-day work and is split across A.1.8.b+.
// A.1.8 ships:
//   * Full 1:1 surface (m_pCamera, m_pServerListDlg, m_pLogoWindow,
//     m_bInit, m_bServerList, m_DistAuthKey, m_UserIdx, m_dwStartTime,
//     ... all 14 fields from MainTitle.h).
//   * Init reads MHVerInfo.ver.
//   * Process / Release do the same bookkeeping the legacy engine
//     did, but the heavy managers (ChatManager, ItemManager, ...)
//     are stubbed out — those are in CGameIn, not CMainTitle.

#include "CMainTitle.hpp"

#include "mxh/log/mlog.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace mxh::client {

namespace {

// 1:1 quirk: the legacy MHClient.cpp reads MHVerInfo.ver as a plain
// text file whose first line is the client version string (e.g.
// "NDSC08070301" for the Korean server).  The version is later
// included in MP_USERCONN_REQUEST_LOGIN.  We preserve the same
// format on the modern side; A.1.8 stores the string but the
// network layer (B.1) is what actually sends it.
constexpr const char* kMHVerInfoPath = "MHVerInfo.ver";

void readMHVerInfo(char out[32]) {
    std::ifstream in(kMHVerInfoPath);
    if (!in.is_open()) {
        // Fallback: the file may live next to the executable.  Try a
        // few common locations.
        for (const char* p : {"Resource/MHVerInfo.ver",
                              "Client/MHVerInfo.ver",
                              "MHVerInfo.bin"}) {
            std::ifstream in2(p);
            if (in2.is_open()) {
                in = std::move(in2);
                break;
            }
        }
    }
    if (!in.is_open()) {
        MLOG_WARN("CMainTitle: MHVerInfo.ver not found, using default version");
        std::strncpy(out, "MXRBN99999999", 32 - 1);
        out[32 - 1] = '\0';
        return;
    }
    std::string line;
    std::getline(in, line);
    if (line.empty()) {
        std::strncpy(out, "MXRBN99999999", 32 - 1);
    } else {
        std::strncpy(out, line.c_str(), 32 - 1);
    }
    out[32 - 1] = '\0';
    MLOG_INFO("CMainTitle: client version = %s", out);
}

} // namespace

CMainTitle::CMainTitle() = default;
CMainTitle::~CMainTitle() = default;

void CMainTitle::Init(void* /*pInitParam*/) {
    MLOG_INFO("CMainTitle::Init — booting into the login flow");
    readMHVerInfo(m_ClientVersion);

    // The legacy engine started the logo window + camera + intro
    // replay here.  A.1.8 just records the start time and marks
    // m_bInit so Process() can drive the state machine.
    m_dwStartTime = ::GetTickCount();
    m_bInit       = true;
    m_bServerList = false;

    setInitialized(true);
}

void CMainTitle::Release() {
    MLOG_INFO("CMainTitle::Release");
    m_pCamera         = nullptr;
    m_pLogoWindow     = nullptr;
    m_pAdvice         = nullptr;
    m_pServerListDlg  = nullptr;
    m_pIntroReplayDlg = nullptr;
    m_bInit           = false;
    setInitialized(false);
}

void CMainTitle::Process() {
    // The legacy CMainTitle::Process drives several timers:
    //   * m_dwDiconWaitTime — Distribute disconnect wait (60s).
    //   * m_dwWaitTime      — Agent connect wait.
    //   * m_dwStartTime     — logo window fade-in.
    // A.1.8 doesn't yet have a real Distribute / Agent connect so
    // these timers stay at 0.  The Process() stub is here so the
    // A.1.8.b network-layer task can fill in the timer logic
    // without changing the surrounding code.
    if (!m_bInit) return;
    // No-op in A.1.8.  A.1.8.b will add: m_dwDiconWaitTime check +
    // server list show + agent connect handshake.
}

void CMainTitle::OnLoginError(std::uint32_t errorcode, std::uint32_t /*dwParam*/) {
    // 1:1 quirk: the legacy engine pops a cMsgBox with the error
    // string keyed by errorcode (see Protocol.h's eLoginError
    // enum).  A.1.8 just logs the error; the msgbox integration
    // lands in A.1.8.b once cMsgBox is wired into the dialog tree.
    MLOG_WARN("CMainTitle::OnLoginError code=%u", errorcode);
}

void CMainTitle::OnDisconnect() {
    MLOG_INFO("CMainTitle::OnDisconnect");
    m_bDisconntinToDist   = true;
    m_dwDiconWaitTime     = ::GetTickCount();
    m_bWaitConnectToAgent = false;
    m_bServerList         = false;
}

} // namespace mxh::client
