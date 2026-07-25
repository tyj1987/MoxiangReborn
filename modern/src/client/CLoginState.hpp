// mxh/client/CLoginState.hpp
// Phase B.2.1 — login state (replaces CConnecting stub).
//
// Wires the eGS_CONNECT (GameStateId::Connect = 2) slot to the modern
// mxh::net::TcpClient and drives the legacy 4DyuchiNET login handshake
// against MoxianLoginServer:
//
//   client.Connect()            ->  recv DistConnectSuccess (auth_key)
//                                  ->  send RequestLogin (legacy 38B payload)
//                                  ->  recv LoginAck (23B legacy payload)
//                                  ->  RequestStateChange(CharSelect)
//
// 1:1 with the legacy MHClient.cpp connecting-screen flow; the wire
// format and protocol numbers (cat=7, proto=0/1/2) are byte-for-byte
// with the LoginHandler in modern/src/server/login_handler.cpp.
//
// Modern port notes:
//   * State-switch hook goes through m_pEngine->RequestStateChange()
//     so this class never has to know about CMainGame directly (avoids
//     a header cycle).  CMainGame::Init() injects the callback into
//     CEngine in the same edit that wires CEngine to the renderer.
//   * The TcpClient instance is owned by the state (unique_ptr) so
//     Release() can drop the socket deterministically before the
//     next state's Init() runs.  Matches the legacy MHClient behaviour
//     of disconnecting on state transitions.
//   * The actual wire-format encode/decode lives in two free functions
//     (legacy_request_login_payload / parse_legacy_login_ack) so unit
//     tests can lock down the binary shape without spinning up a
//     TcpClient.

#pragma once

#include "CGameState.hpp"

#include <cstdint>
#include <memory>
#include <string>

#include "mxh/net/net.hpp"

namespace mxh::client {

class CEngine;
class CMainGame;

// 1:1 with the legacy LoginHandler::make_login_ack() payload layout.
struct LegacyLoginAck {
    std::string  agent_addr;     // null-padded up to 16 bytes
    std::uint16_t agent_port = 0; // little-endian on the wire
    std::uint32_t user_idx   = 0; // little-endian
    std::uint8_t  user_level = 0;
};

// Build the 38-byte RequestLogin payload that the modern LoginServer
// expects in legacy mode (login_handler.cpp::handle_legacy_login):
//   [AuthKey: u32 LE] [id: char[17]] [pw: char[17]]
// id and pw are UTF-8, truncated at 17 bytes, null-padded.
std::vector<std::uint8_t>
legacy_request_login_payload(std::uint32_t auth_key,
                             const std::string& user_id,
                             const std::string& password);

// Parse the 23-byte legacy LoginAck payload (login_handler.cpp lines
// 339-354). Returns std::nullopt if the payload is too short or
// contains invalid UTF-8 in the agent address.
std::optional<LegacyLoginAck>
parse_legacy_login_ack(std::span<const std::uint8_t> payload);

// -------------------------------------------------------------------------
// CLoginState — eGS_CONNECT state, drives the LoginServer handshake.
// -------------------------------------------------------------------------
class CLoginState final : public CGameState,
                          public mxh::net::IConnectionHandler {
public:
    CLoginState();
    ~CLoginState() override;

    CLoginState(const CLoginState&)            = delete;
    CLoginState& operator=(const CLoginState&) = delete;

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

    // Public start hook.  Host calls this right after SetGameState()
    // to begin the connection.  Idempotent — calling twice is a no-op
    // (the existing TcpClient is reused).
    void Start(CEngine* engine, std::string host, std::uint16_t port,
               std::string user_id, std::string password);

    // Inspectors (test + diagnostics).
    bool        is_connected()   const noexcept;
    std::uint32_t auth_key()     const noexcept { return m_authKey; }
    std::uint32_t user_idx()     const noexcept { return m_userIdx; }
    std::string  agent_addr()    const { return m_agentAddr; }
    std::uint16_t agent_port()   const noexcept { return m_agentPort; }

private:
    void dispatch_login_ack(const LegacyLoginAck& ack);
    void fail_with(const std::string& reason);

    CEngine*                 m_pEngine    = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    std::string              m_host       = "127.0.0.1";
    std::uint16_t            m_port       = 6001;
    std::string              m_userId;
    std::string              m_password;

    std::uint32_t            m_authKey    = 0;        // received in DistConnectSuccess
    std::uint32_t            m_userIdx    = 0;        // from LoginAck
    std::string              m_agentAddr;
    std::uint16_t            m_agentPort  = 0;

    bool                     m_started    = false;
    bool                     m_ackReceived = false;
    bool                     m_failed     = false;
    std::string              m_failureReason;
};

} // namespace mxh::client
