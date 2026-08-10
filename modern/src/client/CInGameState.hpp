// mxh/client/CInGameState.hpp
// Phase B.2.3 â€” in-game state (replaces CGameIn stub).
//
// Wires the eGS_GAMEIN (GameStateId::GameIn = 7) slot to the modern
// mxh::net::TcpClient and drives the legacy 4DyuchiNET game-in
// handshake against MoxianMapServer:
//
//   client.Connect(map_host:map_port) ->  no DistConnectSuccess (MapServer
//                                         on_connect is silent)
//                                     ->  send GameInSyn (proto=28, empty
//                                         payload; player_id in MSGBASE)
//                                     ->  recv GameInAck (proto=29, ~3000B
//                                         SEND_HERO_TOTALINFO)
//                                     ->  parse + render "in game" overlay
//
// 1:1 with the legacy MHClient GameIn flow + map_handler.cpp:handle_gamein.
// In the legacy flow the player goes through GameLoading first; the
// modern client can either (a) get there via GameLoading (Phase C+)
// or (b) jump straight from CharSelect to GameIn (B.2.3 dev path).
//
// Modern port notes:
//   * Bypasses the AgentServer's Phase 9 forwarding (which requires
//     CharacterSelectSyn first).  MapServer is reached directly so the
//     "in game" overlay can be exercised before B.4+ wires up
//     Player/AISystem.
//   * GameInSyn payload is empty (the channel/level fields the
//     legacy MSG_DWORD2 carried aren't read by the modern server).
//   * GameInAck parser is a free function so unit tests can lock the
//     SEND_HERO_TOTALINFO layout without a TcpClient.

#pragma once

#include "CGameState.hpp"

#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mxh/net/net.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"

namespace mxh::client {

class CEngine;

// 1:1 with the SEND_HERO_TOTALINFO layout in map_handler.cpp
// (kPayloadBaseObjOff/kPayloadCharTotalOff/...).  We expose the
// minimum subset needed to drive a "you are in game" overlay.
struct GameInInfo {
    std::uint32_t  player_id    = 0;
    std::uint32_t  user_id      = 0;
    std::string    name;             // up to 16 chars + NUL
    std::uint16_t  level        = 0;
    std::uint16_t  map_num      = 0;
    std::uint16_t  life         = 0;
    std::uint16_t  max_life     = 0;
    std::uint8_t   gender       = 0;
    std::uint8_t   face_type    = 0;
    std::uint8_t   hair_type    = 0;
    std::array<std::uint16_t, 10> weared_item_idx{};
    std::uint16_t  position_x   = 0;
    std::uint16_t  position_z   = 0;
    // Server-time stamp from SYSTEMTIME (year/month/wday/day/hour).
    std::uint16_t  server_year  = 0;
    std::uint16_t  server_month = 0;
    std::uint16_t  server_day   = 0;
    std::uint16_t  server_hour  = 0;
};

// Parse the legacy GameInAck payload (map_handler.cpp
// make_gamein_ack).  Returns std::nullopt if the payload is too short
// to safely read BASEOBJECT_INFO / CHARACTER_TOTALINFO / ServerTime.
std::optional<GameInInfo>
parse_legacy_gamein_ack(std::span<const std::uint8_t> payload);

// Server-pushed monster state received after GameInAck.
// Mirrors the wire layout written by map_handler.cpp::broadcast_monster_add:
//   [BASEOBJECT_INFO: 35 bytes] [MONSTER_TOTALINFO: 14 bytes]
//   [SEND_MOVEINFO: 14 bytes] [MobVelocityTable1: 1 byte]
struct MonsterAddInfo {
    std::uint32_t object_id = 0;
    std::uint32_t user_id = 0;
    char name[17] = {};
    std::uint32_t current_life = 0;
    std::uint32_t current_shield = 0;
    std::uint16_t monster_kind = 0;
    std::uint16_t group = 0;
    std::uint16_t map_num = 0;
    std::uint16_t position_x = 0;
    std::uint16_t position_z = 0;
};

std::optional<MonsterAddInfo>
parse_legacy_monster_add(std::span<const std::uint8_t> payload);

// -------------------------------------------------------------------------
// CInGameState â€” eGS_GAMEIN state.
// -------------------------------------------------------------------------
class CInGameState final : public CGameState,
                           public mxh::net::IConnectionHandler {
public:
    CInGameState();
    ~CInGameState() override;

    CInGameState(const CInGameState&)            = delete;
    CInGameState& operator=(const CInGameState&) = delete;

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

    // Public start hook.  MapServer is reached directly (bypassing
    // Agent's Phase 9 forwarding).  Idempotent.
    void Start(CEngine* engine, std::string host, std::uint16_t port,
               std::uint32_t player_id, std::uint16_t map_num,
               bool use_hsel = false);

    // Inspectors (test + overlay).
    bool         is_connected() const noexcept;
    bool         is_in_game()   const noexcept { return m_inGame; }
    std::uint32_t player_id()   const noexcept { return m_playerId; }
    std::uint16_t map_num()     const noexcept { return m_mapNum; }
    const GameInInfo& game_info() const noexcept { return m_info; }
    const std::vector<MonsterAddInfo>& monsters() const noexcept { return monsters_; }

private:
    void send_gamein_syn();
    void dispatch_gamein_ack(const GameInInfo& info);
    void fail_with(const std::string& reason);

    CEngine*                 m_pEngine    = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    std::string              m_host       = "127.0.0.1";
    std::uint16_t            m_port       = 8001;
    std::uint32_t            m_playerId   = 0;
    std::uint16_t            m_mapNum     = 0;

    GameInInfo               m_info;
    std::vector<MonsterAddInfo> monsters_;
    bool                     m_started    = false;
    bool                     m_inGame     = false;
    bool                     m_failed     = false;
    bool                     m_sentGameInSyn = false;  // gate for Process() retry
    bool                     m_releasing = false;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;
    std::string              m_failureReason;
};

} // namespace mxh::client
