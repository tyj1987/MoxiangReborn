#include <gtest/gtest.h>

#include "mxh/server/object.hpp"
#include "mxh/server/object_event.hpp"
#include "mxh/proto/protocol.hpp"

#include <cstring>

using namespace mxh::server;

namespace {

Object make_filled_object() {
    Object obj;
    BaseObjectInfo info{};
    info.dw_object_id = 0xAABBCCDDu;
    info.dw_user_id   = 0x11223344u;
    std::strncpy(info.object_name, "HeroName", MAX_NAME_LENGTH);
    info.battle_id    = 42u;
    info.battle_team  = 1u;
    info.object_state = 0x55u;
    info.single_special_state[static_cast<std::size_t>(SingleSpecialState::Hide)] = true;
    EXPECT_TRUE(obj.init(ObjectKind::Player, 7u, &info));
    return obj;
}

}  // namespace

// -----------------------------------------------------------------------------
// ObjectKind / SpecialState enum matches legacy eObjectKind / eSpecialState.
// -----------------------------------------------------------------------------
TEST(ObjectTest, ObjectKindEnumMirrorsLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Player),            1u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Npc),               2u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Item),              4u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Tactic),            8u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::SkillObject),       16u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Monster),           32u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::BossMonster),       33u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::SpecialMonster),    34u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::FieldBossMonster),  35u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::FieldSubMonster),   36u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::TogetherPlayMonster),37u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::MapObject),         64u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::CastleGate),        65u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Pet),               128u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectKind::Titan),             129u);
}

TEST(ObjectTest, SpecialStateEnumMirrorsLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(SpecialState::Stun),              0u);
    EXPECT_EQ(static_cast<std::uint8_t>(SpecialState::AmplifiedPowerPhy), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(SpecialState::AmplifiedPowerAtt), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(SpecialState::DetectItem),        3u);
    EXPECT_EQ(static_cast<std::uint8_t>(SpecialState::Max),               4u);
    EXPECT_EQ(SINGLE_SPECIAL_STATE_MAX, 4u);
}

// -----------------------------------------------------------------------------
// Init / Release / SetInited lifecycle.
// -----------------------------------------------------------------------------
TEST(ObjectTest, InitCopiesBaseObjectInfoWhenProvided) {
    Object obj;
    BaseObjectInfo info{};
    info.dw_object_id = 100u;
    info.dw_user_id   = 200u;
    std::strncpy(info.object_name, "TestHero", MAX_NAME_LENGTH);
    info.battle_id    = 7u;
    info.battle_team  = 2u;
    info.object_state = 0x77u;

    EXPECT_TRUE(obj.init(ObjectKind::Monster, 9u, &info));
    EXPECT_EQ(obj.get_object_kind(), ObjectKind::Monster);
    EXPECT_EQ(obj.get_agent_num(),   9u);
    EXPECT_EQ(obj.get_id(),          100u);
    EXPECT_EQ(obj.get_user_id(),     200u);
    EXPECT_EQ(obj.get_battle_id(),   7u);
    EXPECT_EQ(obj.get_battle_team(), 2u);
    EXPECT_EQ(obj.get_state(),       0x77u);
    EXPECT_FALSE(obj.get_inited());
}

TEST(ObjectTest, InitZeroesBaseObjectInfoWhenNull) {
    Object obj;
    EXPECT_TRUE(obj.init(ObjectKind::Npc, 0u, nullptr));
    EXPECT_EQ(obj.get_object_kind(), ObjectKind::Npc);
    EXPECT_EQ(obj.get_id(),          0u);
    EXPECT_EQ(obj.get_user_id(),     0u);
    EXPECT_EQ(obj.get_battle_id(),   0u);
    EXPECT_EQ(obj.get_battle_team(), 0u);
}

TEST(ObjectTest, ReleaseClearsAllFields) {
    Object obj = make_filled_object();
    obj.set_inited();
    EXPECT_TRUE(obj.get_inited());

    obj.release();
    EXPECT_FALSE(obj.get_inited());
    EXPECT_EQ(obj.get_id(),          0u);
    EXPECT_EQ(obj.get_user_id(),     0u);
    EXPECT_EQ(obj.get_agent_num(),   0u);
    EXPECT_EQ(obj.get_battle_id(),   0u);
    EXPECT_EQ(obj.get_battle_team(), 0u);
}

TEST(ObjectTest, SetInitedSetNotInitedToggle) {
    Object obj;
    obj.set_inited();
    EXPECT_TRUE(obj.get_inited());
    obj.set_not_inited();
    EXPECT_FALSE(obj.get_inited());
    obj.set_inited();
    EXPECT_TRUE(obj.get_inited());
}

// -----------------------------------------------------------------------------
// Accessors (Get/Set ObjectKind, Battle, etc.).
// -----------------------------------------------------------------------------
TEST(ObjectTest, SetObjectKindUpdatesKind) {
    Object obj;
    obj.set_object_kind(ObjectKind::Pet);
    EXPECT_EQ(obj.get_object_kind(), ObjectKind::Pet);
    obj.set_object_kind(ObjectKind::Titan);
    EXPECT_EQ(obj.get_object_kind(), ObjectKind::Titan);
}

TEST(ObjectTest, SetBattleTeamAndBattleIdAffectBaseObjectInfo) {
    Object obj;
    obj.set_battle_team(3u);
    obj.set_battle_id(12345u);
    EXPECT_EQ(obj.get_battle_team(), 3u);
    EXPECT_EQ(obj.get_battle_id(),   12345u);
}

// -----------------------------------------------------------------------------
// SetState: BeforeState + state_start_time + b_end_state reset.
// -----------------------------------------------------------------------------
TEST(ObjectTest, SetStateSavesBeforeStateAndResetsEndFlag) {
    Object obj = make_filled_object();
    obj.mutable_base_object_info().object_state = 0xAAu;

    obj.set_state(0xBBu, 12345u);
    EXPECT_EQ(obj.get_state(), 0xBBu);
    EXPECT_EQ(obj.get_state_info().before_state, 0xAAu);
    EXPECT_EQ(obj.get_state_info().state_start_time, 12345u);
    EXPECT_EQ(obj.get_state_info().b_end_state, 0u);
}

TEST(ObjectTest, GetBaseObjectInfoCopyReturnsSnapshot) {
    Object obj = make_filled_object();
    BaseObjectInfo copy{};
    obj.get_base_object_info_copy(&copy);
    EXPECT_EQ(copy.dw_object_id, 0xAABBCCDDu);
    EXPECT_EQ(copy.dw_user_id,   0x11223344u);
    EXPECT_STREQ(copy.object_name, "HeroName");
    EXPECT_EQ(copy.battle_id,    42u);
    EXPECT_EQ(copy.battle_team,  1u);
}

// -----------------------------------------------------------------------------
// SetRemoveMsg wire-format: 12 bytes, category=UserConn, protocol=ObjectRemove.
// -----------------------------------------------------------------------------
TEST(ObjectTest, SetRemoveMsgWrites12ByteHeader) {
    Object obj = make_filled_object();
    std::uint8_t buf[16] = {};
    std::size_t n = obj.set_remove_msg(buf, sizeof(buf), 0xDEADBEEFu);
    EXPECT_EQ(n, 12u);

    EXPECT_EQ(buf[0], static_cast<std::uint8_t>(mxh::proto::Category::UserConn));
    EXPECT_EQ(buf[1], static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ObjectRemove));

    std::uint32_t receiver = 0;
    std::uint32_t data     = 0;
    std::memcpy(&receiver, buf + 2, sizeof(receiver));
    std::memcpy(&data,     buf + 6, sizeof(data));
    EXPECT_EQ(receiver, 0xDEADBEEFu);
    EXPECT_EQ(data,     0xAABBCCDDu);
}

TEST(ObjectTest, SetRemoveMsgRespectsBufferSize) {
    Object obj = make_filled_object();
    std::uint8_t small[8] = {};
    EXPECT_EQ(obj.set_remove_msg(small, sizeof(small), 1u), 0u);
    EXPECT_EQ(obj.set_remove_msg(nullptr, 16, 1u), 0u);
}

// -----------------------------------------------------------------------------
// GetSendMoveInfo wire-format: 17 bytes (always writes direction).
// -----------------------------------------------------------------------------
TEST(ObjectTest, GetSendMoveInfoWrites17BytesDefault) {
    Object obj;
    BaseObjectInfo info{};
    info.dw_object_id = 1u;
    EXPECT_TRUE(obj.init(ObjectKind::Player, 0u, &info));
    obj.mutable_move_info().cur_position_x   = 1234;
    obj.mutable_move_info().cur_position_z   = 5678;
    obj.mutable_move_info().move_mode        = true;
    obj.mutable_move_info().kyung_gong_idx   = 7u;

    std::uint8_t buf[32] = {};
    // Wire layout: uint16(2)+uint16(2)+uint8(1)+uint16(2)+uint16(2)+int32(4)+int32(4) = 17
    // The default path always writes direction as zeros, so size is 17 regardless of b_set_dir.
    std::size_t n = obj.get_send_move_info(buf, sizeof(buf));
    EXPECT_EQ(n, 17u);
}

TEST(ObjectTest, GetSendMoveInfoWiresMoveModeKyungGongIdx) {
    Object obj;
    BaseObjectInfo info{};
    info.dw_object_id = 1u;
    EXPECT_TRUE(obj.init(ObjectKind::Player, 0u, &info));
    obj.mutable_move_info().cur_position_x   = 0x1234;
    obj.mutable_move_info().cur_position_z   = 0x5678;
    obj.mutable_move_info().move_mode        = true;
    obj.mutable_move_info().kyung_gong_idx   = 9u;

    std::uint8_t buf[32] = {};
    (void)obj.get_send_move_info(buf, sizeof(buf));

    std::uint16_t wx = 0;
    std::uint16_t wz = 0;
    std::memcpy(&wx, buf,     sizeof(wx));
    std::memcpy(&wz, buf + 2, sizeof(wz));
    EXPECT_EQ(wx, 0x1234u);
    EXPECT_EQ(wz, 0x5678u);
    EXPECT_EQ(buf[4], 1u);
    std::uint16_t kg = 0;
    std::memcpy(&kg, buf + 5, sizeof(kg));
    EXPECT_EQ(kg, 9u);
}

// -----------------------------------------------------------------------------
// Die() dispatches ObjectEventCode::Die via sink.
// -----------------------------------------------------------------------------
namespace {

struct DieRecorder {
    int levelup_count = 0;
    int die_count     = 0;
    int life_count    = 0;
    Object* last_obj  = nullptr;
};

bool record_levelup(Object* obj, void* user) {
    auto* r = static_cast<DieRecorder*>(user);
    r->levelup_count++;
    r->last_obj = obj;
    return true;
}

bool record_die(Object* obj, void* user) {
    auto* r = static_cast<DieRecorder*>(user);
    r->die_count++;
    r->last_obj = obj;
    return true;
}

bool record_life(Object* obj, void* user) {
    auto* r = static_cast<DieRecorder*>(user);
    r->life_count++;
    r->last_obj = obj;
    return true;
}

}  // namespace

TEST(ObjectTest, DieDispatchesObjectEvent) {
    Object obj = make_filled_object();
    DieRecorder rec{};
    set_object_event_sink({record_levelup, record_die, record_life, &rec});

    obj.die(nullptr);
    EXPECT_EQ(rec.die_count,     1);
    EXPECT_EQ(rec.levelup_count, 0);
    EXPECT_EQ(rec.life_count,    0);
    EXPECT_EQ(rec.last_obj,      &obj);
}

// -----------------------------------------------------------------------------
// GridPosition helper.
// -----------------------------------------------------------------------------
TEST(ObjectTest, GridPositionShiftPreservesLast) {
    Object obj;
    obj.set_grid_position(100u, 200u);
    EXPECT_EQ(obj.get_grid_position().x,      100u);
    EXPECT_EQ(obj.get_grid_position().z,      200u);
    EXPECT_EQ(obj.get_grid_position().last_x, 0u);
    EXPECT_EQ(obj.get_grid_position().last_z, 0u);

    obj.set_grid_position(150u, 250u);
    EXPECT_EQ(obj.get_grid_position().x,      150u);
    EXPECT_EQ(obj.get_grid_position().z,      250u);
    EXPECT_EQ(obj.get_grid_position().last_x, 100u);
    EXPECT_EQ(obj.get_grid_position().last_z, 200u);
}
