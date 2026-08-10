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

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mxh/net/net.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"

namespace mxh::client {

class CEngine;

// 1:1 with the legacy LoginAck payload (login_handler.cpp make_login_ack).
// Bridged from CLoginState::TakeLoginResult() to CCharSelectState via
// SetLoginResult() so the agent connect address/port + auth keys flow
// through state boundaries without globals.
struct LoginResult {
    std::string  agent_addr;
    std::uint16_t agent_port = 0;
    std::uint32_t user_idx   = 0;
    std::uint8_t  user_level = 0;
    std::uint32_t dist_auth_key = 0;  // from DistConnectSuccess, needed by Agent's ListSyn
};

struct GameEntryRequest {
    std::uint32_t character_id = 0;
    std::uint16_t map_num = 0;
};

// One slot in the legacy CharacterListAck SEND_CHARSELECT_INFO.
// We parse the minimum needed to auto-select: chrid (u32) per slot.
struct CharacterSlot {
    std::uint32_t chrid = 0;
    bool          valid = false;     // false = empty slot (chrid == 0)
};

// Build the 8-byte CharacterListSyn payload (agent_handler.cpp:526-538):
//   [user_id: u32 LE] [dist_auth_key: u32 LE]
std::vector<std::uint8_t>
legacy_character_list_syn_payload(std::uint32_t user_id,
                                  std::uint32_t dist_auth_key);

// Build the minimal CharacterSelectSyn payload (proto=16):
//   [channel: u16 LE]  (the chrid is in MSGBASE.object_id, not payload)
std::vector<std::uint8_t>
legacy_character_select_syn_payload(std::uint16_t channel);

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

    // Override auto-select.  Default behaviour after ListAck is to send
    // CharacterSelectSyn for the first valid slot; the host can call
    // this from UI handlers to pick a different one.
    void SelectCharacter(std::uint32_t chrid);

    // Inspectors.
    bool        is_connected() const noexcept;
    std::uint16_t selected_map() const noexcept { return m_selectedMap; }
    std::uint32_t selected_chrid() const noexcept { return m_selectedChrid; }
    bool has_character_list() const noexcept { return m_listReceived; }
    const LoginResult& login_result() const noexcept { return m_login; }
    const std::vector<CharacterSlot>& character_list() const noexcept {
        return m_characters;
    }

private:
    void send_list_syn();
    void auto_select_first();
    void dispatch_select_ack(std::uint16_t map_num);
    void fail_with(const std::string& reason);

    CEngine*                 m_pEngine    = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    LoginResult              m_login;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;

    std::vector<CharacterSlot> m_characters;   // populated by ListAck
    std::uint32_t            m_selectedChrid = 0;
    std::uint16_t            m_selectedMap   = 0;

    bool                     m_started     = false;
    bool                     m_listReceived = false;
    bool                     m_selectSent   = false;
    bool                     m_releasing    = false;
    bool                     m_failed      = false;
    std::string              m_failureReason;
};

} // namespace mxh::client
