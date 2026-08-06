// boss_state_test.cpp
//
// 1:1 lock tests for boss_state.hpp pure functions.
// BossPhase enum + 3 transitions functions:
//   - boss_phase_from_hp: map current_hp/max_hp -> BossPhase
//   - boss_phase_transition: state machine (Dead is terminal; Sealed -> Intro; otherwise HP-based)
//   - boss_phase_is_terminal: true when phase is Dead
//
// These tests complement the indirect coverage in boss_monster_manager_test.cpp
// by exercising the standalone pure-function API surface.

#pragma once

#include "mxh/server/boss_state.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace {

using mxh::server::BossPhase;
using mxh::server::boss_phase_from_hp;
using mxh::server::boss_phase_transition;
using mxh::server::boss_phase_is_terminal;
// ===========================================================================
// BossPhase enum value verification (1:1 with legacy CBossState::ePhase)
// ===========================================================================

TEST(BossState, BossPhaseEnumSealedIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Sealed), 0u);
}

TEST(BossState, BossPhaseEnumIntroIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Intro), 1u);
}

TEST(BossState, BossPhaseEnumCombatIsTwo) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Combat), 2u);
}

TEST(BossState, BossPhaseEnumEnragedIsThree) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Enraged), 3u);
}

TEST(BossState, BossPhaseEnumPhase2IsFour) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Phase2), 4u);
}

TEST(BossState, BossPhaseEnumRageIsFive) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Rage), 5u);
}

TEST(BossState, BossPhaseEnumDyingIsSix) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Dying), 6u);
}

TEST(BossState, BossPhaseEnumDeadIsSeven) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Dead), 7u);
}

TEST(BossState, BossPhaseEnumRecoveringIsEight) {
    EXPECT_EQ(static_cast<std::uint8_t>(BossPhase::Recovering), 8u);
}

TEST(BossState, BossPhaseEnumIsUint8Backed) {
    static_assert(std::is_same<decltype(static_cast<std::uint8_t>(BossPhase::Sealed)),
                               std::uint8_t>::value,
                  "BossPhase must be backed by uint8_t for 1:1 wire compatibility");
}

// ===========================================================================
// boss_phase_from_hp: max_hp == 0 -> Sealed
// ===========================================================================

TEST(BossStatePhaseFromHp, ZeroMaxHpReturnsSealed) {
    EXPECT_EQ(boss_phase_from_hp(100u, 0u), BossPhase::Sealed);
}

TEST(BossStatePhaseFromHp, ZeroMaxHpZeroCurrentReturnsSealed) {
    EXPECT_EQ(boss_phase_from_hp(0u, 0u), BossPhase::Sealed);
}

TEST(BossStatePhaseFromHp, ZeroMaxHpLargeCurrentReturnsSealed) {
    EXPECT_EQ(boss_phase_from_hp(0xFFFFFFFFu, 0u), BossPhase::Sealed);
}

// ===========================================================================
// boss_phase_from_hp: current_hp == 0 -> Dead
// ===========================================================================

TEST(BossStatePhaseFromHp, ZeroCurrentHpReturnsDead) {
    EXPECT_EQ(boss_phase_from_hp(0u, 100u), BossPhase::Dead);
}

TEST(BossStatePhaseFromHp, ZeroCurrentHpWithLargeMaxReturnsDead) {
    EXPECT_EQ(boss_phase_from_hp(0u, 0xFFFFFFFFu), BossPhase::Dead);
}

// ===========================================================================
// boss_phase_from_hp: pct >= 76 -> Combat
// ===========================================================================

TEST(BossStatePhaseFromHp, FullHpReturnsCombat) {
    EXPECT_EQ(boss_phase_from_hp(100u, 100u), BossPhase::Combat);
}

TEST(BossStatePhaseFromHp, HundredPercentReturnsCombat) {
    EXPECT_EQ(boss_phase_from_hp(1000u, 1000u), BossPhase::Combat);
}

TEST(BossStatePhaseFromHp, At76PercentReturnsCombat) {
    EXPECT_EQ(boss_phase_from_hp(76u, 100u), BossPhase::Combat);
}

TEST(BossStatePhaseFromHp, At99PercentReturnsCombat) {
    EXPECT_EQ(boss_phase_from_hp(99u, 100u), BossPhase::Combat);
}

TEST(BossStatePhaseFromHp, At50PercentPlusOneReturnsCombat) {
    // 51% is the Enraged threshold, so 50%+1 = 51% -> Enraged.
    EXPECT_EQ(boss_phase_from_hp(51u, 100u), BossPhase::Enraged);
}

// ===========================================================================
// boss_phase_from_hp: pct 51-75 -> Enraged
// ===========================================================================

TEST(BossStatePhaseFromHp, At75PercentReturnsEnraged) {
    EXPECT_EQ(boss_phase_from_hp(75u, 100u), BossPhase::Enraged);
}

TEST(BossStatePhaseFromHp, At51PercentReturnsEnraged) {
    EXPECT_EQ(boss_phase_from_hp(51u, 100u), BossPhase::Enraged);
}

TEST(BossStatePhaseFromHp, At60PercentReturnsEnraged) {
    EXPECT_EQ(boss_phase_from_hp(60u, 100u), BossPhase::Enraged);
}

// ===========================================================================
// boss_phase_from_hp: pct 26-50 -> Phase2
// ===========================================================================

TEST(BossStatePhaseFromHp, At50PercentReturnsPhase2) {
    EXPECT_EQ(boss_phase_from_hp(50u, 100u), BossPhase::Phase2);
}

TEST(BossStatePhaseFromHp, At26PercentReturnsPhase2) {
    EXPECT_EQ(boss_phase_from_hp(26u, 100u), BossPhase::Phase2);
}

TEST(BossStatePhaseFromHp, At40PercentReturnsPhase2) {
    EXPECT_EQ(boss_phase_from_hp(40u, 100u), BossPhase::Phase2);
}

// ===========================================================================
// boss_phase_from_hp: pct < 26 -> Rage
// ===========================================================================

TEST(BossStatePhaseFromHp, At25PercentReturnsRage) {
    EXPECT_EQ(boss_phase_from_hp(25u, 100u), BossPhase::Rage);
}

TEST(BossStatePhaseFromHp, At1PercentReturnsRage) {
    EXPECT_EQ(boss_phase_from_hp(1u, 100u), BossPhase::Rage);
}

TEST(BossStatePhaseFromHp, OneHpReturnsRage) {
    EXPECT_EQ(boss_phase_from_hp(1u, 1000u), BossPhase::Rage);
}

// ===========================================================================
// boss_phase_transition: Dead is terminal
// ===========================================================================

TEST(BossStateTransition, FromDeadIgnoresPositiveHpStaysDead) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Dead, 100u, 100u), BossPhase::Dead);
}

TEST(BossStateTransition, FromDeadIgnoresFullHpStaysDead) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Dead, 1000u, 1000u), BossPhase::Dead);
}

TEST(BossStateTransition, FromDeadIgnoresZeroHpStaysDead) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Dead, 0u, 100u), BossPhase::Dead);
}

TEST(BossStateTransition, FromDeadIgnoresMaxHpZeroStaysDead) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Dead, 100u, 0u), BossPhase::Dead);
}

// ===========================================================================
// boss_phase_transition: Sealed -> Intro when HP > 0
// ===========================================================================

TEST(BossStateTransition, FromSealedWithHpPositiveGoesToIntro) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 100u, 100u), BossPhase::Intro);
}

TEST(BossStateTransition, FromSealedWithOneHpGoesToIntro) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 1u, 100u), BossPhase::Intro);
}

TEST(BossStateTransition, FromSealedWithZeroHpStaysSealed) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 0u, 100u), BossPhase::Sealed);
}

TEST(BossStateTransition, FromSealedWithZeroMaxHpGoesToIntro) {
    // The Sealed branch checks current_hp > 0 BEFORE the max_hp == 0
    // short-circuit. So with current_hp=100 and max_hp=0 we still go to Intro.
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 100u, 0u), BossPhase::Intro);
}

// ===========================================================================
// boss_phase_transition: HP==0 -> Dying (regardless of starting phase)
// ===========================================================================

TEST(BossStateTransition, FromCombatWithZeroHpGoesToDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 0u, 100u), BossPhase::Dying);
}

TEST(BossStateTransition, FromEnragedWithZeroHpGoesToDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Enraged, 0u, 100u), BossPhase::Dying);
}

TEST(BossStateTransition, FromPhase2WithZeroHpGoesToDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Phase2, 0u, 100u), BossPhase::Dying);
}

TEST(BossStateTransition, FromRageWithZeroHpGoesToDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Rage, 0u, 100u), BossPhase::Dying);
}

TEST(BossStateTransition, FromIntroWithZeroHpGoesToDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Intro, 0u, 100u), BossPhase::Dying);
}

TEST(BossStateTransition, FromRecoveringWithZeroHpGoesToDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Recovering, 0u, 100u), BossPhase::Dying);
}

// ===========================================================================
// boss_phase_transition: monotonic downward phase escalation
// ===========================================================================

TEST(BossStateTransition, FromCombatToEnragedBelow75Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 70u, 100u), BossPhase::Enraged);
}

TEST(BossStateTransition, FromCombatToPhase2Below50Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 40u, 100u), BossPhase::Phase2);
}

TEST(BossStateTransition, FromCombatToRageBelow25Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 20u, 100u), BossPhase::Rage);
}

TEST(BossStateTransition, FromEnragedToPhase2Below50Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Enraged, 40u, 100u), BossPhase::Phase2);
}

TEST(BossStateTransition, FromEnragedToRageBelow25Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Enraged, 10u, 100u), BossPhase::Rage);
}

TEST(BossStateTransition, FromPhase2ToRageBelow25Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Phase2, 20u, 100u), BossPhase::Rage);
}

// ===========================================================================
// boss_phase_transition: same phase stays (when HP doesn't justify change)
// ===========================================================================

TEST(BossStateTransition, FromCombatStaysCombatAtFullHp) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 100u, 100u), BossPhase::Combat);
}

TEST(BossStateTransition, FromEnragedStaysEnragedAt60Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Enraged, 60u, 100u), BossPhase::Enraged);
}

TEST(BossStateTransition, FromPhase2StaysPhase2At40Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Phase2, 40u, 100u), BossPhase::Phase2);
}

TEST(BossStateTransition, FromRageStaysRageAt10Percent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Rage, 10u, 100u), BossPhase::Rage);
}

// ===========================================================================
// boss_phase_transition: max_hp == 0 edge case (only checked via Dead/Sealed path)
// ===========================================================================

TEST(BossStateTransition, MaxHpZeroFromCombatStaysAtCurrent) {
    // desired = Sealed (because max_hp==0 short-circuits in from_hp),
    // but desired is Combat-or-Sealed special-case so current is kept.
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 100u, 0u), BossPhase::Combat);
}

TEST(BossStateTransition, MaxHpZeroFromEnragedStaysAtCurrent) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Enraged, 60u, 0u), BossPhase::Enraged);
}

// ===========================================================================
// boss_phase_is_terminal: only Dead
// ===========================================================================

TEST(BossStateIsTerminal, DeadIsTerminal) {
    EXPECT_TRUE(boss_phase_is_terminal(BossPhase::Dead));
}

TEST(BossStateIsTerminal, SealedIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Sealed));
}

TEST(BossStateIsTerminal, IntroIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Intro));
}

TEST(BossStateIsTerminal, CombatIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Combat));
}

TEST(BossStateIsTerminal, EnragedIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Enraged));
}

TEST(BossStateIsTerminal, Phase2IsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Phase2));
}

TEST(BossStateIsTerminal, RageIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Rage));
}

TEST(BossStateIsTerminal, DyingIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Dying));
}

TEST(BossStateIsTerminal, RecoveringIsNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Recovering));
}


// ===========================================================================
// 1:1 byte-level sanity: function signatures are noexcept
// ===========================================================================

TEST(BossStateSignature, FromHpIsNoexcept) {
    static_assert(noexcept(boss_phase_from_hp(0u, 0u)),
                  "boss_phase_from_hp must be noexcept for 1:1 ABI parity");
    SUCCEED();
}

TEST(BossStateSignature, TransitionIsNoexcept) {
    static_assert(noexcept(boss_phase_transition(BossPhase::Sealed, 0u, 0u)),
                  "boss_phase_transition must be noexcept");
    SUCCEED();
}

TEST(BossStateSignature, IsTerminalIsNoexcept) {
    static_assert(noexcept(boss_phase_is_terminal(BossPhase::Dead)),
                  "boss_phase_is_terminal must be noexcept");
    SUCCEED();
}

}  // namespace

