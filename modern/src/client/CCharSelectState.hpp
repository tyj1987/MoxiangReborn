// mxh/client/CCharSelectState.hpp
// Phase B.2.2 — character-select state (replaces CCharSelect stub).
//
// Wires the eGS_CHARSELECT (GameStateId::CharSelect = 4) slot to the
// modern mxh::net::TcpClient and drives the legacy 4DyuchiNET
// character-list + character-select handshake against MoxianAgentServer:
//
//   client.Connect(agent_addr:port) ->  recv AgentConnectSuccess (proto=8)
//                                     ->  send CharacterListSyn (proto=9)
//                                     ->  recv CharacterListAck (proto=12)
//                                     ->  auto-select first non-empty slot
//                                     ->  send CharacterSelectSyn (proto=16)
//                                     ->  recv CharacterSelectAck (proto=17) or
//                                         CharacterSelectNack (proto=18)
//                                     ->  RequestStateChange(GameLoading)
//
// 1:1 with the legacy MHClient CharSelect flow + agent_handler.cpp
// (handle_legacy_character_list / handle_legacy_character_select).
//
// Modern port notes:
//   * State data is bridged from CLoginState via a LoginResult struct
//     that the host moves between states (CLoginState::TakeLoginResult
//     -> CCharSelectState::SetLoginResult).  No global state, no header
//     cycle.
//   * Wire-format encode/decode (ListSyn payload, ListAck parse,
//     SelectSyn payload) are free functions so unit tests can lock
//     down the binary shape without a TcpClient.
//   * The auto-select-first path matches the legacy default behaviour
//     (the first valid character is preselected; user input can
//     override via SelectCharacter()).

#pragma once

#include "CGameState.hpp"
#include "ClientUiRuntime.hpp"
#include "ClientWire.hpp"
#include "StateTransfer.hpp"

#include <chrono>
#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mxh/net/net.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/render/EntityScene.hpp"

namespace mxh::client {

class CEngine;

enum class CharSelectUiCommandKind : std::uint8_t {
    None,
    SelectSlot,
    Create,
    Delete,
    Enter,
    Logout,
};

struct CharSelectUiCommand {
    CharSelectUiCommandKind kind = CharSelectUiCommandKind::None;
    std::size_t slot_index = 0;
};

CharSelectUiCommand resolve_char_select_ui_command(
    const ClientUiActivation& activation) noexcept;

// Converts the server's complete character slot into the renderer's stable
// appearance payload. The preview scene can place this at any camera anchor
// without inventing appearance defaults or touching protocol data.
std::optional<mxh::gx::ScenePlayer> make_character_preview(
    const CharacterSlot& slot, float world_x = 25600.0f,
    float world_y = 0.0f, float world_z = 25600.0f);

// -------------------------------------------------------------------------
// CCharSelectState — eGS_CHARSELECT state.
// -------------------------------------------------------------------------
class CCharSelectState final : public CGameState,
                               public mxh::net::IConnectionHandler {
public:
    CCharSelectState();
    ~CCharSelectState() override;

    CCharSelectState(const CCharSelectState&)            = delete;
    CCharSelectState& operator=(const CCharSelectState&) = delete;

    // CGameState
    void Init(void* pInitParam) override;
    void Release() override;
    void Process() override;

    // mxh::net::IConnectionHandler
    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
void on_disconnect(mxh::net::ConnectionId id,
                    mxh::net::NetError reason) override;
mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id) override;

    // Host bridge: receive the LoginResult captured by CLoginState.
    // Must be called before Start().
    void SetLoginResult(LoginResult r) { m_login = std::move(r); }

    // Public start hook.  Begins TCP connect to AgentServer.
    // Idempotent (second call is a no-op).
    void Start(CEngine* engine, bool use_hsel = false);

    // ---- Phase 1 §7.3 protocol-burst test hooks ------------------------
    // Direct dispatch (skips network recv thread) so the test can drive
    // the state with synthetic CharacterRemoveAck / ListAck packets and
    // verify §7.3 删除确认 / 取消 and 重登持久化 paths without spinning
    // up a real AgentServer.  Mirrors the CInGameState / CLoginState /
    // CCharMake hook pattern (commits a8e194da / d3804f82 / f8eb9acd).
    // Production code never calls these.
    void SetDispatchForTest(bool enabled) noexcept { m_dispatchEnabledForTest = enabled; }
    void HandleMessageForTest(const mxh::net::Message& msg) {
        if (!m_dispatchEnabledForTest) return;
        on_message({}, msg);
    }

    // ---- Phase 1 §7.2 / §7.3 test hook: arm the CharacterListAck / -----
    // CharacterSelectAck application-level timeouts.  Mirrors the
    // CLoginState login-timeout hook (commit fa74305e) — production code
    // never calls these.
    void SetAckTimeoutForTest(std::chrono::milliseconds t) noexcept {
        m_ackTimeout = t;
    }
    void ArmListAckDeadlineForTest() noexcept {
        m_listAckDeadline = std::chrono::steady_clock::now() + m_ackTimeout;
    }
    void ArmSelectAckDeadlineForTest() noexcept {
        m_selectAckDeadline = std::chrono::steady_clock::now() + m_ackTimeout;
    }

    // Manual selection is the production default. The test-only switch
    // preserves deterministic unattended E2E coverage.
    void SelectCharacter(std::uint32_t chrid);
    bool SelectSlot(std::size_t slot_index) noexcept;
    bool ConfirmSelection();
    bool RequestCharacterCreation();
    bool RequestCharacterDeletion();
    bool RequestLogout();
    bool OnMouseButton(bool left, bool down, std::int32_t x, std::int32_t y);
    bool OnMouseMove(std::int32_t x, std::int32_t y);
    bool OnKeyEvent(bool down, std::uint32_t key);
    bool OnChar(std::uint32_t ch);
    void set_auto_select_for_test(bool enabled) noexcept { m_autoSelectForTest = enabled; }
    bool auto_select_for_test() const noexcept { return m_autoSelectForTest; }

    // Inspectors.
    bool        is_connected() const noexcept;
    bool        is_failed() const noexcept { return m_failed; }
    const std::string& failure_reason() const noexcept { return m_failureReason; }
    std::uint16_t selected_map() const noexcept { return m_selectedMap; }
    std::uint32_t selected_chrid() const noexcept { return m_selectedChrid; }
    bool has_character_list() const noexcept { return m_listReceived; }
    bool deletion_pending() const noexcept { return m_removeSent; }
    bool logout_pending() const noexcept { return m_logoutSent; }
    const LoginResult& login_result() const noexcept { return m_login; }
    // M-R7.1 (2026-08-20): host reads the loaded cDialog tree to render
    // the 1:1 UI (CharSelectDlg.bin — 12 child widgets, 5 character
    // slots, 4 buttons, 3 statics).
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& ui_dialogs() const noexcept {
        return m_uiRuntime.dialogs();
    }
    ClientUiRuntime& ui_runtime() noexcept { return m_uiRuntime; }
    const std::vector<CharacterSlot>& character_list() const noexcept {
        return m_characters;
    }

private:
    void send_list_syn();
    bool send_remove_syn(std::uint32_t character_id);
    void auto_select_first();
    bool select_adjacent(int direction) noexcept;
    bool handle_ui_activation(const ClientUiActivation& activation);
    void apply_character_remove_ack();
    void refresh_character_slot_ui();
    void dispatch_select_ack(std::uint16_t map_num);
    void fail_with(const std::string& reason);

    CEngine*                 m_pEngine    = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    LoginResult              m_login;
    ClientUiRuntime          m_uiRuntime;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;

    std::vector<CharacterSlot> m_characters;   // populated by ListAck
    std::uint32_t            m_selectedChrid = 0;
    std::uint16_t            m_selectedMap   = 0;

    bool                     m_started     = false;
    bool                     m_listReceived = false;
    bool                     m_selectSent   = false;
    bool                     m_removeSent   = false;
    bool                     m_logoutSent   = false;
    bool                     m_listSynSent  = false;
    std::uint32_t            m_removeChrid  = 0;
    bool                     m_autoSelectForTest = false;
    // Phase 1 §7.3: opt-in flag for ccharselect_state_test to call
    // on_message() directly.  When false, HandleMessageForTest is a
    // no-op so a missing test setup cannot accidentally exercise the
    // dispatch path.  Defaults to false.
    bool                     m_dispatchEnabledForTest = false;
    bool                     m_releasing    = false;
    bool                     m_failed      = false;
    std::string              m_failureReason;

    // Phase 1 §7.2 / §7.3: application-level timeouts for the
    // CharacterListAck (after CharacterListSyn) and CharacterSelectAck
    // (after CharacterSelectSyn) round-trips.  Mirrors the CLoginState
    // login-timeout pattern (commit fa74305e).  Default 10 s, overridable
    // by SetAckTimeoutForTest.  steady_clock so a wall-clock adjustment
    // (e.g. NTP) cannot false-fire.
    std::chrono::steady_clock::time_point m_listAckDeadline{};
    std::chrono::steady_clock::time_point m_selectAckDeadline{};
    std::chrono::milliseconds             m_ackTimeout{std::chrono::seconds(10)};
};

} // namespace mxh::client
