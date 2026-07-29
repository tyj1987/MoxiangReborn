// object.cpp - Phase 6.2 Object 1:1 port implementations.
//
// Mirrors legacy [Server]Map/Object.cpp init / release / set_state
// and the SetRemoveMsg / Die entry points.  Special state machines
// (CStunState / CAmplifiedPower*) are deferred; here we only wire
// the structural shape so dependent modules (Player / Monster /
// Pet / Titan) can build against it.

#include "mxh/server/object.hpp"

#include "mxh/server/object_event.hpp"
#include "mxh/server/object_state_manager.hpp"

#include <cstring>

namespace mxh::server {

namespace {

void init_object_state(Object* obj) {
    if (obj == nullptr) return;
    start_object_state(ObjectState::None, ObjectState::None);
}

bool dispatch_event_levelup(Object* obj, void* /*user*/) {
    return obj != nullptr;
}

bool dispatch_event_die(Object* obj, void* /*user*/) {
    return obj != nullptr;
}

bool dispatch_event_life_recover(Object* obj, void* /*user*/) {
    return obj != nullptr;
}

}  // namespace

bool Object::init(ObjectKind kind, std::uint32_t agent_num,
                  const BaseObjectInfo* info) {
    object_kind_       = kind;
    agent_num_         = agent_num;

    if (info != nullptr) {
        base_object_info_ = *info;
    } else {
        base_object_info_ = BaseObjectInfo{};
    }

    move_info_.clear();
    state_info_        = StateInfo{};
    grid_position_     = GridPosition{};
    inited_            = false;
    grid_inited_       = false;

    init_object_state(this);
    return true;
}

void Object::release() {
    object_kind_       = ObjectKind::Player;
    agent_num_         = 0;
    base_object_info_  = BaseObjectInfo{};
    state_info_        = StateInfo{};
    move_info_.clear();
    grid_position_     = GridPosition{};
    inited_            = false;
    grid_inited_       = false;
}

void Object::set_inited() {
    do_set_inited();
    inited_ = true;
}

void Object::set_not_inited() {
    inited_ = false;
}

void Object::set_state(std::uint8_t new_state, std::uint32_t cur_time) {
    state_info_.before_state      = base_object_info_.object_state;
    base_object_info_.object_state = new_state;
    state_info_.state_start_time  = cur_time;
    state_info_.b_end_state       = 0;
}

void Object::get_base_object_info_copy(BaseObjectInfo* out) const {
    if (out == nullptr) return;
    *out = base_object_info_;
}

std::size_t Object::set_remove_msg(void* buf, std::size_t buf_len,
                                   std::uint32_t dw_receiver_id) const {
    // Mirror legacy MSG_DWORD layout (MP_USERCONN_OBJECT_REMOVE):
    //   [category:u8][protocol:u8][dwObjectID:u32][dwData:u32]
    constexpr std::size_t kMsgDwordSize = 12;
    if (buf == nullptr || buf_len < kMsgDwordSize) {
        return 0;
    }
    auto* p = static_cast<std::uint8_t*>(buf);
    p[0] = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    p[1] = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ObjectRemove);
    std::uint32_t receiver = dw_receiver_id;
    std::uint32_t data     = base_object_info_.dw_object_id;
    std::memcpy(p + 2,  &receiver, sizeof(receiver));
    std::memcpy(p + 6,  &data,     sizeof(data));
    return kMsgDwordSize;
}

std::size_t Object::get_send_move_info(void* buf, std::size_t buf_len,
                                       bool b_set_dir) const {
    constexpr std::size_t kWireSize =
        sizeof(std::uint16_t) + sizeof(std::uint16_t)
        + sizeof(std::uint8_t)
        + sizeof(std::uint16_t) + sizeof(std::uint16_t)
        + sizeof(std::int32_t)  + sizeof(std::int32_t);
    if (buf == nullptr || buf_len < kWireSize) {
        return 0;
    }
    auto* p = static_cast<std::uint8_t*>(buf);
    std::uint16_t wx = static_cast<std::uint16_t>(move_info_.cur_position_x);
    std::uint16_t wz = static_cast<std::uint16_t>(move_info_.cur_position_z);
    std::uint8_t  mm = move_info_.move_mode ? 1u : 0u;
    std::memcpy(p,      &wx, sizeof(wx));                 p += sizeof(wx);
    std::memcpy(p,      &wz, sizeof(wz));                 p += sizeof(wz);
    std::memcpy(p,      &mm, sizeof(mm));                 p += sizeof(mm);
    std::memcpy(p,      &move_info_.kyung_gong_idx,
                 sizeof(move_info_.kyung_gong_idx));       p += sizeof(move_info_.kyung_gong_idx);
    std::memcpy(p,      &move_info_.ability_kyung_gong_level,
                 sizeof(move_info_.ability_kyung_gong_level)); p += sizeof(move_info_.ability_kyung_gong_level);
    if (b_set_dir) {
        std::memcpy(p,  &move_info_.move_direction_x,
                     sizeof(move_info_.move_direction_x));   p += sizeof(move_info_.move_direction_x);
        std::memcpy(p,  &move_info_.move_direction_z,
                     sizeof(move_info_.move_direction_z));
    } else {
        std::int32_t zero = 0;
        std::memcpy(p, &zero, sizeof(zero)); p += sizeof(zero);
        std::memcpy(p, &zero, sizeof(zero));
    }
    return kWireSize;
}

void Object::die(Object* p_attacker) {
    do_die(p_attacker);
    object_event_dispatch(ObjectEventCode::Die, this);
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int object_translation_unit_anchor = 0;
}
