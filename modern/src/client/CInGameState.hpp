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
#include "EffectRuntime.hpp"
#include "mxh/game/skill_manager.hpp"
#include "mxh/game/experience_curve.hpp"
#include "ClientUiRuntime.hpp"

#include <cstdint>
#include <functional>
#include <future>
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
#include "mxh/compat/quest_string_catalog.hpp"
#include "mxh/game/item_types.hpp"
#include "services/InventoryServiceImpl.hpp"
#include "mxh/services/IPlayerStatsService.hpp"

namespace mxh::ui { class cOptionDialog; class cMiniFriendDialog; class cFriendDialog; }

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
    std::uint16_t  gen_gol      = 0;
    std::uint16_t  min_chub     = 0;
    std::uint16_t  che_ryuk     = 0;
    std::uint16_t  sim_mek      = 0;
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
    float facing_yaw = 0.0f;
    bool moving = false;
};

std::optional<MonsterAddInfo>
parse_legacy_monster_add(std::span<const std::uint8_t> payload);

// One remote player pushed by MapServer (UserConn CharacterAdd).
// Mirrors the fixed prefix of legacy SEND_CHARACTER_TOTALINFO:
//   [BASEOBJECT_INFO: 35 bytes] [CHARACTER_TOTALINFO: 112 bytes]
//   [SEND_MOVEINFO: 14 bytes]
struct RemotePlayerInfo {
    std::uint32_t object_id = 0;
    std::uint32_t user_id = 0;
    std::string name;
    std::uint32_t life = 0;
    std::uint32_t max_life = 0;
    std::uint32_t shield = 0;
    std::uint32_t max_shield = 0;
    std::uint8_t gender = 0;
    std::uint8_t face_type = 0;
    std::uint8_t hair_type = 0;
    std::array<std::uint16_t, 10> weared_item_idx{};
    std::uint16_t level = 0;
    std::uint16_t map_num = 0;
    std::uint16_t position_x = 0;
    std::uint16_t position_z = 0;
    float height = 0.0f;
    float width = 0.0f;
    bool visible = false;
    bool appearance_known = false;
    float facing_yaw = 0.0f;
    bool moving = false;
};

std::optional<RemotePlayerInfo>
parse_legacy_character_add(std::span<const std::uint8_t> payload);

// One static NPC pushed by the map server (UserConn NpcAdd, 64B payload).
struct NpcInfo {
    std::uint32_t npc_id = 0;
    std::uint16_t npc_kind = 0;
    char name[17] = {};
    std::uint16_t position_x = 0;
    std::uint16_t position_z = 0;
};

// Server-confirmed presentation event. Gameplay remains authoritative on the
// server; BEFF, animation and audio consumers subscribe to this timeline.
enum class EffectEventKind : std::uint8_t { CastStart, CastRelease, Hit, End, Death };

struct EffectEvent {
    EffectEventKind kind = EffectEventKind::CastStart;
    std::uint64_t timestamp_ms = 0;
    std::uint32_t source_object_id = 0;
    std::uint32_t target_object_id = 0;
    std::uint32_t skill_id = 0;
    std::uint32_t effect_id = 0;
    std::uint32_t sound_id = 0;
    std::int32_t damage = 0;
    std::uint8_t hit_result = 0;
};

std::optional<NpcInfo> parse_legacy_npc_add(std::span<const std::uint8_t> payload);

// Approximate world -> screen projection for static NPC markers, using the
// same camera convention as the terrain scene (yaw around Y, forward = +Z at
// yaw 0). Returns false when the marker is behind the camera.
bool project_npc_to_screen(float player_x, float player_z, float yaw,
                           float npc_x, float npc_z,
                           float& screen_x, float& screen_y) noexcept;

// Inverse of project_npc_to_screen for the fixed 4:3 logical world canvas.
// Returns false for clicks behind the camera plane.
bool unproject_screen_to_world(float player_x, float player_z, float yaw,
                               float screen_x, float screen_y,
                               float& world_x, float& world_z) noexcept;

// -------------------------------------------------------------------------
// In-game input + gameplay wire helpers (pure functions, unit-tested).
//
// Movement follows the legacy MHClient bindings: W/S forward/back, Q/E
// strafe, A/D rotate the camera (arrow keys mirror W/S/A/D), mouse right
// drag rotates the camera, left click attacks an entity under the cursor or
// issues a point-to-move request when the click lands on empty world space.
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
inline constexpr float kPickupRange      = 500.0f;
inline constexpr float kNpcInteractionRange = 500.0f;
// Coordinates on the legacy wire are unsigned 16-bit values.  Do not impose
// an artificial 50,000-unit ceiling: map-specific bounds may use the full
// representable domain and are clamped only when explicitly supplied.
inline constexpr float kWorldLimit       = 65535.0f;
inline constexpr std::uint32_t kVkF      = 0x46;
inline constexpr std::uint32_t kVkL      = 0x4C;

std::uint32_t key_mask_for_vk(std::uint32_t vk) noexcept;

// Advance position/yaw for one tick given the held key mask.
MoveResult step_movement(std::uint32_t keyMask, float yaw,
                         float x, float z, float dt,
                         float max_x = kWorldLimit,
                         float max_z = kWorldLimit) noexcept;

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

mxh::net::Message make_quest_message(std::uint32_t player_id,
                                     mxh::proto::QuestProtocol protocol,
                                     std::uint16_t quest_id);

std::optional<std::pair<std::uint16_t, std::uint16_t>>
parse_move_payload(std::span<const std::uint8_t> payload);

std::optional<std::pair<std::uint32_t, std::uint32_t>>
parse_monster_life_payload(std::span<const std::uint8_t> payload);

// Build the modern MapServer Chat All packet (payload = message bytes).
mxh::net::Message make_chat_message(std::uint32_t player_id,
                                    const std::string& text);

std::string parse_chat_payload(std::span<const std::uint8_t> payload);

// One NPC shop row (modern ShopList payload).
struct ShopItem {
    std::uint16_t item_id = 0;
    std::uint32_t price   = 0;
};

// Parse the modern ShopList payload:
// [npc_id:u32][count:u16] + count x [item_id:u16][price:u32].
std::vector<ShopItem> parse_shop_list(std::span<const std::uint8_t> payload);

// Buy packet: [item_id:u16][qty:u16].
mxh::net::Message make_buy_message(std::uint32_t player_id,
                                   std::uint16_t item_id,
                                   std::uint16_t qty);

// Ground drop pushed after a monster dies (Item MonsterObtainNotify, 20B):
//   [object_id:u32][source_monster:u32][item_id:u16][count:u16][x:f32][z:f32]
struct GroundDropInfo {
    std::uint32_t object_id = 0;
    std::uint32_t source_monster_id = 0;
    std::uint16_t item_id = 0;
    std::uint16_t count = 1;
    float position_x = 0.0f;
    float position_z = 0.0f;
};

std::optional<GroundDropInfo>
parse_legacy_ground_drop(std::span<const std::uint8_t> payload);

mxh::net::Message make_pickup_message(std::uint32_t player_id,
                                      std::uint32_t drop_object_id);

// Shop panel layout (shared with the host renderer).
inline constexpr float kShopPanelX = 200.0f;
inline constexpr float kShopPanelY = 100.0f;
inline constexpr float kShopPanelW = 400.0f;
inline constexpr float kShopRowH   = 26.0f;

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
    void PrepareForProcessExit() noexcept override;
    void Process() override;

    // mxh::net::IConnectionHandler
    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
void on_disconnect(mxh::net::ConnectionId id,
                   mxh::net::NetError reason) override;
mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id) override;

    // Compatibility overload: host/port are ignored. GameInSyn always uses
    // the long-lived AgentSession, which forwards it to MapServer.
    void Start(CEngine* engine, std::string host, std::uint16_t port,
               std::uint32_t player_id, std::uint16_t map_num,
               bool use_hsel = false);
    void Start(CEngine* engine, std::uint32_t player_id, std::uint16_t map_num);

    // Input hooks driven by the host Win32 message pump (in-game only).
    void OnKeyEvent(bool pressed, std::uint32_t vk);
    void OnChar(std::uint32_t ch);
    bool OnMouseButton(bool left, bool down, std::int32_t x, std::int32_t y);
    void OnMouseMove(std::int32_t x, std::int32_t y);
    void OnMouseWheel(std::int32_t delta);
    void set_world_bounds(float max_x, float max_z) noexcept;
    void set_collision_query(std::function<bool(float, float, float)> query);
    // Resolve a MapChange-role NPC to its authoritative destination.  The
    // client must not invent a map number when the server/resource route is
    // unavailable; an empty result leaves the current scene untouched.
    using MapChangeTargetResolver =
        std::function<std::optional<std::uint16_t>(std::uint32_t npc_id,
                                                    std::uint16_t current_map)>;
    void set_map_change_target_resolver(MapChangeTargetResolver resolver);
    void use_quick_slot(std::size_t slot);
    void toggle_inventory() noexcept;
    void try_pickup();
    // Pick the nearest NPC to the player by world distance (max 500 units).
    std::uint32_t pick_nearest_npc() const noexcept;
    void open_shop(std::uint32_t npc_id);
    void buy_shop_item(std::size_t index);
    void set_quest_catalog(mxh::compat::QuestStringCatalog catalog);

    // Inspectors (test + overlay).
    bool         is_connected() const noexcept;
    bool         is_in_game()   const noexcept { return m_inGame; }
    bool         is_failed() const noexcept { return m_failed; }
    const std::string& failure_reason() const noexcept { return m_failureReason; }
    bool smoke_exit_requested() const noexcept { return m_smokeExitRequested; }
    // The smoke-exit gate is the canonical "the world has settled enough
    // to take a meaningful acceptance screenshot" predicate.  Production
    // smoke harnesses call --exit-after-gamein / MXH_GUI_SMOKE_EXIT=1 and
    // immediately set m_smokeExitRequested from the GameInAck callback.
    // The naive `monsters_.size() >= 228` check used to return true on
    // the very first frame, which made the GUI client close before any
    // terrain or sprite had been drawn.  We now require a configurable
    // number of in-game Process() ticks so the headless capture path
    // observes a populated, rendered frame.  The required frame count
    // comes from the env var MXH_GUI_SMOKE_SETTLE_FRAMES (default 90,
    // matches --smoke-settle-frames) and is captured once at GameInAck
    // so a late env-var change cannot race the exit.
    bool smoke_exit_ready() const noexcept {
        if (!m_smokeExitRequested) return false;
        if (m_smokeSettleFrames < m_smokeSettleRequired) return false;
        if (m_mapNum == 10) return monsters_.size() >= 228;
        return !monsters_.empty() || !m_npcs.empty();
    }
    std::uint32_t smoke_settle_frames() const noexcept {
        return m_smokeSettleFrames;
    }
    std::uint32_t smoke_settle_required() const noexcept {
        return m_smokeSettleRequired;
    }
    std::uint32_t player_id()   const noexcept { return m_playerId; }
    // Resolve an authoritative world distance for effect/audio consumers.
    // Unknown IDs remain absent so remote sounds do not become local 2D cues.
    std::optional<float> distance_to_object(std::uint32_t object_id) const noexcept;
    std::uint16_t map_num()     const noexcept { return m_mapNum; }
    const GameInInfo& game_info() const noexcept { return m_info; }
    const std::vector<MonsterAddInfo>& monsters() const noexcept { return monsters_; }
    const std::vector<GroundDropInfo>& ground_drops() const noexcept {
        return m_groundDrops;
    }
    std::uint32_t pending_pickup_drop() const noexcept { return m_pendingPickupDrop; }
    std::uint32_t last_attack_target() const noexcept { return m_lastAttackTarget; }
    const std::vector<NpcInfo>& npcs() const noexcept { return m_npcs; }
    const std::unordered_map<std::uint32_t, RemotePlayerInfo>& remote_players()
        const noexcept { return m_remotePlayers; }
    float camera_yaw() const noexcept { return m_cameraYaw; }
    float camera_distance() const noexcept { return m_cameraDistance; }
    // Test/smoke hook: override the follow-camera yaw so the canonical
    // Map 10 NPC spawn positions land inside the 4:3 viewport.  Production
    // players continue to rotate via right-button drag (OnMouseMove) so
    // this helper is opt-in from main.cpp when --follow-camera is set.
    void set_camera_yaw(float radians) noexcept { m_cameraYaw = radians; }
    bool is_moving() const noexcept { return m_moving; }
    bool chat_open() const noexcept { return m_chatOpen; }
    bool inventory_open() const noexcept { return m_inventoryOpen; }
    std::size_t inventory_tab() const noexcept { return m_inventoryTab; }
    bool shop_open() const noexcept { return m_shopOpen; }
    bool quest_open() const noexcept { return m_questOpen; }
    bool character_open() const noexcept { return m_characterOpen; }
    bool map_open() const noexcept { return m_mapOpen; }
    bool friend_open() const noexcept { return m_friendOpen; }
    bool guild_open() const noexcept { return m_guildOpen; }
    bool option_open() const noexcept { return m_optionOpen; }
    mxh::ui::cOptionDialog* option_dialog() noexcept;
    mxh::ui::cMiniFriendDialog* mini_friend_dialog() noexcept;
    mxh::ui::cFriendDialog* friend_dialog() noexcept;
    std::uint16_t quest_id() const noexcept { return m_questId; }
    const std::string& quest_status() const noexcept { return m_questStatus; }
    const mxh::compat::QuestStringEntry* selected_quest() const noexcept {
        return m_questSelection < m_mainQuests.size() ? m_mainQuests[m_questSelection] : nullptr;
    }
    const std::vector<ShopItem>& shop_items() const noexcept { return m_shopItems; }
    const std::vector<EffectEvent>& effect_events() const noexcept { return m_effectEvents; }
    std::vector<EffectEvent> drain_effect_events() noexcept;
    const mxh::game::EffectCatalog& effect_catalog() const noexcept {
        return m_effectRuntime.catalog();
    }
    EffectRuntime& effect_runtime() noexcept { return m_effectRuntime; }
    void set_effect_tick_per_frame_ms(float value) noexcept {
        m_effectTickPerFrameMs = value;
    }
    std::vector<RuntimeEffectEvent> drain_runtime_effect_events() noexcept;
    const mxh::game::SkillManager& skill_manager() const noexcept {
        return m_skillManager;
    }
    std::uint32_t shop_npc_id() const noexcept { return m_shopNpcId; }
    std::uint32_t last_buy_item_id() const noexcept { return m_lastBuyItemId; }
    const std::string& chat_buffer() const noexcept { return m_chatBuffer; }
    const std::vector<std::string>& chat_lines() const noexcept {
        return m_chatLines;
    }
    std::uint32_t party_id() const noexcept { return m_partyId; }
    std::uint8_t party_member_count() const noexcept { return m_partyMemberCount; }
    bool request_party_create(std::uint8_t option = 0);
    bool request_party_invite(std::uint32_t target_player_id);
    bool accept_party_invite();
    std::uint32_t pending_party_invite_id() const noexcept { return m_pendingPartyInviteId; }
    std::uint32_t guild_id() const noexcept { return m_guildId; }
    std::uint8_t guild_member_count() const noexcept { return m_guildMemberCount; }
    bool request_guild_create(std::string_view name);
    bool request_guild_invite(std::uint32_t target_player_id);
    bool accept_guild_invite();
    std::uint32_t pending_guild_invite_id() const noexcept { return m_pendingGuildInviteId; }
    bool request_friend_add(std::uint32_t target_player_id);
    bool request_friend_add_by_name(std::string_view name);
    bool request_friend_delete(std::uint32_t friend_id);
    bool accept_friend_invite();
    bool deny_friend_invite();
    std::uint32_t pending_friend_invite_id() const noexcept { return m_pendingFriendInviteId; }
    ClientUiRuntime& ui_runtime() noexcept { return m_uiRuntime; }
    const ClientUiRuntime& ui_runtime() const noexcept { return m_uiRuntime; }
    const std::string& last_item_error() const noexcept { return m_lastItemError; }
    const std::string& last_npc_error() const noexcept { return m_lastNpcError; }
    const std::string& last_skill_error() const noexcept { return m_lastSkillError; }
    const std::string& last_friend_error() const noexcept { return m_lastFriendError; }
    std::int32_t last_damage() const noexcept { return m_lastDamage; }
    std::uint32_t last_hit_target() const noexcept { return m_lastHitTarget; }
    std::uint8_t last_hit_result() const noexcept { return m_lastHitResult; }
    std::uint64_t last_damage_timestamp_ms() const noexcept {
        return m_lastDamageTimestampMs;
    }
    // Attack flash age in ms; 0 = no active flash (attack happened >200ms ago or none).
    std::uint64_t attack_flash_age_ms() const noexcept;

    std::uint16_t local_x() const noexcept {
        return static_cast<std::uint16_t>(m_localX);
    }
    std::uint16_t local_z() const noexcept {
        return static_cast<std::uint16_t>(m_localZ);
    }

public:
    // ---- Phase 0 §6.4 protocol-burst test hooks ---------------------------
    // Direct dispatch (skips network recv thread) so the test can slam the
    // state with hundreds of synthetic packets per frame and verify the
    // state machine does not double-free / leak / promote to in-game
    // before GameInAck.  Production code never calls these; they exist
    // purely so protocol_burst_test.cpp can drive on_message() without
    // spinning up a real MapServer connection.
    // m_dispatchEnabledForTest is consulted inside HandleMessageForTest
    // so a missing opt-in is loud (no-op) rather than a silent fall-through.
    void SetDispatchForTest(bool enabled) noexcept { m_dispatchEnabledForTest = enabled; }
    void HandleMessageForTest(const mxh::net::Message& msg) {
        if (!m_dispatchEnabledForTest) return;
        on_message({}, msg);
    }

    // ---- Phase 1 test hook: arm the GameInAck timeout.  Mirrors the
    // CLoginState / CCharSelectState / CCharMake ack-timeout pattern
    // (commits fa74305e / 45009501 / b4d2b68c).  Production code never
    // calls these.  `ArmGameInAckDeadlineForTest` puts the pending
    // timestamp `m_gameInAckTimeoutMs` milliseconds in the past so the
    // next Process() tick fires the timeout.
    void SetGameInAckTimeoutForTest(std::uint64_t budget_ms) noexcept {
        m_gameInAckTimeoutMs = budget_ms;
    }
    void ArmGameInAckDeadlineForTest() noexcept;
    bool handle_ui_activation(const ClientUiActivation& activation);
    void send_gamein_syn();
    void send_gameout_syn();
    bool request_inventory_move(std::size_t source, std::size_t target);
    void dispatch_gamein_ack(const GameInInfo& info);
    void fail_with(const std::string& reason);
    void update_movement(std::uint64_t now_ms);
    bool move_to_screen(float screen_x, float screen_y);
    // Returns true when the interaction request was handled locally or sent
    // to the server.  A false result lets the click path fall back to
    // click-to-move for a visible but distant NPC.
    bool interact_with_npc(std::uint32_t npc_id);
    void send_move(std::uint16_t x, std::uint16_t z,
                   mxh::proto::MoveProtocol proto);
    void try_attack();
    std::uint32_t pick_drop_at_screen(float sx, float sy) const;
    std::uint32_t pick_nearest_drop() const noexcept;
    std::uint32_t pick_monster_at_screen(float sx, float sy) const;
    void send_chat();
    std::uint32_t pick_npc_at_screen(float sx, float sy) const;
    void handle_userconn_message(const mxh::net::Message& msg);
    void handle_move_broadcast(const mxh::net::Message& msg);
    void handle_monster_broadcast(const mxh::net::Message& msg);
    void handle_skill_broadcast(const mxh::net::Message& msg);
    void handle_chat_broadcast(const mxh::net::Message& msg);
    void handle_party_message(const mxh::net::Message& msg);
    void handle_guild_message(const mxh::net::Message& msg);
    void handle_friend_message(const mxh::net::Message& msg);
    void handle_item_broadcast(const mxh::net::Message& msg);
    void handle_npc_message(const mxh::net::Message& msg);
    bool apply_pickup_to_inventory(std::uint32_t drop_id,
                                   std::uint16_t item_id,
                                   std::uint16_t count) noexcept;
    void handle_quest_broadcast(const mxh::net::Message& msg);
    void send_quest(mxh::proto::QuestProtocol protocol);
    void refresh_live_ui_bindings();
    void set_inventory_open(bool open) noexcept;
    void set_shop_open(bool open) noexcept;
    void set_quest_open(bool open) noexcept;
    void set_character_open(bool open) noexcept;
    void set_chat_open(bool open) noexcept;
    void set_map_open(bool open) noexcept;
    void set_friend_open(bool open) noexcept;
    void set_mini_friend_open(bool open) noexcept;
    void set_guild_open(bool open) noexcept;
    void set_option_open(bool open) noexcept;
    bool select_quest_index(std::size_t index) noexcept;
    bool select_inventory_tab(std::size_t tab) noexcept;
    void push_effect_event(EffectEvent event) noexcept;
    void start_skill_effect(std::uint32_t skill_id,
                            std::uint32_t target_object_id,
                            std::uint64_t now_ms,
                            std::uint32_t source_object_id = 0);

    CEngine*                 m_pEngine    = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    std::string              m_host       = "127.0.0.1";
    std::uint16_t            m_port       = 8001;
    std::uint32_t            m_playerId   = 0;
    std::uint16_t            m_mapNum     = 0;

    GameInInfo               m_info;
    std::unique_ptr<mxh::services::InventoryServiceImpl> m_inventoryService;
    std::unique_ptr<mxh::services::IPlayerStatsService> m_playerStatsService;
    std::unique_ptr<mxh::game::ExperienceCurve> m_experienceCurve;
    std::vector<MonsterAddInfo> monsters_;
    std::vector<GroundDropInfo> m_groundDrops;
    // One authoritative pickup request may be in flight at a time.  This
    // prevents key-repeat or double-click input from duplicating PickupSyn.
    std::uint32_t            m_pendingPickupDrop = 0;
    std::uint64_t            m_pendingPickupSinceMs = 0;
    std::uint32_t            m_lastAttackTarget = 0;
    std::uint32_t            m_pendingAttackTarget = 0;
    std::uint32_t            m_pendingSkillId = 0;
    std::uint32_t            m_pendingSkillTargetId = 0;
    std::uint64_t             m_pendingSkillSinceMs = 0;
    std::vector<NpcInfo> m_npcs;
    std::unordered_map<std::uint32_t, RemotePlayerInfo> m_remotePlayers;
    bool                     m_started    = false;
    bool                     m_inGame     = false;
    bool                     m_failed     = false;
    bool                     m_sentGameInSyn = false;  // gate for Process() retry
    bool                     m_sentGameOutSyn = false;
    // Phase 1: timestamp (steady_now_ms) of the most recent GameInSyn
    // send.  0 means no Syn is in flight (cleared by the GameInAck
    // dispatch path or by Release).  Mirrors the four other request
    // timeouts in this class (pickup / inventory / combat / shop).
    std::uint64_t            m_pendingGameInAckSinceMs = 0;
    std::uint64_t            m_gameInAckTimeoutMs      = 10000;  // 10s default
    bool                     m_releasing = false;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;
    std::string              m_failureReason;
    std::string              m_lastItemError;
    std::string              m_lastNpcError;
    std::string              m_lastSkillError;
    std::string              m_lastFriendError;
    std::int32_t             m_lastDamage = 0;
    std::uint32_t            m_lastHitTarget = 0;
    std::uint8_t              m_lastHitResult = 0;
    std::uint64_t             m_lastDamageTimestampMs = 0;

    // In-game input/movement state.
    std::uint32_t  m_keyMask      = 0;
    float          m_localX       = 0;
    float          m_localZ       = 0;
    float          m_cameraYaw    = 0;
    float          m_cameraDistance = 6.0f;
    float          m_worldLimitX = kWorldLimit;
    float          m_worldLimitZ = kWorldLimit;
    std::function<bool(float, float, float)> m_collisionQuery;
    MapChangeTargetResolver m_mapChangeTargetResolver;
    bool           m_moving       = false;
    std::uint64_t  m_lastTickMs   = 0;
    std::uint64_t  m_lastMoveSendMs = 0;
    std::uint64_t  m_lastAttackMs = 0;
    std::uint64_t  m_attackFlashMs = 0;  // timestamp of last attack for visual flash
    std::int32_t   m_lastMouseX   = 0;
    std::int32_t   m_lastMouseY   = 0;
    bool           m_cameraDrag   = false;

    // Chat state (in-game).
    bool                 m_chatOpen   = false;
    std::string          m_chatBuffer;
    std::vector<std::string> m_chatLines;
    std::uint32_t m_partyId = 0;
    std::uint8_t m_partyMemberCount = 0;
    std::uint32_t m_pendingPartyInviteId = 0;
    std::uint32_t m_guildId = 0;
    std::uint8_t m_guildMemberCount = 0;
    std::uint32_t m_pendingGuildInviteId = 0;
    std::uint32_t m_pendingFriendInviteId = 0;
    bool                 m_inventoryOpen = false;
    std::size_t          m_inventoryTab = 0;
    ClientUiRuntime      m_uiRuntime;

    // NPC shop state.
    bool                 m_shopOpen  = false;
    std::uint32_t        m_shopNpcId = 0;
    std::uint32_t        m_lastBuyItemId = 0;
    std::uint32_t        m_pendingBuyItemId = 0;
    std::uint64_t        m_pendingBuySinceMs = 0;
    std::vector<ShopItem> m_shopItems;
    bool                 m_questOpen = false;
    bool                 m_characterOpen = false;
    bool                 m_mapOpen = false;
    std::uint16_t        m_lastUiMapNum = 0xffffu;
    bool                 m_friendOpen = false;
    bool                 m_guildOpen = false;
    bool                 m_optionOpen = false;
    // Zero means no quest is selected; a real quest id is installed only
    // after the live quest list is received.
    std::uint16_t        m_questId = 0;
    std::string          m_questStatus = "Not accepted";
    mxh::compat::QuestStringCatalog m_questCatalog;
    std::vector<const mxh::compat::QuestStringEntry*> m_mainQuests;
    std::size_t          m_questSelection = 0;
    std::optional<std::size_t> m_inventoryDragSource;
    bool m_pendingInventoryMove = false;
    std::uint64_t m_pendingInventoryMoveSinceMs = 0;
    std::vector<EffectEvent> m_effectEvents;
    EffectRuntime m_effectRuntime;
    struct PendingSkillEffect {
        std::uint32_t skill_id = 0;
        std::uint32_t target_object_id = 0;
        std::uint64_t timestamp_ms = 0;
        std::uint32_t source_object_id = 0;
    };
    std::vector<PendingSkillEffect> m_pendingSkillEffects;
    std::future<std::pair<mxh::game::EffectCatalog, std::string>> m_effectCatalogLoad;
    bool m_effectCatalogLoading = false;
    bool m_smokeExitRequested = false;
    // Frame counter that begins incrementing from the first Process()
    // tick after GameInAck.  Combined with m_smokeSettleRequired this
    // gives smoke_exit_ready() a deterministic "world has rendered for
    // N frames" gate instead of a monster-count instant-true.
    std::uint32_t m_smokeSettleFrames = 0;
    // Captured at GameInAck from MXH_GUI_SMOKE_SETTLE_FRAMES (or the
    // build default 90).  See smoke_exit_ready() for the rationale.
    std::uint32_t m_smokeSettleRequired = 90;
    // Phase 0 §6.4: opt-in flag for protocol_burst_test to call
    // on_message() directly.  When false, HandleMessageForTest is a no-op
    // so a missing test setup cannot accidentally exercise the dispatch
    // path.  Defaults to false; tests must call SetDispatchForTest(true).
    bool m_dispatchEnabledForTest = false;
    mxh::game::SkillManager m_skillManager;
    float m_effectTickPerFrameMs = 1000.0f / 30.0f;
    std::vector<RuntimeEffectEvent> m_runtimeEffectEvents;
};

} // namespace mxh::client
