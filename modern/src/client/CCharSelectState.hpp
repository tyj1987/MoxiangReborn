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
#include "StateTransfer.hpp"

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

// One slot in the legacy CharacterListAck SEND_CHARSELECT_INFO.
// We retain the character id and original 17-byte name used by the slot UI.
struct CharacterSlot {
    std::uint32_t chrid = 0;
    std::string   name;
    bool          valid = false;     // false = empty slot (chrid == 0)
    std::uint8_t gender = 0;
    std::uint8_t face_type = 0;
    std::uint8_t hair_type = 0;
    std::uint16_t level = 0;
    std::uint16_t map_num = 0;
    std::array<std::uint16_t, 10> weared_item_idx{};
};

// Converts the server's complete character slot into the renderer's stable
// appearance payload. The preview scene can place this at any camera anchor
// without inventing appearance defaults or touching protocol data.
std::optional<mxh::gx::ScenePlayer> make_character_preview(
    const CharacterSlot& slot, float world_x = 25600.0f,
    float world_y = 0.0f, float world_z = 25600.0f);

// Selection is authoritative to the most recently received list.  Keeping
// this check separate makes the rule testable without opening a socket.
bool is_listed_character(std::span<const CharacterSlot> slots,
                         std::uint32_t chrid) noexcept;

// Build the 8-byte CharacterListSyn payload (agent_handler.cpp:526-538):
//   [user_id: u32 LE] [dist_auth_key: u32 LE]
std::vector<std::uint8_t>
legacy_character_list_syn_payload(std::uint32_t user_id,
                                  std::uint32_t dist_auth_key);

// Build the minimal CharacterSelectSyn payload (proto=16):
//   [channel: u16 LE]  (the chrid is in MSGBASE.object_id, not payload)
std::vector<std::uint8_t>
legacy_character_select_syn_payload(std::uint16_t channel);

// Build the 4-byte CharacterRemoveSyn payload (legacy MSG_DWORD):
//   [character_id: u32 LE]
std::vector<std::uint8_t>
legacy_character_remove_syn_payload(std::uint32_t character_id);

// Build the empty-payload DisconnectSyn packet used to leave Agent cleanly.
mxh::net::Message legacy_character_disconnect_syn_message();

// Parse the 889-byte legacy CharacterListAck payload (no _CRYPTCHECK_,
// CHINA locale, kMaxCharSlots=5).  Returns the first 5 slots; valid
// flag is true for slots 0..char_count-1.  Returns std::nullopt if the
// payload is shorter than 4 bytes (CharNum header) or the slot fields
// would read past the end.
std::optional<std::vector<CharacterSlot>>
parse_legacy_character_list_ack(std::span<const std::uint8_t> payload);

// Parse the 1-byte CharacterSelectAck payload (map number).
std::optional<std::uint16_t>
parse_legacy_character_select_ack(std::span<const std::uint8_t> payload);

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
    bool                     m_releasing    = false;
    bool                     m_failed      = false;
    std::string              m_failureReason;
};

} // namespace mxh::client
