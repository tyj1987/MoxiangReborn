// shop_item_update_sql_test.cpp - 1:1 SQL format tests for the legacy
// MapDBMsgParser::ShopItemUpdatetimeToDB / ShopItemUpdateUseInfoToDB /
// ShopItemParamUpdateToDB functions. Locks the EXEC <sp> <args> byte-for-byte
// against the legacy sprintf output, so the modern DbThread dispatcher can
// hand the produced string to any MSSQL / SQLite backend without semantic drift.

#include <mxh/server/shop_item_update_sql.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>

using mxh::server::build_shop_item_update_time_sql;
using mxh::server::build_shop_item_update_use_info_sql;
using mxh::server::build_shop_item_update_param_sql;

// -----------------------------------------------------------------------
// build_shop_item_update_time_sql -- 3 args (EXEC dbo.MP_SHOPITEM_Updatetime)
// -----------------------------------------------------------------------

TEST(ShopItemUpdateTimeSql, BasicFormat) {
    auto s = build_shop_item_update_time_sql(123, 456, 789);
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_Updatetime 123, 456, 789"));
}

TEST(ShopItemUpdateTimeSql, ZeroArguments) {
    auto s = build_shop_item_update_time_sql(0, 0, 0);
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_Updatetime 0, 0, 0"));
}

TEST(ShopItemUpdateTimeSql, LargeUnsignedArguments) {
    // 0x0FFFFFFF = 268435455. Realistic character / item / remain values
    // are well below 2^31, so this still matches legacy sprintf %d output.
    auto s = build_shop_item_update_time_sql(0x0FFFFFFFu, 0x0FFFFFFFu, 0x0FFFFFFFu);
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_Updatetime 268435455, 268435455, 268435455"));
}

TEST(ShopItemUpdateTimeSql, StoredProcedureNameMatchesLegacy) {
    auto s = build_shop_item_update_time_sql(1, 2, 3);
    EXPECT_NE(s.find("dbo.MP_SHOPITEM_Updatetime"), std::string::npos);
    // Legacy macro: STORED_SHOPITEM_UPDATETIME -- the dbo. prefix is part
    // of the macro value, not a runtime prefix added by the dispatcher.
}

TEST(ShopItemUpdateTimeSql, TwoCommasImpliesThreeArguments) {
    // Lock the comma count: 2 commas between digits = 3 args (legacy
    // sprintf %d, %d, %d). Guarding against a future %d, %d, %d, %d typo.
    auto s = build_shop_item_update_time_sql(1, 2, 3);
    EXPECT_EQ(std::count(s.begin(), s.end(), ','), 2L);
}

TEST(ShopItemUpdateTimeSql, HasExecPrefix) {
    auto s = build_shop_item_update_time_sql(1, 2, 3);
    EXPECT_EQ(s.substr(0, 5), std::string("EXEC "));
}

// -----------------------------------------------------------------------
// build_shop_item_update_use_info_sql -- 4 args (EXEC dbo.MP_SHOPITEM_UpdateUseInfo)
// -----------------------------------------------------------------------

TEST(ShopItemUpdateUseInfoSql, BasicFormat) {
    auto s = build_shop_item_update_use_info_sql(100, 200, 300, 400);
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_UpdateUseInfo 100, 200, 300, 400"));
}

TEST(ShopItemUpdateUseInfoSql, FourArgumentsThreeCommas) {
    auto s = build_shop_item_update_use_info_sql(1, 2, 3, 4);
    EXPECT_EQ(std::count(s.begin(), s.end(), ','), 3L);
}

TEST(ShopItemUpdateUseInfoSql, ZeroParamAndRemain) {
    auto s = build_shop_item_update_use_info_sql(7, 8, 0, 0);
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_UpdateUseInfo 7, 8, 0, 0"));
}

// -----------------------------------------------------------------------
// build_shop_item_update_param_sql -- 3 args (EXEC dbo.MP_SHOPITEM_UpdateParam)
// -----------------------------------------------------------------------

TEST(ShopItemUpdateParamSql, BasicFormat) {
    auto s = build_shop_item_update_param_sql(11, 22, 33);
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_UpdateParam 11, 22, 33"));
}

TEST(ShopItemUpdateParamSql, ThreeArgumentsTwoCommas) {
    auto s = build_shop_item_update_param_sql(1, 2, 3);
    EXPECT_EQ(std::count(s.begin(), s.end(), ','), 2L);
}

// -----------------------------------------------------------------------
// Cross-builder invariants
// -----------------------------------------------------------------------

TEST(ShopItemUpdateSql, NoHexPrefix) {
    auto s = build_shop_item_update_time_sql(0xFFu, 0xFFu, 0xFFu);
    // Legacy uses %d (decimal). 0xFF = 255.
    EXPECT_EQ(s, std::string("EXEC dbo.MP_SHOPITEM_Updatetime 255, 255, 255"));
}

TEST(ShopItemUpdateSql, NoLeadingZeros) {
    auto s = build_shop_item_update_param_sql(0, 1, 2);
    // Legacy sprintf %d does not zero-pad. The EXEC prefix and SP name
    // are the only literal text before the args.
    EXPECT_EQ(s.find("EXEC dbo.MP_SHOPITEM_UpdateParam 0, 1, 2"), 0u);
}

TEST(ShopItemUpdateSql, Deterministic) {
    auto a = build_shop_item_update_time_sql(123, 456, 789);
    auto b = build_shop_item_update_time_sql(123, 456, 789);
    EXPECT_EQ(a, b);
}
