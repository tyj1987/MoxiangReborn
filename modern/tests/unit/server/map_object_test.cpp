#include <gtest/gtest.h>

#include "mxh/server/map_object.hpp"
#include "mxh/server/object.hpp"
#include "mxh/server/object_event.hpp"

#include <cstring>

using namespace mxh::server;

namespace {

MapObjectInfo make_filled_info() {
    MapObjectInfo info{};
    info.life        = 100u;
    info.max_life    = 200u;
    info.shield      = 50u;
    info.max_shield  = 80u;
    info.phy_defence = 30u;
    info.radius      = 12.5f;
    info.attr_regist.set_element_val(static_cast<std::uint8_t>(Element::Fire),  0.10f);
    info.attr_regist.set_element_val(static_cast<std::uint8_t>(Element::Water), 0.20f);
    info.attr_regist.set_element_val(static_cast<std::uint8_t>(Element::Tree),  0.30f);
    info.attr_regist.set_element_val(static_cast<std::uint8_t>(Element::Iron),  0.40f);
    info.attr_regist.set_element_val(static_cast<std::uint8_t>(Element::Earth), 0.50f);
    return info;
}

}  // namespace

TEST(MapObjectTest, ElementEnumMirrorsLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(Element::None),    0u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::Fire),    1u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::Earth),   2u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::Iron),    3u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::Water),   4u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::Tree),    5u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::Max),     5u);
    EXPECT_EQ(static_cast<std::uint8_t>(Element::NoAttr),  6u);
    EXPECT_EQ(ATTR_VAL_COUNT, 6u);
}

TEST(MapObjectTest, AttributeValGetReturnsZeroForNone) {
    AttributeRegist r{};
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::None)), 0.0f);
}

TEST(MapObjectTest, AttributeValGetReturnsZeroForNoAttr) {
    AttributeRegist r{};
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::NoAttr)), 0.0f);
}

TEST(MapObjectTest, AttributeValSetAndGetRoundtrip) {
    AttributeRegist r{};
    r.set_element_val(static_cast<std::uint8_t>(Element::Fire),  0.5f);
    r.set_element_val(static_cast<std::uint8_t>(Element::Water), 0.6f);
    r.set_element_val(static_cast<std::uint8_t>(Element::Tree),  0.7f);
    r.set_element_val(static_cast<std::uint8_t>(Element::Iron),  0.8f);
    r.set_element_val(static_cast<std::uint8_t>(Element::Earth), 0.9f);
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::Fire)),  0.5f);
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::Water)), 0.6f);
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::Tree)),  0.7f);
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::Iron)),  0.8f);
    EXPECT_EQ(r.get_element_val(static_cast<std::uint8_t>(Element::Earth)), 0.9f);
}

TEST(MapObjectTest, AttributeValAddAccumulates) {
    AttributeRegist a{};
    a.set_element_val(static_cast<std::uint8_t>(Element::Fire), 1.0f);
    AttributeRegist b{};
    b.set_element_val(static_cast<std::uint8_t>(Element::Fire), 0.5f);
    a.add(b);
    EXPECT_EQ(a.get_element_val(static_cast<std::uint8_t>(Element::Fire)), 1.5f);
}

TEST(MapObjectTest, MapObjectInfoStructHasExpectedLayout) {
    MapObjectInfo info = make_filled_info();
    EXPECT_EQ(info.life,        100u);
    EXPECT_EQ(info.max_life,    200u);
    EXPECT_EQ(info.shield,      50u);
    EXPECT_EQ(info.max_shield,  80u);
    EXPECT_EQ(info.phy_defence, 30u);
    EXPECT_FLOAT_EQ(info.radius, 12.5f);
}

TEST(MapObjectTest, MapObjectInfoStructIsPacked) {
    EXPECT_EQ(sizeof(MapObjectInfo), 6u * 4u + 6u * sizeof(float));
    EXPECT_EQ(sizeof(MapObjectInfo), 24u + 24u);
}

TEST(MapObjectTest, InitMapObjectCopiesInfoAndZeroesLevel) {
    MapObject obj;
    MapObjectInfo info = make_filled_info();
    obj.init_map_object(&info);
    obj.set_inited();

    EXPECT_TRUE(obj.get_inited());
    EXPECT_EQ(obj.get_level(),   0u);
    EXPECT_EQ(obj.get_life(),    100u);
    EXPECT_EQ(obj.get_shield(),  50u);
    EXPECT_EQ(obj.get_radius(),  12.5f);
    EXPECT_EQ(obj.do_get_max_life(),    200u);
    EXPECT_EQ(obj.do_get_max_shield(),  80u);
    EXPECT_EQ(obj.do_get_phy_defense(), 30u);
}

TEST(MapObjectTest, InitMapObjectNullPtrZeroesFields) {
    MapObject obj;
    obj.init_map_object(nullptr);
    EXPECT_EQ(obj.get_life(),   0u);
    EXPECT_EQ(obj.get_shield(), 0u);
    EXPECT_EQ(obj.get_radius(), 0.0f);
}

TEST(MapObjectTest, SetLevelUpdatesLevel) {
    MapObject obj;
    obj.init_map_object(nullptr);
    obj.set_level(7u);
    EXPECT_EQ(obj.get_level(), 7u);
    obj.set_level(42u);
    EXPECT_EQ(obj.get_level(), 42u);
}

TEST(MapObjectTest, SetLifeSetShieldUpdateFields) {
    MapObject obj;
    obj.init_map_object(nullptr);
    obj.set_life(99u);
    obj.set_shield(33u);
    EXPECT_EQ(obj.get_life(),   99u);
    EXPECT_EQ(obj.get_shield(), 33u);
}

TEST(MapObjectTest, DoGetAttDefenseReturnsElementRegist) {
    MapObject obj;
    MapObjectInfo info = make_filled_info();
    obj.init_map_object(&info);

    EXPECT_FLOAT_EQ(
        obj.do_get_att_defense(static_cast<std::uint16_t>(Element::Fire)),
        0.10f);
    EXPECT_FLOAT_EQ(
        obj.do_get_att_defense(static_cast<std::uint16_t>(Element::Water)),
        0.20f);
    EXPECT_FLOAT_EQ(
        obj.do_get_att_defense(static_cast<std::uint16_t>(Element::Tree)),
        0.30f);
    EXPECT_FLOAT_EQ(
        obj.do_get_att_defense(static_cast<std::uint16_t>(Element::Iron)),
        0.40f);
    EXPECT_FLOAT_EQ(
        obj.do_get_att_defense(static_cast<std::uint16_t>(Element::Earth)),
        0.50f);
}

namespace {

int g_die_count = 0;

bool count_die(Object* /*o*/, void* user) {
    (void)user;
    g_die_count++;
    return true;
}

}  // namespace

TEST(MapObjectTest, DoDieOnCastleGateKindDoesNotCrash) {
    MapObject obj;
    BaseObjectInfo base{};
    base.dw_object_id = 7u;
    EXPECT_TRUE(obj.init(ObjectKind::CastleGate, 0u, &base));
    obj.set_inited();
    obj.init_map_object(nullptr);

    g_die_count = 0;
    set_object_event_sink({nullptr, count_die, nullptr, nullptr});

    obj.die(nullptr);
    EXPECT_EQ(g_die_count, 1);
    EXPECT_EQ(obj.get_inited(), true);
}

TEST(MapObjectTest, ObjectEventForwardDeclResolvesCorrectly) {
    MapObject obj;
    BaseObjectInfo base{};
    base.dw_object_id = 0xDEADBEEFu;
    EXPECT_TRUE(obj.init(ObjectKind::MapObject, 1u, &base));
    EXPECT_EQ(obj.get_id(), 0xDEADBEEFu);
    EXPECT_EQ(obj.get_object_kind(), ObjectKind::MapObject);
}
