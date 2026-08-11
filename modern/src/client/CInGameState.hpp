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
#include <unordered_map>
#include <utility>
#include <vector>

#include "mxh/net/net.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/game/item_types.hpp"

namespace mxh::client {

class CEngine;

// 1:1 with legacy MUGONGBASE (packed 18 bytes).
struct MugongInfo {
    std::uint32_t db_idx       = 0;
    std::uint16_t icon_idx     = 0;  // mugong/skill idx
    std::uint16_t position     = 0;
    std::uint32_t exp          = 0;
    std::uint8_t  sung         = 0;
    std::uint8_t  wear         = 0;
    std::uint16_t quick_position = 0;
    std::uint16_t option_idx   = 0;
};

inline constexpr std::size_t kMugongSlotCount = 25;  // 20 mugong + 5 jinbub
inline constexpr std::size_t kQuickSlotCount  = 8;

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
    std::uint32_t  mp           = 0;
    std::uint32_t  max_mp       = 0;
    std::uint32_t  exp          = 0;
    std::uint32_t  money        = 0;
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
    std::array<MugongInfo, kMugongSlotCount> mugong{};
    mxh::game::ItemTotalInfo items{};
};

// Parse the legacy GameInAck payload (map_handler.cpp
// make_gamein_ack).  Returns std::nullopt if the payload is too short
// to safely read BASEOBJECT_INFO / CHARACTER_TOTALINFO / ServerTime.
std::optional<GameInInfo>
parse_legacy_gamein_ack(std::span<const std::uint8_t> payload);

std::array<MugongInfo, kMugongSlotCount>
parse_legacy_mugong_total(std::span<const std::uint8_t> payload);

mxh::game::ItemTotalInfo
parse_legacy_item_total(std::span<const std::uint8_t> payload);

// Quick-slot skill for a slot index. Uses parsed mugong data when present,
// otherwise the level-1 starter set [1,2,3,10] until the server sends real
// per-character skills.
std::uint32_t quick_skill_for_slot(const GameInInfo& info,
                                   std::size_t slot) noexcept;

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
// In-game input + gameplay wire helpers (pure functions, unit-tested).
//
// Movement follows the legacy MHClient bindings: W/S forward/back, Q/E
// strafe, A/D rotate the camera (arrow keys mirror W/S/A/D), mouse right
// drag rotates the camera, left click attacks the nearest monster.
// -------------------------------------------------------------------------
enum class MoveKey : std::uint32_t {
    Forward     = 1u << 0,
    Back        = 1u << 1,
    StrafeLeft  = 1u << 2,
    StrafeRight = 1u << 3,
    RotateLeft  = 1u << 4,
    RotateRight = 1u << 5,
};

struct MoveResult {
    float x = 0;
    float z = 0;
    float yaw = 0;
    bool moving = false;  // position changed this step
};

// Win32 virtual-key codes (kept local so the client library stays
// windows-free in tests).
inline constexpr std::uint32_t kVkW      = 0x57;
inline constexpr std::uint32_t kVkA      = 0x41;
inline constexpr std::uint32_t kVkS      = 0x53;
inline constexpr std::uint32_t kVkD      = 0x44;
inline constexpr std::uint32_t kVkQ      = 0x51;
inline constexpr std::uint32_t kVkE      = 0x45;
inline constexpr std::uint32_t kVkReturn = 0x0D;
inline constexpr std::uint32_t kVkEscape = 0x1B;
inline constexpr std::uint32_t kVkBack   = 0x08;
inline constexpr std::uint32_t kVkUp     = 0x26;
inline constexpr std::uint32_t kVkDown   = 0x28;
inline constexpr std::uint32_t kVkLeft   = 0x25;
inline constexpr std::uint32_t kVkRight  = 0x27;

inline constexpr float kMoveSpeed       = 220.0f;  // world units / second
inline constexpr float kRotateSpeed     = 1.6f;    // radians / second
inline constexpr float kMoveReportEveryMs = 300.0f;  // legacy 300ms notice
inline constexpr float kAttackCooldownMs = 800.0f;
inline constexpr float kAttackRange      = 500.0f;
inline constexpr float kWorldLimit       = 50000.0f;

std::uint32_t key_mask_for_vk(std::uint32_t vk) noexcept;

// Advance position/yaw for one tick given the held key mask.
MoveResult step_movement(std::uint32_t keyMask, float yaw,
                         float x, float z, float dt) noexcept;

// Nearest alive monster within range, or std::nullopt.
std::optional<std::uint32_t>
pick_attack_target(const std::vector<MonsterAddInfo>& monsters,
                   float px, float pz, float range) noexcept;

// Build the modern MapServer Move packet (payload = [x:u16][z:u16]).
mxh::net::Message make_move_message(std::uint32_t player_id,
                                    mxh::proto::MoveProtocol proto,
                                    std::uint16_t x, std::uint16_t z);

// Build the modern MapServer Skill StartSyn packet
// (payload = [skill_idx:u32][main_target:u32][target_x:f32][target_z:f32]).
mxh::net::Message make_attack_message(std::uint32_t player_id,
                                      std::uint32_t skill_idx,
                                      std::uint32_t main_target,
                                      float target_x, float target_z);

std::optional<std::pair<std::uint16_t, std::uint16_t>>
parse_move_payload(std::span<const std::uint8_t> payload);

std::optional<std::pair<std::uint32_t, std::uint32_t>>
parse_monster_life_payload(std::span<const std::uint8_t> payload);

// Build the modern MapServer Chat All packet (payload = message bytes).
mxh::net::Message make_chat_message(std::uint32_t player_id,
                                    const std::string& text);

std::string parse_chat_payload(std::span<const std::uint8_t> payload);

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

    // Input hooks driven by the host Win32 message pump (in-game only).
    void OnKeyEvent(bool pressed, std::uint32_t vk);
    void OnChar(std::uint32_t ch);
    void OnMouseButton(bool left, bool down, std::int32_t x, std::int32_t y);
    void OnMouseMove(std::int32_t x, std::int32_t y);
    void use_quick_slot(std::size_t slot);
    void toggle_inventory() noexcept { m_inventoryOpen = !m_inventoryOpen; }

    // Inspectors (test + overlay).
    bool         is_connected() const noexcept;
    bool         is_in_game()   const noexcept { return m_inGame; }
    std::uint32_t player_id()   const noexcept { return m_playerId; }
    std::uint16_t map_num()     const noexcept { return m_mapNum; }
    const GameInInfo& game_info() const noexcept { return m_info; }
    const std::vector<MonsterAddInfo>& monsters() const noexcept { return monsters_; }
    float camera_yaw() const noexcept { return m_cameraYaw; }
    bool chat_open() const noexcept { return m_chatOpen; }
    bool inventory_open() const noexcept { return m_inventoryOpen; }
    const std::string& chat_buffer() const noexcept { return m_chatBuffer; }
    const std::vector<std::string>& chat_lines() const noexcept {
        return m_chatLines;
    }
    std::uint16_t local_x() const noexcept {
        return static_cast<std::uint16_t>(m_localX);
    }
    std::uint16_t local_z() const noexcept {
        return static_cast<std::uint16_t>(m_localZ);
    }

private:
    void send_gamein_syn();
    void dispatch_gamein_ack(const GameInInfo& info);
    void fail_with(const std::string& reason);
    void update_movement(std::uint64_t now_ms);
    void send_move(std::uint16_t x, std::uint16_t z,
                   mxh::proto::MoveProtocol proto);
    void try_attack();
    void send_chat();
    void handle_userconn_message(const mxh::net::Message& msg);
    void handle_move_broadcast(const mxh::net::Message& msg);
    void handle_monster_broadcast(const mxh::net::Message& msg);
    void handle_skill_broadcast(const mxh::net::Message& msg);
    void handle_chat_broadcast(const mxh::net::Message& msg);

    CEngine*                 m_pEngine    = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    std::string              m_host       = "127.0.0.1";
    std::uint16_t            m_port       = 8001;
    std::uint32_t            m_playerId   = 0;
    std::uint16_t            m_mapNum     = 0;

    GameInInfo               m_info;
    std::vector<MonsterAddInfo> monsters_;
    std::unordered_map<std::uint32_t, std::pair<std::uint16_t, std::uint16_t>>
        m_remotePlayers;
    bool                     m_started    = false;
    bool                     m_inGame     = false;
    bool                     m_failed     = false;
    bool                     m_sentGameInSyn = false;  // gate for Process() retry
    bool                     m_releasing = false;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;
    std::string              m_failureReason;

    // In-game input/movement state.
    std::uint32_t  m_keyMask      = 0;
    float          m_localX       = 0;
    float          m_localZ       = 0;
    float          m_cameraYaw    = 0;
    bool           m_moving       = false;
    std::uint64_t  m_lastTickMs   = 0;
    std::uint64_t  m_lastMoveSendMs = 0;
    std::uint64_t  m_lastAttackMs = 0;
    std::int32_t   m_lastMouseX   = 0;
    std::int32_t   m_lastMouseY   = 0;
    bool           m_cameraDrag   = false;

    // Chat state (in-game).
    bool                 m_chatOpen   = false;
    std::string          m_chatBuffer;
    std::vector<std::string> m_chatLines;
    bool                 m_inventoryOpen = false;
};

} // namespace mxh::client
