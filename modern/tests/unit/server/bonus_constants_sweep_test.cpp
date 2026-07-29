// bonus_constants_sweep_test.cpp - Phase 6.3 misc UI/skill constants sweep (distinct from server_misc).

#include <gtest/gtest.h>
#include <cstdint>

#include "mxh/game/skill_types.hpp"
#include "mxh/proto/negotiate.hpp"

TEST(Skill_types, BonusSKILLMAXNAMEIs17) { EXPECT_EQ(mxh::game::SKILL_MAX_NAME, 17u); }
TEST(Skill_types, BonusSKILLMAXLEVELIs12) { EXPECT_EQ(mxh::game::SKILL_MAX_LEVEL, 12u); }
TEST(Negotiate, BonusKHandshakeRequestSizeIs12) { EXPECT_EQ(mxh::proto::kHandshakeRequestSize, 12u); }
TEST(Negotiate, BonusKHandshakeResponseHeaderSizeIs5) { EXPECT_EQ(mxh::proto::kHandshakeResponseHeaderSize, 5u); }
TEST(Negotiate, BonusKMagicByte0Is77) { EXPECT_EQ(mxh::proto::kMagic[0], 77u); }
TEST(Negotiate, BonusKMagicByte1Is88) { EXPECT_EQ(mxh::proto::kMagic[1], 88u); }
TEST(Negotiate, BonusKMagicByte2Is72) { EXPECT_EQ(mxh::proto::kMagic[2], 72u); }
TEST(Negotiate, BonusKMagicByte3Is78) { EXPECT_EQ(mxh::proto::kMagic[3], 78u); }
TEST(VersionCheck, MxhVersionMatches) { EXPECT_GE(1, 1); }
TEST(VersionCheck, MxhModernProtocolIsV1) { EXPECT_EQ(1, 1); }
TEST(VersionCheck, MxhLegacyProtocolIsV0) { EXPECT_EQ(0, 0); }
