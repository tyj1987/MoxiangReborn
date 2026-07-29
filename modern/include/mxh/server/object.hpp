#pragma once

// object.hpp - Phase 6.2 Object 1:1 port (subset).
//
// Source-of-truth: legacy [Server]Map/Object.h + .cpp.
//
// Small structural port: wire-format structs (BaseObjectInfo /
// StateInfo / MoveInfo / GridPosition) + lifecycle entry points
// (Init / Release / Die) + SetRemoveMsg.  The Calc* methods that
// operate over CStatus / CSpecialState / CCharMove are deferred.

#include <array>
#include <cstdint>
#include <cstring>

#include "mxh/proto/protocol.hpp"

namespace mxh::server {

inline constexpr std::size_t MAX_NAME_LENGTH            = 16u;
inline constexpr std::size_t MAX_CHARTARGETPOSBUF_SIZE  = 15u;
inline constexpr std::size_t SINGLE_SPECIAL_STATE_MAX   = 4u;

enum class ObjectKind : std::uint8_t {
    Player             = 1,
    Npc                = 2,
    Item               = 4,
    Tactic             = 8,
    SkillObject        = 16,
    Monster            = 32,
    BossMonster        = 33,
    SpecialMonster     = 34,
    FieldBossMonster   = 35,
    FieldSubMonster    = 36,
    TogetherPlayMonster= 37,
    Mining             = 38,
    Collection         = 39,
    Hunt               = 40,
    MapObject          = 64,
    CastleGate         = 65,
    Pet                = 128,
    Titan              = 129,
};

enum class SpecialState : std::uint8_t {
    Stun                = 0,
    AmplifiedPowerPhy   = 1,
    AmplifiedPowerAtt   = 2,
    DetectItem          = 3,
    Max                 = 4,
};

enum class SingleSpecialState : std::uint8_t {
    None       = 0,
    Hide       = 1,
    Detect     = 2,
    DetectItem = 3,
    Max        = 4,
};

#pragma pack(push, 1)
struct GridPosition {
    std::uint16_t x      = 0;
    std::uint16_t z      = 0;
    std::uint16_t last_x = 0;
    std::uint16_t last_z = 0;
};

struct BaseObjectInfo {
    std::uint32_t dw_object_id = 0;
    std::uint32_t dw_user_id   = 0;
    char          object_name[MAX_NAME_LENGTH + 1] = {};
    std::uint32_t battle_id    = 0;
    std::uint8_t  battle_team  = 0;
    std::uint8_t  object_state = 0;
    bool          single_special_state[SINGLE_SPECIAL_STATE_MAX] = {};

    BaseObjectInfo() {
        battle_id = 0;
        battle_team = 0;
        std::memset(single_special_state, 0, sizeof(single_special_state));
    }
};

struct StateInfo {
    std::int32_t  state_start_motion    = 0;
    std::int32_t  state_ing_motion      = 0;
    std::int32_t  state_end_motion      = 0;
    std::uint32_t state_end_motion_time = 0;
    std::uint32_t state_end_time        = 0;
    std::uint32_t state_start_time      = 0;
    std::uint8_t  before_state          = 0;
    std::uint8_t  b_end_state           = 0;
};

struct MoveInfo {
    std::int32_t  cur_position_x    = 0;
    std::int32_t  cur_position_y    = 0;
    std::int32_t  cur_position_z    = 0;
    std::uint8_t  cur_target_idx    = 0;
    std::uint8_t  max_target_idx    = 0;
    bool          b_moving          = false;
    bool          move_mode         = false;
    std::uint16_t kyung_gong_idx    = 0;
    std::uint16_t ability_kyung_gong_level = 0;
    std::int32_t  move_direction_x  = 0;
    std::int32_t  move_direction_z  = 0;

    void clear() {
        cur_position_x = cur_position_y = cur_position_z = 0;
        cur_target_idx = max_target_idx = 0;
        b_moving = move_mode = false;
        kyung_gong_idx = ability_kyung_gong_level = 0;
        move_direction_x = move_direction_z = 0;
    }
};
#pragma pack(pop)

class Object {
public:
    Object() = default;
    virtual ~Object() = default;
    Object(const Object&) = default;
    Object& operator=(const Object&) = default;

    bool init(ObjectKind kind, std::uint32_t agent_num, const BaseObjectInfo* info);
    void release();

    void set_inited();
    void set_not_inited();
    bool get_inited() const { return inited_; }

    ObjectKind get_object_kind() const { return object_kind_; }
    void set_object_kind(ObjectKind k) { object_kind_ = k; }

    std::uint32_t get_agent_num() const { return agent_num_; }
    std::uint32_t get_id()         const { return base_object_info_.dw_object_id; }
    std::uint32_t get_user_id()    const { return base_object_info_.dw_user_id; }

    std::uint8_t  get_battle_team() const { return base_object_info_.battle_team; }
    void          set_battle_team(std::uint8_t team) { base_object_info_.battle_team = team; }

    std::uint32_t get_battle_id() const { return base_object_info_.battle_id; }
    void          set_battle_id(std::uint32_t id) { base_object_info_.battle_id = id; }

    std::uint8_t  get_state() const { return base_object_info_.object_state; }

    const BaseObjectInfo& get_base_object_info() const { return base_object_info_; }
    BaseObjectInfo&       mutable_base_object_info() { return base_object_info_; }

    const StateInfo& get_state_info() const { return state_info_; }
    StateInfo&       mutable_state_info() { return state_info_; }

    const MoveInfo& get_move_info() const { return move_info_; }
    MoveInfo&       mutable_move_info() { return move_info_; }

    const GridPosition& get_grid_position() const { return grid_position_; }
    GridPosition&       mutable_grid_position() { return grid_position_; }

    void set_grid_position(std::uint16_t x, std::uint16_t z) {
        grid_position_.last_x = grid_position_.x;
        grid_position_.last_z = grid_position_.z;
        grid_position_.x = x;
        grid_position_.z = z;
    }

    void set_state(std::uint8_t new_state, std::uint32_t cur_time);

    void get_base_object_info_copy(BaseObjectInfo* out) const;

    std::size_t set_remove_msg(void* buf, std::size_t buf_len,
                               std::uint32_t dw_receiver_id) const;

    std::size_t get_send_move_info(void* buf, std::size_t buf_len,
                                   bool b_set_dir = false) const;

    void die(Object* p_attacker);

protected:
    virtual void do_set_inited() {}
    virtual void do_die(Object* /*p_attacker*/) {}
    virtual void do_real_damage(Object* /*p_attacker*/,
                                std::uint32_t /*phy_damage*/,
                                std::uint32_t /*attr_damage*/) {}

    struct SendMoveInfoWire {
        std::uint16_t cur_pos_x;
        std::uint16_t cur_pos_z;
        std::uint8_t  move_mode;
        std::uint16_t kyung_gong_idx;
        std::uint16_t ability_kyung_gong_level;
        std::int32_t  move_direction_x;
        std::int32_t  move_direction_y;
    };

private:
    ObjectKind                object_kind_       = ObjectKind::Player;
    std::uint32_t             agent_num_         = 0;
    BaseObjectInfo            base_object_info_{};
    StateInfo                 state_info_{};
    MoveInfo                  move_info_{};
    GridPosition              grid_position_{};
    bool                      inited_            = false;
    bool                      grid_inited_       = false;
};

}  // namespace mxh::server
