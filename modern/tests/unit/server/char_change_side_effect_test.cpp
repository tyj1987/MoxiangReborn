// Tests for MP_ITEM_SHOPITEM_CHARCHANGE_SYN side-effect dispatcher.

#include <mxh/server/char_change_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

CharChangeValidationInput success_input() {
    CharChangeValidationInput in{};
    in.avatar_effect_clear = true;
    in.item_exists = true;
    in.item_icon_is_char_or_shape = true;
    in.height_in_range = true;
    in.width_in_range = true;
    in.gender_in_range = true;
    in.hair_face_in_range = true;
    in.discard_returned_true = true;
    return in;
}

TEST(CharChangeOutcome, AllGatesPassIsSuccess) {
    EXPECT_EQ(classify_char_change_outcome(success_input()),
              CharChangeOutcome::Success);
}

TEST(CharChangeOutcome, AvatarEffectSetIsAvatarEffect) {
    auto in = success_input();
    in.avatar_effect_clear = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::AvatarEffect);
}

TEST(CharChangeOutcome, ItemMissingIsBadItem) {
    auto in = success_input();
    in.item_exists = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::BadItem);
}

TEST(CharChangeOutcome, WrongIconIsBadItem) {
    auto in = success_input();
    in.item_icon_is_char_or_shape = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::BadItem);
}

TEST(CharChangeOutcome, HeightTooSmallIsBadShape) {
    auto in = success_input();
    in.height_in_range = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::BadShape);
}

TEST(CharChangeOutcome, WidthTooLargeIsBadShape) {
    auto in = success_input();
    in.width_in_range = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::BadShape);
}

TEST(CharChangeOutcome, GenderOutOfRangeIsBadGender) {
    auto in = success_input();
    in.gender_in_range = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::BadGender);
}

TEST(CharChangeOutcome, HairTypeOutOfRangeIsBadHairFace) {
    auto in = success_input();
    in.hair_face_in_range = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::BadHairFace);
}

TEST(CharChangeOutcome, DiscardFailedIsDiscardFailed) {
    auto in = success_input();
    in.discard_returned_true = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::DiscardFailed);
}

TEST(CharChangeOutcome, AvatarEffectTakesPrecedenceOverBadItem) {
    auto in = success_input();
    in.avatar_effect_clear = false;
    in.item_exists = false;
    EXPECT_EQ(classify_char_change_outcome(in),
              CharChangeOutcome::AvatarEffect);
}

TEST(CharChangeNackCode, MapsOutcomesToLegacyCodes) {
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::BadItem), 1u);
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::BadShape), 2u);
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::BadGender), 3u);
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::BadHairFace), 4u);
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::DiscardFailed), 5u);
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::AvatarEffect), 6u);
    EXPECT_EQ(char_change_nack_code(CharChangeOutcome::Success), 0u);
}

TEST(CharChangePlan, SuccessEmitsFullSequence) {
    auto in = success_input();
    CharChangeValidationFields info{};
    info.height = 1.05f;
    info.width  = 0.95f;
    info.gender = 1;
    info.hair_type = 3;
    info.face_type = 2;
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.send_char_change_ack);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_TRUE(plan.db_call);
    EXPECT_TRUE(plan.discard_item);
    EXPECT_TRUE(plan.set_char_change_info);
    EXPECT_TRUE(plan.log_item_money);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 7u);
    EXPECT_EQ(plan.effects[0].kind,
              CharChangeSideEffectKind::DiscardCharChangeItem);
    EXPECT_EQ(plan.effects[1].kind,
              CharChangeSideEffectKind::SetCharChangeInfo);
    EXPECT_EQ(plan.effects[1].info.height, 1.05f);
    EXPECT_EQ(plan.effects[2].kind,
              CharChangeSideEffectKind::SendUseAckToPlayer);
    EXPECT_EQ(plan.effects[3].kind,
              CharChangeSideEffectKind::BroadcastCharChange);
    EXPECT_EQ(plan.effects[3].icon_kind, CharChangeIcon::CharChange);
    EXPECT_EQ(plan.effects[4].kind,
              CharChangeSideEffectKind::CharacterChangeInfoToDB);
    EXPECT_EQ(plan.effects[5].kind,
              CharChangeSideEffectKind::SendCharChangeAck);
    EXPECT_EQ(plan.effects[6].kind,
              CharChangeSideEffectKind::LogItemMoney);
}

TEST(CharChangePlan, SuccessShapeChangePreservesSavedValues) {
    auto in = success_input();
    CharChangeValidationFields info{};
    info.height = 1.05f;
    info.width  = 0.95f;
    info.gender = 1;
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::ShapeChange, info, 2, 1.0f, 1.0f);
    EXPECT_EQ(plan.effects[3].icon_kind, CharChangeIcon::ShapeChange);
    EXPECT_EQ(plan.effects[3].saved_gender, 2u);
    EXPECT_EQ(plan.effects[4].icon_kind, CharChangeIcon::ShapeChange);
}

TEST(CharChangePlan, AvatarEffectSendsNackCode6) {
    auto in = success_input();
    in.avatar_effect_clear = false;
    CharChangeValidationFields info{};
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, 6u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              CharChangeSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 6u);
}

TEST(CharChangePlan, BadItemSendsNackCode1) {
    auto in = success_input();
    in.item_exists = false;
    CharChangeValidationFields info{};
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);
}

TEST(CharChangePlan, BadShapeSendsNackCode2) {
    auto in = success_input();
    in.height_in_range = false;
    CharChangeValidationFields info{};
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);
}

TEST(CharChangePlan, BadGenderSendsNackCode3) {
    auto in = success_input();
    in.gender_in_range = false;
    CharChangeValidationFields info{};
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 3u);
}

TEST(CharChangePlan, BadHairFaceSendsNackCode4) {
    auto in = success_input();
    in.hair_face_in_range = false;
    CharChangeValidationFields info{};
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 4u);
}

TEST(CharChangePlan, DiscardFailedSendsNackCode5) {
    auto in = success_input();
    in.discard_returned_true = false;
    CharChangeValidationFields info{};
    auto plan = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 5u);
}

TEST(CharChangePlan, PlanIsIdempotent) {
    auto in = success_input();
    CharChangeValidationFields info{};
    auto a = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    auto b = char_change_side_effect_plan(
        in, 100, 50, 6, CharChangeIcon::CharChange, info, 0, 1.0f, 1.0f);
    EXPECT_EQ(a.send_use_ack, b.send_use_ack);
    EXPECT_EQ(a.broadcast, b.broadcast);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
