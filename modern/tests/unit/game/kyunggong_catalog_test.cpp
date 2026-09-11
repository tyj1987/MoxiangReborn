#include "mxh/game/kyunggong_catalog.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include <gtest/gtest.h>
using namespace mxh::game;
TEST(KyungGongCatalog, ExactFieldsAndEffectSentinelArePreserved) {
    std::string error;
    auto catalog=KyungGongCatalog::parse("2600 raw 5 1 600 2500 -1 151 152\r\n",error);
    ASSERT_TRUE(catalog)<<error; ASSERT_EQ(catalog->size(),1);
    const auto* row=catalog->find(2600); ASSERT_NE(row,nullptr);
    EXPECT_EQ(row->need_mp,5); EXPECT_EQ(row->move_type,1); EXPECT_FLOAT_EQ(row->speed,600);
    EXPECT_EQ(row->change_time,2500); EXPECT_EQ(row->start_effect,65535);
    EXPECT_EQ(row->ongoing_effect,151); EXPECT_EQ(row->end_effect,152);
    EXPECT_EQ(row->source_name,"raw"); EXPECT_EQ(catalog->find(2601),nullptr);
    // CharMove.cpp explicitly ends movement when Speed==0. Parsed is not the
    // same as permission to move; retain that original representable value.
    auto zero=KyungGongCatalog::parse("2600 raw 5 1 0 2500 -1 151 152",error);
    ASSERT_TRUE(zero); EXPECT_FLOAT_EQ(zero->find(2600)->speed,0);
}
TEST(KyungGongCatalog, MalformedOrAmbiguousDataDoesNotProducePartialCatalog) {
    for(const auto* text : {"", "2600 x 5 1 600 2500 -1 151", "2600 x 5 1 NaN 2 -1 1 2",
        "2600 x 5 1 -2 2 -1 1 2", "2600 x 65536 1 600 2 -1 1 2", "2600 x 5 1 600x 2 -1 1 2",
        "2600 x 5 1 600 2 -2 1 2", "2600 x 5 1 600 2 -1 1 2 trailing",
        "2600 x 5 1 600 2 -1 1 2 2600 y 6 2 900 2 -1 3 4"}) {
        std::string error; EXPECT_FALSE(KyungGongCatalog::parse(text,error))<<text;
        EXPECT_FALSE(error.empty());
    }
}
TEST(KyungGongCatalog, CanonicalSourceHasFiveOriginalSpeedsAndMpCosts) {
    auto raw=mxh::compat::read_mh_bin(std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource/KyungGongInfo.bin");
    ASSERT_TRUE(raw);
    std::string error;
    auto catalog=KyungGongCatalog::parse(std::string_view(reinterpret_cast<const char*>(raw.value.data.data()),raw.value.data.size()),error);
    ASSERT_TRUE(catalog)<<error; ASSERT_EQ(catalog->size(),5);
    const unsigned costs[]{5,7,10,13,15};
    for(unsigned i=0;i<5;++i) {
        const auto* row=catalog->find(static_cast<std::uint16_t>(2600+i)); ASSERT_NE(row,nullptr);
        EXPECT_FLOAT_EQ(row->speed,600.0f+150.0f*i); EXPECT_EQ(row->need_mp,costs[i]);
        EXPECT_EQ(row->start_effect,65535);
    }
}
