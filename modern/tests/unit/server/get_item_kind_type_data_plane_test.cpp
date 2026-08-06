
// get_item_kind_type_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::get_item_kind_type (D4.134).
// Augments the legacy 3-test get_item_kind_type_test.cpp with deeper coverage of:
//   - Hit path: out_kind = info->ItemKind, out_type = info->ItemType.
//   - Miss path: out_kind = 0, out_type = 0 (both cleared).
//   - Out-param overwriting semantics (pre-existing garbage discarded).
//   - Boundary values (0, max uint16).
//   - Multi-call sequencing.
//   - ItemInfo with other populated fields does not affect output.
//
// 1:1 invariants (locked):
//   - On non-null ItemInfo pointer: out_kind = info->ItemKind, out_type = info->ItemType.
//   - On null ItemInfo pointer: out_kind = 0, out_type = 0.
//   - Out params are pure out params (caller initial values overwritten).
//   - Other ItemInfo fields (besides ItemKind / ItemType) do not
//     influence the output.

#pragma once

#include "mxh/server/get_item_kind_type.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

namespace {

using mxh::server::get_item_kind_type;
using mxh::game::ItemInfo;

ItemInfo make_info(std::uint16_t kind, std::uint16_t type) {
    ItemInfo info{};
    info.ItemKind = kind;
    info.ItemType = type;
    return info;
}

}  // namespace


// ===========================================================================
// Hit path: non-null ItemInfo pointer
// ===========================================================================

TEST(GetItemKindTypeDataPlane, WritesBothFieldsOnHit) {
    ItemInfo info = make_info(259, 11);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 259u);
    EXPECT_EQ(type, 11u);
}

TEST(GetItemKindTypeDataPlane, WritesZerosOnNull) {
    std::uint16_t kind = 99, type = 88;
    get_item_kind_type(nullptr, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 0u);
}

TEST(GetItemKindTypeDataPlane, KindAndTypeCanBeIndependent) {
    ItemInfo info = make_info(512, 0);  // youngyak, no type
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 512u);
    EXPECT_EQ(type, 0u);
    info = make_info(0, 10);  // no kind, has type
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 10u);
}

TEST(GetItemKindTypeDataPlane, BothZeroIsValid) {
    ItemInfo info = make_info(0, 0);
    std::uint16_t kind = 0xFFFF, type = 0xFFFF;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 0u);
}

TEST(GetItemKindTypeDataPlane, BothMaxIsValid) {
    ItemInfo info = make_info(0xFFFFu, 0xFFFFu);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 0xFFFFu);
    EXPECT_EQ(type, 0xFFFFu);
}



// ===========================================================================
// Out-param overwrite semantics
// ===========================================================================

TEST(GetItemKindTypeDataPlane, OverwritesInitialOutParamsHit) {
    ItemInfo info = make_info(100, 200);
    std::uint16_t kind = 0xAAAAu, type = 0xBBBBu;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 100u);
    EXPECT_EQ(type, 200u);
}

TEST(GetItemKindTypeDataPlane, OverwritesInitialOutParamsNull) {
    std::uint16_t kind = 0xAAAAu, type = 0xBBBBu;
    get_item_kind_type(nullptr, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 0u);
}

TEST(GetItemKindTypeDataPlane, SameOutParamForKindAndType) {
    ItemInfo info = make_info(42, 99);
    std::uint16_t k1 = 0, t1 = 0;
    std::uint16_t k2 = 0, t2 = 0;
    get_item_kind_type(&info, k1, t1);
    get_item_kind_type(&info, k2, t2);
    EXPECT_EQ(k1, k2);
    EXPECT_EQ(t1, t2);
    EXPECT_EQ(k1, 42u);
    EXPECT_EQ(t1, 99u);
}


// ===========================================================================
// Boundary values
// ===========================================================================

TEST(GetItemKindTypeDataPlane, KindMaxValue) {
    ItemInfo info = make_info(0xFFFFu, 1);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 0xFFFFu);
    EXPECT_EQ(type, 1u);
}

TEST(GetItemKindTypeDataPlane, TypeMaxValue) {
    ItemInfo info = make_info(1, 0xFFFFu);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 1u);
    EXPECT_EQ(type, 0xFFFFu);
}

TEST(GetItemKindTypeDataPlane, KindOneValue) {
    ItemInfo info = make_info(1, 1);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 1u);
    EXPECT_EQ(type, 1u);
}

TEST(GetItemKindTypeDataPlane, KindValueTwoFiftyNine) {
    ItemInfo info = make_info(259, 11);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 259u);
    EXPECT_EQ(type, 11u);
}


// ===========================================================================
// ItemInfo with other fields populated
// ===========================================================================

TEST(GetItemKindTypeDataPlane, OtherFieldsIgnored) {
    ItemInfo info{};
    info.ItemKind = 100;
    info.ItemType = 200;
    info.ItemIdx = 99999;
    info.BuyPrice = 50000;
    info.ItemGrade = 5;
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 100u);
    EXPECT_EQ(type, 200u);
}

TEST(GetItemKindTypeDataPlane, OnlyKindAffectsOutKind) {
    ItemInfo info{};
    info.ItemKind = 7;
    info.ItemType = 0;
    info.ItemGrade = 99;  // should not affect type
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 7u);
    EXPECT_EQ(type, 0u);
}

TEST(GetItemKindTypeDataPlane, OnlyTypeAffectsOutType) {
    ItemInfo info{};
    info.ItemKind = 0;
    info.ItemType = 11;
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 11u);
}


// ===========================================================================
// Sequential calls
// ===========================================================================

TEST(GetItemKindTypeDataPlane, SequentialHits) {
    ItemInfo info = make_info(1, 2);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 1u);
    EXPECT_EQ(type, 2u);

    info = make_info(3, 4);
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 3u);
    EXPECT_EQ(type, 4u);

    info = make_info(5, 6);
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 5u);
    EXPECT_EQ(type, 6u);
}

TEST(GetItemKindTypeDataPlane, HitThenNull) {
    ItemInfo info = make_info(100, 200);
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 100u);
    EXPECT_EQ(type, 200u);
    get_item_kind_type(nullptr, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 0u);
}

TEST(GetItemKindTypeDataPlane, NullThenHit) {
    std::uint16_t kind = 0, type = 0;
    get_item_kind_type(nullptr, kind, type);
    EXPECT_EQ(kind, 0u);
    EXPECT_EQ(type, 0u);
    ItemInfo info = make_info(100, 200);
    get_item_kind_type(&info, kind, type);
    EXPECT_EQ(kind, 100u);
    EXPECT_EQ(type, 200u);
}
