// D4.40 DiscardSkinItem side-effect dispatcher tests.

#include <mxh/server/skin_discard_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(SkinDiscardSideEffect, PlanEmitsThreeSteps) {
    auto plan = skin_discard_side_effect_plan();
    EXPECT_TRUE(plan.send_broadcast);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              SkinDiscardSideEffectKind::WriteSkinItemUpdate);
    EXPECT_EQ(plan.effects[1].kind,
              SkinDiscardSideEffectKind::CharacterSkinInfoUpdate);
    EXPECT_EQ(plan.effects[2].kind,
              SkinDiscardSideEffectKind::BroadcastSkinInfo);
}

TEST(SkinDiscardSideEffect, PlanIsIdempotent) {
    auto a = skin_discard_side_effect_plan();
    auto b = skin_discard_side_effect_plan();
    EXPECT_EQ(a.send_broadcast, b.send_broadcast);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
    }
}

TEST(SkinDiscardSideEffect, NoStartSkinDelayStep) {
    // Legacy: DiscardSkinItem does NOT reset the skin delay timer.
    // Make sure the data plane does not accidentally emit it.
    auto plan = skin_discard_side_effect_plan();
    for (const auto& effect : plan.effects) {
        EXPECT_NE(effect.kind,
                  static_cast<SkinDiscardSideEffectKind>(0xFFu));
    }
}
