// mxh/client/CMainTitle.hpp
// Phase A.1.8 — 1:1 port of legacy CMainTitle (墨香【源码】\[Client]MH\MainTitle.h).
//
// CMainTitle owns the boot → login → server-select → agent-connect
// flow.  The legacy CMainTitle is a 1860-line file with ~50 members;
// A.1.8 ships the full 1:1 surface as a header (so future A.1.8.b+
// tasks can fill the bodies in without changing the public API),
// and a minimal Init / Release / Process implementation that:
//   * Reads MHVerInfo.ver (client version string — see legacy
//     MHClient.cpp SetGameVersion()).
//   * Skips the logo window, server list dialog, intro replay, and
//     Distribute TCP connect (those land in A.1.8.b / B.1 once the
//     resource / network layers are wired in).
//
// 1:1 quirks preserved:
//   * 9 field surface matching legacy MainTitle.h (m_pCamera,
//     m_pServerListDlg, m_pIntroReplayDlg, m_bDisconntinToDist,
//     m_DistAuthKey, m_UserIdx, m_pLogoWindow, m_dwStartTime,
//     m_bInit, m_bServerList, m_bWaitConnectToAgent, m_bNoDiconMsg,
//     m_dwDiconWaitTime, m_dwWaitTime, m_ConnectionServerNo).
//   * State machine flags (m_bInit, m_bServerList, m_bDisconntinToDist,
//     m_bWaitConnectToAgent) drive the same Process() branches as
//     the legacy engine.
//   * m_ConnectionServerNo is initialised to 0; the legacy engine
//     treated 0 as "no server selected yet" (the server list dialog
//     sets it on click).
#pragma once

#include "CGameState.hpp"
#include "ClientUiRuntime.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::ui {
class cDialog;
class cEditBox;
}

namespace mxh::client {

class CEngine;

enum class LoginUiCommandKind : std::uint8_t {
    None,
    Submit,
    Exit,
};

struct LoginUiCommand {
    LoginUiCommandKind kind = LoginUiCommandKind::None;
};

// 1:1 with recovered GlobalEventFunc.cpp:
//   MT_OKBTN / MT_LogInOkBtnFunc -> login
//   MT_ENDBTN / MT_ExitBtnFunc   -> clear fields (server list later)
LoginUiCommand resolve_login_ui_command(
    const ClientUiActivation& activation) noexcept;

// Forward-declared classes (legacy MainTitle.h's dependencies) are
// kept as opaque `void*` for now.  This matches the Phase 6.4
// cWindow::m_basicImage pattern: the legacy class has the same
// pointer, the modern port holds it as void* to avoid pulling the
// full include chain into every translation unit.  A.1.8.b swaps
// them for typed pointers once the corresponding modern classes
// land.

// 1:1 surface of legacy CMainTitle.  Each member's purpose is
// documented inline.
class CMainTitle : public CGameState {
public:
    CMainTitle();
    ~CMainTitle() override;

    CMainTitle(const CMainTitle&) = delete;
    CMainTitle& operator=(const CMainTitle&) = delete;

    void Init(void* pInitParam) override;
    void Release() override;
    void Process() override;

    // -------------------------------------------------------------------------
    // Legacy 1:1 accessors used by the rest of the engine.  These are
    // intentionally present (even though the A.1.8 stub bodies don't
    // do much) so future A.1.8.b tasks can fill the bodies in
    // without changing the public API.
    // -------------------------------------------------------------------------
    std::uint32_t GetDistAuthKey() const noexcept          { return m_DistAuthKey; }
    std::uint32_t GetUserIdx() const noexcept             { return m_UserIdx; }

    void OnLoginError(std::uint32_t errorcode, std::uint32_t dwParam);
    void OnDisconnect();

    // Load IDDlg.bin (MT_LOGINDLG) and seed the ID/password edit boxes.
    // Idempotent for the dialog tree; credentials are applied every call.
    void Start(CEngine* engine,
               std::string username = {},
               std::string password = {});

    bool OnMouseButton(bool left, bool down, std::int32_t x, std::int32_t y);
    bool OnMouseMove(std::int32_t x, std::int32_t y);
    bool OnKeyEvent(bool down, std::uint32_t key);
    bool OnChar(std::uint32_t ch);

    bool trySubmit();
    void clearFields();
    bool consumeSubmit() noexcept;
    const std::string& username() const noexcept { return m_username; }
    const std::string& password() const noexcept { return m_password; }

    ClientUiRuntime& ui_runtime() noexcept { return m_uiRuntime; }
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& ui_dialogs() const noexcept {
        return m_uiRuntime.dialogs();
    }

    // 1:1 quirk: m_DistAuthKey and m_UserIdx are populated by the
    // Distribute server's response to MP_USERCONN.  A.1.8 leaves them
    // at 0 (no Distribute connect yet).  When the network layer lands
    // in B.1, the response handler will write to them directly.
    void SetDistAuthKey(std::uint32_t v) noexcept          { m_DistAuthKey = v; }
    void SetUserIdx(std::uint32_t v) noexcept             { m_UserIdx = v; }

    // Server list dialog accessor.  Used by the menu code to check
    // whether the server list is currently visible.  Returns the
    // opaque pointer; A.1.8.b will type it as CServerListDialog*.
    void* GetServerListDialog() const noexcept { return m_pServerListDlg; }

    // Distributed state-machine flags.  Exposed read-only so the
    // diagnostic harness can observe the boot flow.
    bool isServerList() const noexcept                    { return m_bServerList; }
    bool isInit() const noexcept                          { return m_bInit; }
    bool isWaitingForAgent() const noexcept               { return m_bWaitConnectToAgent; }

private:
    // 1:1 surface — see legacy MainTitle.h for the original
    // declarations.  Names match the legacy spelling so a search-
    // and-replace across the legacy source translates 1:1.
    // Pointer fields are opaque `void*` until the corresponding
    // modern classes (CEngineCamera, cDialog, CServerListDialog,
    // CIntroReplayDlg) land in A.1.8.b / Phase B.
    void*                 m_pCamera                 = nullptr;
    bool                  m_bDisconntinToDist       = false;
    std::uint32_t         m_DistAuthKey             = 0;
    std::uint32_t         m_UserIdx                 = 0;
    void*                 m_pLogoWindow             = nullptr;
    std::uint32_t         m_dwStartTime             = 0;
    bool                  m_bInit                   = false;
    void*                 m_pAdvice                 = nullptr;  // TAIWAN_LOCAL
    void*                 m_pServerListDlg          = nullptr;
    char                  m_DistributeAddr[16]      = {0};
    std::uint16_t         m_DistributePort          = 0;
    bool                  m_bServerList             = false;
    std::uint32_t         m_dwDiconWaitTime         = 0;
    std::uint32_t         m_dwWaitTime              = 0;
    bool                  m_bWaitConnectToAgent     = false;
    bool                  m_bNoDiconMsg             = false;
    std::uint32_t         m_ConnectionServerNo      = 0;
    void*                 m_pIntroReplayDlg         = nullptr;

    // Modern port addition: the MHVerInfo.ver version string is
    // parsed in Init() and stashed here so a future protocol layer
    // (B.1) can include it in MP_USERCONN_REQUEST_LOGIN.  Matches
    // the legacy MHClient.cpp g_CLIENTVERSION[] global.
    char                  m_ClientVersion[32]       = {0};

    void bind_login_edits();
    void sync_credentials_from_edits();
    bool handle_ui_activation(const ClientUiActivation& activation);
    bool handle_edit_return();
    mxh::ui::cEditBox* id_edit() const;
    mxh::ui::cEditBox* password_edit() const;

    ClientUiRuntime m_uiRuntime;
    std::string     m_username;
    std::string     m_password;
    bool            m_submitRequested = false;
};

} // namespace mxh::client
