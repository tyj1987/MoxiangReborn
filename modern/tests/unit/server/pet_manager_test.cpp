// pet_manager_test.cpp - Phase D5 PetManager 1:1 port tests.
// Locks behavior of CPetManager state machine against legacy constants.
// Each TEST verifies one 1:1 contract: constants, formulas, or state
// transitions matching ??[Source]/[Server]Map/PetManager.cpp.

#include "mxh/server/pet_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::PetManagerState;
using mxh::server::PetTotalInfo;
using mxh::server::PetBuffKind;
using mxh::server::PetUpgradeResult;
using mxh::server::PetFeedResult;
using mxh::server::PetKind;
using mxh::server::PetBuffData;
using mxh::server::PetUpgradeProfile;
using mxh::server::make_pet_manager;
using mxh::server::init_pet_manager;
using mxh::server::add_pet_total_info;
using mxh::server::remove_pet_total_info;
using mxh::server::find_pet_total_info;
using mxh::server::gradeup_probability_basis_points;
using mxh::server::upgrade_pet;
using mxh::server::add_friendship;
using mxh::server::is_pet_max_friendship;
using mxh::server::is_pet_stamina_zero;
using mxh::server::is_pet_above_default_friendly;
using mxh::server::tick_skill_recharge;
using mxh::server::set_skill_ready;
using mxh::server::add_stamina;
using mxh::server::begin_resummon_cooldown;
using mxh::server::check_resummon_available;
using mxh::server::begin_release_delay;
using mxh::server::clear_release_delay;
using mxh::server::summon_pet;
using mxh::server::unsummon_pet;
using mxh::server::has_cur_summon;
using mxh::server::feed_up_pet;
using mxh::server::seal_pet;
using mxh::server::revival_pet;
using mxh::server::tick_event_pet_remain;
using mxh::server::start_event_pet;
using mxh::server::add_pet_buff_flag;
using mxh::server::remove_pet_buff_flag;
using mxh::server::has_pet_buff_flag;
using mxh::server::apply_friendship_protection;
using mxh::server::set_buff_data;
using mxh::server::get_buff_data;
using mxh::server::PET_DEFAULT_FRIENDLY;
using mxh::server::PET_REVIVAL_FRIENDLY;
using mxh::server::PET_MAX_FRIENDLY;
using mxh::server::PET_MAX_GRADE;
using mxh::server::PET_GRADEUP_PROB_1TO2;
using mxh::server::PET_GRADEUP_PROB_2TO3;
using mxh::server::PET_MAX_SKILL_CHARGE;
using mxh::server::PET_SKILLCHARGE_CHECKTIME;
using mxh::server::PET_RESUMMON_VALID_TIME;
using mxh::server::CRISTMAS_EVENTPET_SUMMONNING_TIME;

static PetTotalInfo make_pet(std::uint32_t item_db_idx = 1u,
                             std::uint16_t kind = static_cast<std::uint16_t>(PetKind::CommonPet),
                             std::uint16_t grade = 1u,
                             std::uint32_t stamina = 1000u,
                             std::uint32_t friendly = PET_DEFAULT_FRIENDLY,
                             std::uint8_t alive = 1u) {
    PetTotalInfo p;
    p.PetSummonItemDBIdx = item_db_idx;
    p.PetKind = kind;
    p.PetGrade = grade;
    p.PetStamina = stamina;
    p.PetFriendly = friendly;
    p.bAlive = alive;
    p.bRest = 0;
    p.bSummonning = 0;
    return p;
}
}

// ---- Constants 1:1 ----

TEST(PetManagerConstants, DefaultFriendlyMatchesLegacy) {
    EXPECT_EQ(PET_DEFAULT_FRIENDLY, 3000000u);
}

TEST(PetManagerConstants, RevivalFriendlyMatchesLegacy) {
    EXPECT_EQ(PET_REVIVAL_FRIENDLY, 2000000u);
}

TEST(PetManagerConstants, MaxFriendlyMatchesLegacy) {
    EXPECT_EQ(PET_MAX_FRIENDLY, 10000000u);
}

TEST(PetManagerConstants, MaxGradeMatchesLegacy) {
    EXPECT_EQ(PET_MAX_GRADE, 3u);
}

TEST(PetManagerConstants, GradeUpProb1To2MatchesLegacy) {
    EXPECT_EQ(PET_GRADEUP_PROB_1TO2, 80u);
}

TEST(PetManagerConstants, GradeUpProb2To3MatchesLegacy) {
    EXPECT_EQ(PET_GRADEUP_PROB_2TO3, 80u);
}

TEST(PetManagerConstants, MaxSkillChargeMatchesLegacy) {
    EXPECT_EQ(PET_MAX_SKILL_CHARGE, 10000u);
}

TEST(PetManagerConstants, SkillChargeCheckTimeMatchesLegacy) {
    EXPECT_EQ(PET_SKILLCHARGE_CHECKTIME, 1000u);
}

TEST(PetManagerConstants, ResummonValidTimeMatchesLegacy) {
    EXPECT_EQ(PET_RESUMMON_VALID_TIME, 30000u);
}

TEST(PetManagerConstants, EventPetSummonTimeMatchesLegacy) {
    EXPECT_EQ(CRISTMAS_EVENTPET_SUMMONNING_TIME, 60000u * 30u);
}

// ---- State construction ----

TEST(PetManagerState, DefaultConstructionIsZeroed) {
    auto s = make_pet_manager();
    EXPECT_EQ(s.m_dwSkillRechargeAmount, 0u);
    EXPECT_FALSE(s.m_bSkillGuageFull);
    EXPECT_FALSE(s.m_bPetStaminaZero);
    EXPECT_EQ(s.m_dwResummonDelayTime, 0u);
    EXPECT_EQ(s.m_BuffFlag, static_cast<int>(PetBuffKind::None));
    EXPECT_EQ(s.m_wPetKind, 0u);
    EXPECT_TRUE(s.m_PetInfoList.empty());
    EXPECT_FALSE(has_cur_summon(s));
}

TEST(PetManagerState, InitClearsAllTimers) {
    auto s = make_pet_manager();
    s.m_dwReleaseDelayTime = 999u;
    s.m_BuffFlag = 0xFF;
    init_pet_manager(s);
    EXPECT_EQ(s.m_dwReleaseDelayTime, 0u);
    EXPECT_EQ(s.m_BuffFlag, static_cast<int>(PetBuffKind::None));
    EXPECT_TRUE(s.m_PetInfoList.empty());
}

// ---- Add/Remove pet total info ----

TEST(PetManagerAddRemove, AddAppendsToList) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u));
    add_pet_total_info(s, make_pet(2u));
    EXPECT_EQ(s.m_PetInfoList.size(), 2u);
}

TEST(PetManagerAddRemove, FindByItemDBIdx) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(7u));
    add_pet_total_info(s, make_pet(13u));
    auto* p = find_pet_total_info(s, 13u);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->PetSummonItemDBIdx, 13u);
}

TEST(PetManagerAddRemove, FindMissingReturnsNull) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u));
    EXPECT_EQ(find_pet_total_info(s, 99u), nullptr);
}

TEST(PetManagerAddRemove, RemovePetDeletesFromList) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u));
    add_pet_total_info(s, make_pet(2u));
    remove_pet_total_info(s, 1u);
    EXPECT_EQ(s.m_PetInfoList.size(), 1u);
    EXPECT_EQ(s.m_PetInfoList[0].PetSummonItemDBIdx, 2u);
}

TEST(PetManagerAddRemove, RemoveCurSummonClearsIt) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u));
    summon_pet(s, 1u);
    ASSERT_TRUE(has_cur_summon(s));
    remove_pet_total_info(s, 1u);
    EXPECT_FALSE(has_cur_summon(s));
}

// ---- Grade-up ----

TEST(PetManagerGradeUp, ProbabilityAtGrade1Is80Percent) {
    EXPECT_EQ(gradeup_probability_basis_points(1), 8000u);
}

TEST(PetManagerGradeUp, ProbabilityAtGrade2Is80Percent) {
    EXPECT_EQ(gradeup_probability_basis_points(2), 8000u);
}

TEST(PetManagerGradeUp, ProbabilityAtMaxGradeIsZero) {
    EXPECT_EQ(gradeup_probability_basis_points(PET_MAX_GRADE), 0u);
}

TEST(PetManagerGradeUp, RollBelowProbSucceeds) {
    auto p = make_pet(/*idx*/1u, static_cast<std::uint16_t>(PetKind::CommonPet), /*grade*/1u);
    auto r = upgrade_pet(p, /*roll*/7999u);
    EXPECT_EQ(r, PetUpgradeResult::UpgradeSucess);
    EXPECT_EQ(p.PetGrade, 2u);
}

TEST(PetManagerGradeUp, RollAtOrAboveProbFails) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u);
    auto r = upgrade_pet(p, 8000u);
    EXPECT_EQ(r, PetUpgradeResult::UpgradeFailforProb);
    EXPECT_EQ(p.PetGrade, 1u);
}

TEST(PetManagerGradeUp, AtMaxGradeFailsThirdUpgrade) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), PET_MAX_GRADE);
    auto r = upgrade_pet(p, 0u);
    EXPECT_EQ(r, PetUpgradeResult::UpgradeFailfor3rdUp);
}

TEST(PetManagerGradeUp, ThirdUpgradeBlockFlag) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u);
    auto r = upgrade_pet(p, 0u, /*is_third_upgrade_blocked*/ true);
    EXPECT_EQ(r, PetUpgradeResult::UpgradeFailforSamePetSummoned);
    EXPECT_EQ(p.PetGrade, 1u);
}

// ---- Friendship ----

TEST(PetManagerFriendship, AddNegativeClampsAtZero) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 0u, 100u);
    add_friendship(p, -50);
    EXPECT_EQ(p.PetFriendly, 50u);
    add_friendship(p, -9999);
    EXPECT_EQ(p.PetFriendly, 0u);
}

TEST(PetManagerFriendship, AddPositiveClampsAtMax) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 0u, PET_MAX_FRIENDLY - 10u);
    add_friendship(p, 100);
    EXPECT_EQ(p.PetFriendly, PET_MAX_FRIENDLY);
}

TEST(PetManagerFriendship, IsMaxDetectsMax) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 0u, PET_MAX_FRIENDLY);
    EXPECT_TRUE(is_pet_max_friendship(p));
    p.PetFriendly -= 1u;
    EXPECT_FALSE(is_pet_max_friendship(p));
}

TEST(PetManagerFriendship, IsStaminaZeroDetectsZero) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, /*stamina*/0u);
    EXPECT_TRUE(is_pet_stamina_zero(p));
    p.PetStamina = 1u;
    EXPECT_FALSE(is_pet_stamina_zero(p));
}

TEST(PetManagerFriendship, AboveDefaultFriendlyFlag) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 0u, PET_DEFAULT_FRIENDLY);
    EXPECT_FALSE(is_pet_above_default_friendly(p));
    p.PetFriendly = PET_DEFAULT_FRIENDLY + 1u;
    EXPECT_TRUE(is_pet_above_default_friendly(p));
}

// ---- Skill recharge ----

TEST(PetManagerSkillRecharge, TickIncrementsAmount) {
    auto s = make_pet_manager();
    tick_skill_recharge(s, 100u);
    EXPECT_EQ(s.m_dwSkillRechargeAmount, 100u);
    EXPECT_FALSE(s.m_bSkillGuageFull);
}

TEST(PetManagerSkillRecharge, FullChargesFlipFlag) {
    auto s = make_pet_manager();
    tick_skill_recharge(s, PET_MAX_SKILL_CHARGE);
    EXPECT_TRUE(s.m_bSkillGuageFull);
    EXPECT_EQ(s.m_dwSkillRechargeAmount, PET_MAX_SKILL_CHARGE);
}

TEST(PetManagerSkillRecharge, SetReadySaturatesAtMax) {
    auto s = make_pet_manager();
    set_skill_ready(s);
    EXPECT_TRUE(s.m_bSkillGuageFull);
    EXPECT_EQ(s.m_dwSkillRechargeAmount, PET_MAX_SKILL_CHARGE);
}

// ---- Stamina ----

TEST(PetManagerStamina, AddPositiveClampsAtMax) {
    auto s = make_pet_manager();
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 90u);
    add_stamina(s, p, 50, 100u);
    EXPECT_EQ(p.PetStamina, 100u);
}

TEST(PetManagerStamina, AddNegativeClampsAtZeroAndFlipsFlag) {
    auto s = make_pet_manager();
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 100u);
    add_stamina(s, p, -200, 1000u);
    EXPECT_EQ(p.PetStamina, 0u);
    EXPECT_TRUE(s.m_bPetStaminaZero);
}

TEST(PetManagerStamina, RecoverFromZeroClearsFlag) {
    auto s = make_pet_manager();
    s.m_bPetStaminaZero = true;
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 0u);
    add_stamina(s, p, 10, 100u);
    EXPECT_EQ(p.PetStamina, 10u);
    EXPECT_FALSE(s.m_bPetStaminaZero);
}

// ---- Resummon / Release delays ----

TEST(PetManagerDelays, ResummonAvailableImmediatelyIfUnset) {
    auto s = make_pet_manager();
    EXPECT_TRUE(check_resummon_available(s, 100000u));
}

TEST(PetManagerDelays, ResummonBlockedWithinWindow) {
    auto s = make_pet_manager();
    begin_resummon_cooldown(s, 1000u);
    EXPECT_FALSE(check_resummon_available(s, 1000u + PET_RESUMMON_VALID_TIME - 1u));
}

TEST(PetManagerDelays, ResummonAllowedAfterWindow) {
    auto s = make_pet_manager();
    begin_resummon_cooldown(s, 1000u);
    EXPECT_TRUE(check_resummon_available(s, 1000u + PET_RESUMMON_VALID_TIME));
}

TEST(PetManagerDelays, ReleaseDelayStoresAndClears) {
    auto s = make_pet_manager();
    begin_release_delay(s, 5555u);
    EXPECT_EQ(s.m_dwReleaseDelayTime, 5555u);
    clear_release_delay(s);
    EXPECT_EQ(s.m_dwReleaseDelayTime, 0u);
}

// ---- Summon / Unsummon ----

TEST(PetManagerSummon, SummonExistingPet) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(42u, static_cast<std::uint16_t>(PetKind::EventPet)));
    EXPECT_TRUE(summon_pet(s, 42u));
    EXPECT_TRUE(has_cur_summon(s));
    EXPECT_EQ(s.m_wPetKind, static_cast<std::uint16_t>(PetKind::EventPet));
}

TEST(PetManagerSummon, SummonUnknownFails) {
    auto s = make_pet_manager();
    EXPECT_FALSE(summon_pet(s, 1u));
    EXPECT_FALSE(has_cur_summon(s));
}

TEST(PetManagerSummon, SummonSamePetTwiceFails) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u));
    EXPECT_TRUE(summon_pet(s, 1u));
    EXPECT_FALSE(summon_pet(s, 1u));
}

TEST(PetManagerSummon, UnsummonClears) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u));
    summon_pet(s, 1u);
    unsummon_pet(s);
    EXPECT_FALSE(has_cur_summon(s));
    EXPECT_EQ(s.m_wPetKind, 0u);
}

// ---- Feed ----

TEST(PetManagerFeed, WithoutSummonReturnsUnsummoned) {
    auto s = make_pet_manager();
    EXPECT_EQ(feed_up_pet(s, 10u, 100u), PetFeedResult::Unsummoned);
}

TEST(PetManagerFeed, FullStaminaReturnsStaminaFull) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, /*stamina*/1000u));
    summon_pet(s, 1u);
    EXPECT_EQ(feed_up_pet(s, 10u, 1000u), PetFeedResult::StaminaFull);
}

TEST(PetManagerFeed, FeedBelowMaxSucceeds) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, /*stamina*/500u));
    summon_pet(s, 1u);
    EXPECT_EQ(feed_up_pet(s, 100u, 1000u), PetFeedResult::Sucess);
    EXPECT_EQ(find_pet_total_info(s, 1u)->PetStamina, 600u);
}

// ---- Seal / Revival ----

TEST(PetManagerLifecycle, SealClearsAliveAndSummon) {
    auto s = make_pet_manager();
    add_pet_total_info(s, make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 100u));
    summon_pet(s, 1u);
    seal_pet(s);
    EXPECT_EQ(find_pet_total_info(s, 1u)->bAlive, 0u);
    EXPECT_FALSE(has_cur_summon(s));
}

TEST(PetManagerLifecycle, RevivalResetsFriendlyToRevival) {
    auto p = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 100u, /*friendly*/10u, /*alive*/0u);
    revival_pet(p);
    EXPECT_EQ(p.PetFriendly, PET_REVIVAL_FRIENDLY);
    EXPECT_EQ(p.bAlive, 1u);
}

// ---- Event pet timer ----

TEST(PetManagerEventPet, StartSetsRemainTime) {
    auto s = make_pet_manager();
    start_event_pet(s);
    EXPECT_EQ(s.m_dwEventPetSummonRemainTime, CRISTMAS_EVENTPET_SUMMONNING_TIME);
}

TEST(PetManagerEventPet, TickDecreasesRemaining) {
    auto s = make_pet_manager();
    start_event_pet(s);
    EXPECT_FALSE(tick_event_pet_remain(s, 60000u));
    EXPECT_EQ(s.m_dwEventPetSummonRemainTime, CRISTMAS_EVENTPET_SUMMONNING_TIME - 60000u);
}

TEST(PetManagerEventPet, TickToZeroReportsExpired) {
    auto s = make_pet_manager();
    start_event_pet(s);
    EXPECT_TRUE(tick_event_pet_remain(s, CRISTMAS_EVENTPET_SUMMONNING_TIME));
    EXPECT_EQ(s.m_dwEventPetSummonRemainTime, 0u);
}

// ---- Buff flag ----

TEST(PetManagerBuffFlag, AddSetsBit) {
    auto s = make_pet_manager();
    add_pet_buff_flag(s, PetBuffKind::DemagePercent);
    add_pet_buff_flag(s, PetBuffKind::Dodge);
    EXPECT_TRUE(has_pet_buff_flag(s, PetBuffKind::DemagePercent));
    EXPECT_TRUE(has_pet_buff_flag(s, PetBuffKind::Dodge));
    EXPECT_FALSE(has_pet_buff_flag(s, PetBuffKind::MasterAllStatUp));
}

TEST(PetManagerBuffFlag, RemoveClearsBit) {
    auto s = make_pet_manager();
    add_pet_buff_flag(s, PetBuffKind::ItemDoubleChance);
    remove_pet_buff_flag(s, PetBuffKind::ItemDoubleChance);
    EXPECT_FALSE(has_pet_buff_flag(s, PetBuffKind::ItemDoubleChance));
}

TEST(PetManagerBuffFlag, NoneKindIsNoOp) {
    auto s = make_pet_manager();
    add_pet_buff_flag(s, PetBuffKind::None);
    EXPECT_EQ(s.m_BuffFlag, 0);
}

// ---- Friendship protection ----

TEST(PetManagerProtection, ZeroRateLosesFullAmount) {
    EXPECT_EQ(apply_friendship_protection(1000u, 0.0f), 1000);
}

TEST(PetManagerProtection, HalfRateLosesHalf) {
    EXPECT_EQ(apply_friendship_protection(1000u, 0.5f), 500);
}

TEST(PetManagerProtection, FullRateLosesZero) {
    EXPECT_EQ(apply_friendship_protection(1000u, 1.0f), 0);
}

// ---- Buff data ----

TEST(PetManagerBuffData, SetAndGetByKind) {
    auto s = make_pet_manager();
    PetBuffData d;
    d.Prob = 50u;
    d.BuffValueData = 100u;
    set_buff_data(s, PetBuffKind::DemagePercent, d);
    auto got = get_buff_data(s, PetBuffKind::DemagePercent);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->Prob, 50u);
    EXPECT_EQ(got->BuffValueData, 100u);
}

TEST(PetManagerFriendship, FriendshipLossKillsPetAtZero) {
    auto pet = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 100u, 10u, 1u);
    add_friendship(pet, -10);
    EXPECT_EQ(pet.PetFriendly, 0u);
    EXPECT_EQ(pet.bAlive, 1u);
    add_friendship(pet, -1);
    EXPECT_EQ(pet.PetFriendly, 0u);
    EXPECT_EQ(pet.bAlive, 0u);
}

TEST(PetManagerFriendship, EventPetIgnoresFriendshipDelta) {
    auto pet = make_pet(1u, static_cast<std::uint16_t>(PetKind::EventPet), 1u, 100u, 10u, 1u);
    add_friendship(pet, -100);
    EXPECT_EQ(pet.PetFriendly, 10u);
    EXPECT_EQ(pet.bAlive, 1u);
}

TEST(PetManagerUpgrade, ProfileResetsStatsOnSuccess) {
    auto pet = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 10u, 123u, 1u);
    PetUpgradeProfile profile;
    profile.stamina_max = {100u, 250u, 500u};
    profile.default_friendship = 3000000u;
    profile.failure_friendship_loss = 10u;
    EXPECT_EQ(upgrade_pet_with_profile(pet, 0u, profile), PetUpgradeResult::UpgradeSucess);
    EXPECT_EQ(pet.PetGrade, 2u);
    EXPECT_EQ(pet.PetStamina, 250u);
    EXPECT_EQ(pet.PetFriendly, 3000000u);
}

TEST(PetManagerUpgrade, ProfileFailureReducesFriendshipAndCanKill) {
    auto pet = make_pet(1u, static_cast<std::uint16_t>(PetKind::CommonPet), 1u, 10u, 5u, 1u);
    PetUpgradeProfile profile;
    profile.failure_friendship_loss = 10u;
    EXPECT_EQ(upgrade_pet_with_profile(pet, 9999u, profile), PetUpgradeResult::UpgradeFailforProb);
    EXPECT_EQ(pet.PetFriendly, 0u);
    EXPECT_EQ(pet.bAlive, 0u);
}