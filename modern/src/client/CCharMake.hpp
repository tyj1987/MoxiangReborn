// mxh/client/CCharMake.hpp
// Phase B.4 - character-creation state (replaces CCharMake stub).
//
// Wires the eGS_CHARMAKE (GameStateId::CharMake = 5) slot to the modern
// mxh::net::TcpClient and drives the legacy 4DyuchiNET character-creation
// handshake against MoxianAgentServer:
//
//   client.Connect(agent_addr:port) -> recv AgentConnectSuccess (proto=8)
//                                    ->  [host fills the creation form]
//                                    ->  send CharacterMakeSyn (proto=22,
//                                        CHARACTERMAKEINFO 59B payload)
//                                    ->  recv CharacterListAck (proto=12)
//                                        = success (server re-sends the
//                                        refreshed list) -> CharSelect
//                                    ->  recv CharacterMakeNack (proto=24)
//                                        = failure (stay, host may retry)
//
// 1:1 with the legacy MHClient CharMake.cpp + GlobalEventFunc.cpp flow
// (CM_CharMakeBtnFunc) and agent_handler.cpp
// (handle_legacy_character_make):
//   * The legacy client memcpy's its CHARACTERMAKEINFO, sets
//     Category/Protocol/StandingArrayNum=-1 and the name, then sends
//     sizeof(msg) (59 bytes after MSGBASE).  The agent ignores
//     bDuplCheck/WearedItemIdx and overwrites UserID server-side.
//   * On success the legacy client receives the refreshed
//     MP_USERCONN_CHARACTERLIST_ACK and switches to eGAMESTATE_CHARSELECT;
//     on MP_USERCONN_CHARACTER_MAKE_NACK it stays in the create state.
//   * CharacterMakeAck (proto=23) is emitted by the modern agent as a
//     no-op marker; the state transition is driven by ListAck exactly
//     like the legacy client.
//
// Modern port notes (same conventions as CCharSelectState):
//   * The agent address/port + auth keys are bridged via LoginResult
//     (defined in CCharSelectState.hpp).
//   * The 59-byte encode is a free function (legacy_character_make_syn_payload)
//     so unit tests can lock the binary shape without a TcpClient.
//   * The state owns its TcpClient (unique_ptr) and disconnects in
//     Release(), matching the legacy per-state network ownership.

#pragma once

#include "CGameState.hpp"
#include "CCharSelectState.hpp"  // LoginResult

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mxh/net/net.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"

namespace mxh::client {

class CEngine;

// Character-creation options the host collects from the creation UI.
// 1:1 field set with the legacy CHARACTERMAKEINFO (CommonStruct.h:734)
// minus the fields the server overwrites / ignores.
struct CharacterMakeParams {
    std::string name;       // truncated to MAX_NAME_LENGTH (16) on the wire
    std::uint8_t sex_type   = 0;
    std::uint8_t body_type  = 0;
    std::uint8_t hair_type  = 0;
    std::uint8_t face_type  = 0;
    std::uint8_t start_area = 0;    // user-selected in the legacy client
    float        height     = 1.0f;
    float        width      = 1.0f;
};

// Build the 59-byte legacy CHARACTERMAKEINFO payload (after MSGBASE) that
// agent_handler.cpp::handle_legacy_character_make parses:
//   [0..17)  Name[17] (16 chars + NUL, truncated, zero-padded)
//   [17..21) UserID (u32 LE) - filled from LoginResult.user_idx
//   [21]     SexType (u8)
//   [22]     BodyType (u8)
//   [23]     HairType (u8)
//   [24]     FaceType (u8)
//   [25]     StartArea (u8)
//   [26..30) bDuplCheck (u32 LE) - 0 (legacy client sends FALSE)
//   [30..50) WearedItemIdx[10] (10 * u16 LE) - all 0 (no starter items)
//   [50]     StandingArrayNum (u8) - legacy client sends -1 = 0xFF
//   [51..55) Height (f32 LE)
//   [55..59) Width (f32 LE)
std::vector<std::uint8_t>
legacy_character_make_syn_payload(const CharacterMakeParams& params,
                                  std::uint32_t user_id);

// -------------------------------------------------------------------------
// CCharMake - eGS_CHARMAKE state.
// -------------------------------------------------------------------------
class CCharMake final : public CGameState,
                        public mxh::net::IConnectionHandler {
public:
    CCharMake();
    ~CCharMake() override;

    CCharMake(const CCharMake&)            = delete;
    CCharMake& operator=(const CCharMake&) = delete;

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

    // Submit the creation form: validates + builds the 59-byte
    // CHARACTERMAKEINFO and sends CharacterMakeSyn.  Safe to call once
    // the agent connection is up; returns false if not connected or an
    // identical submit is already in flight.
    bool SubmitCharacter(const CharacterMakeParams& params);

    // Inspectors.
    bool        is_connected() const noexcept;
    bool        is_submitted() const noexcept { return m_makeSent; }
    bool        is_failed() const noexcept { return m_failed; }
    const std::string& failure_reason() const noexcept { return m_failureReason; }

private:
    void send_make_syn();
    void fail_with(const std::string& reason);

    CEngine*                 m_pEngine = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    LoginResult              m_login;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;

    CharacterMakeParams      m_pending;     // captured by SubmitCharacter
    bool                     m_started  = false;
    bool                     m_connectAcked = false;
    bool                     m_makeSent = false;
    bool                     m_failed   = false;
    std::string              m_failureReason;
};

} // namespace mxh::client
