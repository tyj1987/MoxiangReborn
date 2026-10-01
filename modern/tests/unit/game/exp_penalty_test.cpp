#include "mxh/game/exp_penalty.hpp"
#include "mxh/game/map_kind.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

TEST(MapKind, PreservesOriginalFlagBitsAndRejectsIncompleteTables) {
    const auto table=mxh::game::parse_map_kind_text(
        "10 name MAPVIEW # 5 siege MAPVIEW | SIEGEWAR # 28 tournament TOURNAMENT # "
        "37 quest QUEST # 42 survival SURVIVAL # 55 eventname # 99 running RUNNING # 0");
    ASSERT_TRUE(table);
    EXPECT_EQ(table->at(10),64u);
    EXPECT_EQ(table->at(5),320u);
    EXPECT_EQ(table->at(99),129u);
    EXPECT_EQ(table->at(55),0u); // Name is not a map-kind flag.
    EXPECT_EQ(table->at(10)&mxh::game::present_revive_exempt_map_mask,0u);
    for(auto id:{5u,28u,37u,42u,99u})
        EXPECT_NE(table->at(id)&mxh::game::present_revive_exempt_map_mask,0u);
    const auto unknown=mxh::game::parse_map_kind_text("10 n UNKNOWN #");
    ASSERT_TRUE(unknown); EXPECT_FALSE(unknown->resolved_flags(10));
    for(auto text:{"", "10 name MAPVIEW", "10 n # 10 n #", "10 n # 0 garbage"})
        EXPECT_FALSE(mxh::game::parse_map_kind_text(text))<<text;
}

TEST(MapKind, CurrentResourceHasValidatedContainerAndMap10IsOrdinary) {
    std::ifstream input(std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource/MapKindInfo.bin",std::ios::binary);
    ASSERT_TRUE(input);
    const std::vector<std::uint8_t> raw{std::istreambuf_iterator<char>(input),{}};
    ASSERT_GE(raw.size(),14u);
    const auto table=mxh::game::decode_map_kind(raw);
    ASSERT_TRUE(table);
    EXPECT_EQ(table->at(10),64u);
    EXPECT_EQ(table->resolved_flags(10),64u);
    EXPECT_FALSE(table->resolved_flags(118)); // current INSDUNGEON has no source definition yet
    EXPECT_NE(table->at(5)&mxh::game::present_revive_exempt_map_mask,0u);
    for (const auto at : {4u,8u,12u,13u,static_cast<unsigned>(raw.size()-1)}) {
        auto corrupt=raw; corrupt[at]^=1;
        EXPECT_FALSE(mxh::game::decode_map_kind(corrupt)) << at;
    }
    for (std::size_t length=0;length<raw.size();++length)
        EXPECT_FALSE(mxh::game::decode_map_kind(std::span(raw).first(length))) << length;
}

TEST(ExpPenalty, PreservesPercentagesAndMissingLevels) {
    const auto table = mxh::game::parse_exp_penalty_text("1\t0\t0\r\n48\t2.4\t1.9\r\n99 1.7 1.4\n");
    ASSERT_TRUE(table);
    EXPECT_FLOAT_EQ(table->at(48).present_percent, 2.4f);
    EXPECT_FLOAT_EQ(table->at(48).login_percent, 1.9f);
    EXPECT_EQ(table->count(100), 0u);
}

TEST(ExpPenalty, UnprotectedLossUsesTableFallbackAndModeSpecificRates) {
    using namespace mxh::game;
    const ExpPenaltyTable table{{48, {2.4f,1.9f}}};
    const auto present=unprotected_revive_loss(table,ReviveLocation::Present,48,101,10000);
    ASSERT_TRUE(present); EXPECT_EQ(present->money,6u); EXPECT_EQ(present->experience,240u);
    const auto login=unprotected_revive_loss(table,ReviveLocation::Login,48,101,10000);
    ASSERT_TRUE(login); EXPECT_EQ(login->money,4u); EXPECT_EQ(login->experience,190u);
    EXPECT_EQ(unprotected_revive_loss(table,ReviveLocation::Login,48,101,10000,true)->experience,100u);
    EXPECT_EQ(unprotected_revive_loss(table,ReviveLocation::Village,48,101,10000)->experience,100u);
    EXPECT_EQ(unprotected_revive_loss(table,ReviveLocation::Present,100,0,10000)->experience,300u);
    EXPECT_EQ(unprotected_revive_loss(table,ReviveLocation::Login,100,0,10000)->experience,200u);
    EXPECT_FALSE(unprotected_revive_loss(table,ReviveLocation::Login,0,0,100));
    EXPECT_FALSE(unprotected_revive_loss(table,ReviveLocation::Present,48,0,100,true));
    EXPECT_FALSE(unprotected_revive_loss(table,ReviveLocation::Login,48,0,~std::uint64_t{}));
}

TEST(ExpPenalty, CombinedProtectionHasPriorityAndExpiresAtLastCharge) {
    const auto plan=mxh::game::plan_revive_protection({60,240},2,true,true,true);
    ASSERT_TRUE(plan);
    EXPECT_TRUE(plan->consume_combined); EXPECT_FALSE(plan->end_combined);
    EXPECT_EQ(plan->remaining_combined,1);
    EXPECT_FALSE(plan->consume_money); EXPECT_FALSE(plan->consume_experience);
    EXPECT_EQ(plan->loss.money,0u); EXPECT_EQ(plan->loss.experience,0u);
    const auto last=mxh::game::plan_revive_protection({60,240},1,true,false,false);
    ASSERT_TRUE(last); EXPECT_TRUE(last->end_combined); EXPECT_EQ(last->remaining_combined,0);
}

TEST(ExpPenalty, OrphanCombinedCountFallsBackToIndividualProtections) {
    const auto money=mxh::game::plan_revive_protection({60,240},2,false,true,false);
    ASSERT_TRUE(money); EXPECT_FALSE(money->consume_combined);
    EXPECT_TRUE(money->consume_money); EXPECT_EQ(money->loss.money,0u);
    EXPECT_EQ(money->loss.experience,240u);
    const auto exp=mxh::game::plan_revive_protection({60,240},0,false,false,true);
    ASSERT_TRUE(exp); EXPECT_TRUE(exp->consume_experience);
    EXPECT_EQ(exp->loss.money,60u); EXPECT_EQ(exp->loss.experience,0u);
    EXPECT_FALSE(mxh::game::plan_revive_protection({60,240},-1,true,false,false));
}

TEST(ExpPenalty, RejectsIncompleteDuplicateAndInvalidNumbers) {
    for (const auto* text : {"", "1 2", "1 2 3 1 2 3", "0 2 3", "65536 2 3",
                            "1 nan 2", "1 2 inf", "1 -1 2", "1 2 101", "1 2x 3"}) {
        SCOPED_TRACE(text);
        EXPECT_FALSE(mxh::game::parse_exp_penalty_text(text));
    }
}

TEST(ExpPenalty, VerifiedCurrentResourceContains99Levels) {
    const auto path = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/Server/ExpPenalty.bin";
    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input);
    const std::vector<unsigned char> raw((std::istreambuf_iterator<char>(input)), {});
    ASSERT_EQ(raw.size(), 1025u);
    const auto table = mxh::game::decode_exp_penalty(raw, mxh::game::ExpPenaltyProfile::PlayDhCurrent);
    ASSERT_TRUE(table);
    ASSERT_EQ(table->size(), 99u);
    for (unsigned level = 1; level <= 99; ++level) EXPECT_EQ(table->count(level), 1u);
    EXPECT_FLOAT_EQ(table->at(5).present_percent, 0.0f);
    EXPECT_FLOAT_EQ(table->at(6).present_percent, 3.0f);
    EXPECT_FLOAT_EQ(table->at(48).present_percent, 2.4f);
    EXPECT_FLOAT_EQ(table->at(48).login_percent, 1.9f);
    EXPECT_FLOAT_EQ(table->at(99).login_percent, 1.4f);
    EXPECT_FALSE(mxh::game::decode_exp_penalty(raw, mxh::game::ExpPenaltyProfile::LegacyMhFile));
    auto truncated = raw;
    truncated.pop_back();
    EXPECT_FALSE(mxh::game::decode_exp_penalty(truncated, mxh::game::ExpPenaltyProfile::PlayDhCurrent));
}

TEST(ExpPenalty, LegacyCrcAndCurrentProfileAgreeForEveryLevel) {
    const auto root = std::filesystem::path(MXH_SOURCE_DIR).parent_path();
    std::ifstream input(root / "reference/legacy-source/4dddd9a6/SWorking/Resource/Server/ExpPenalty.bin", std::ios::binary);
    ASSERT_TRUE(input);
    std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(input)), {});
    const auto legacy = mxh::game::decode_exp_penalty(raw, mxh::game::ExpPenaltyProfile::LegacyMhFile);
    ASSERT_TRUE(legacy);
    std::ifstream current_input(root / "modern/data/PlayDH/Resource/Server/ExpPenalty.bin", std::ios::binary);
    ASSERT_TRUE(current_input);
    const std::vector<std::uint8_t> current_raw((std::istreambuf_iterator<char>(current_input)), {});
    const auto current = mxh::game::decode_exp_penalty(current_raw, mxh::game::ExpPenaltyProfile::PlayDhCurrent);
    ASSERT_TRUE(current);
    ASSERT_EQ(legacy->size(), current->size());
    for (const auto& [level, penalty] : *legacy) {
        ASSERT_NE(current->find(level), current->end());
        EXPECT_FLOAT_EQ(penalty.present_percent, current->at(level).present_percent);
        EXPECT_FLOAT_EQ(penalty.login_percent, current->at(level).login_percent);
    }
    raw[12] ^= 1;
    EXPECT_FALSE(mxh::game::decode_exp_penalty(raw, mxh::game::ExpPenaltyProfile::LegacyMhFile));
}
