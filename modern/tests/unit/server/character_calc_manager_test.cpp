// character_calc_manager_test.cpp - 1:1 unit tests for the modern
// CharacterCalcManager port. Locks the formula byte-for-byte against the
// legacy [Server]Map/CharacterCalcManager.cpp paths.

#include "mxh/server/character_calc_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::CalcBaseStats;
using mxh::server::CalcEquipBonuses;
using mxh::server::MussangStage;
using mxh::server::RecoveryResult;
using mxh::server::compute_max_life;
using mxh::server::compute_max_shield;
using mxh::server::compute_max_naeryuk;
using mxh::server::tick_life_recovery;
using mxh::server::tick_shield_recovery;
using mxh::server::tick_naeryuk_recovery;
using mxh::server::tick_life_ungi;
using mxh::server::tick_shield_ungi;
using mxh::server::tick_naeryuk_ungi;
}

// ---- compute_max_life ----
TEST(MaxLife, Level1CheRyuk10) {
    CalcBaseStats s; s.level = 1; s.cheryuk = 10;
    EXPECT_EQ(compute_max_life(s, CalcEquipBonuses{}), 105u);  // 1*5 + 10*10
}

TEST(MaxLife, Level50CheRyuk100) {
    CalcBaseStats s; s.level = 50; s.cheryuk = 100;
    EXPECT_EQ(compute_max_life(s, CalcEquipBonuses{}), 1250u);  // 50*5 + 100*10
}

TEST(MaxLife, AllBonuses) {
    CalcBaseStats s; s.level = 30; s.cheryuk = 50;
    CalcEquipBonuses b;
    b.item_max_life = 100;
    b.set_dw_life = 50;
    b.ability_life_up = 20;
    b.shop_life = 5;
    b.avatar_life = 3;
    b.skill_life = 7;
    // base = 30*5 + 50*10 = 650; +185 = 835
    EXPECT_EQ(compute_max_life(s, b), 835u);
}

TEST(MaxLife, UniqueNegativeClampedTo1) {
    CalcBaseStats s; s.level = 1; s.cheryuk = 0;
    CalcEquipBonuses b; b.unique_n_hp = -10000;
    // base = 5; + (-10000) = -9995; clamp to 1
    EXPECT_EQ(compute_max_life(s, b), 1u);
}

TEST(MaxLife, UniquePositiveAdded) {
    CalcBaseStats s; s.level = 10; s.cheryuk = 5;
    CalcEquipBonuses b; b.unique_n_hp = 500;
    // base = 10*5 + 5*10 = 100; +500 = 600
    EXPECT_EQ(compute_max_life(s, b), 600u);
}

// ---- compute_max_shield ----
TEST(MaxShield, Level1Simmek10Gengol5Minchub5) {
    CalcBaseStats s; s.level = 1; s.simmek = 10; s.gengol = 5; s.minchub = 5;
    // 1*5 + 10*10 + 5*5 + 5*5 = 5 + 100 + 25 + 25 = 155
    EXPECT_EQ(compute_max_shield(s, CalcEquipBonuses{}), 155u);
}

TEST(MaxShield, Level99AllZero) {
    CalcBaseStats s; s.level = 99;
    EXPECT_EQ(compute_max_shield(s, CalcEquipBonuses{}), 99u * 5);  // 495
}

TEST(MaxShield, AllBonuses) {
    CalcBaseStats s; s.level = 20; s.simmek = 30; s.gengol = 10; s.minchub = 10;
    CalcEquipBonuses b;
    b.item_max_shield = 50;
    b.set_dw_shield = 30;
    b.ability_shield_up = 20;
    b.shop_shield = 5;
    b.avatar_shield = 3;
    b.skill_shield = 7;
    // base = 20*5 + 30*10 + 10*5 + 10*5 = 100 + 300 + 50 + 50 = 500
    // +115 = 615
    EXPECT_EQ(compute_max_shield(s, b), 615u);
}

TEST(MaxShield, UniqueNegativeClampedTo1) {
    CalcBaseStats s; s.level = 1;
    CalcEquipBonuses b; b.unique_n_shield = -1000;
    EXPECT_EQ(compute_max_shield(s, b), 1u);
}

// ---- compute_max_naeryuk ----
TEST(MaxNaeRyuk, Level1Simmek10) {
    CalcBaseStats s; s.level = 1; s.simmek = 10;
    // 1*5 + 10*10 = 105
    EXPECT_EQ(compute_max_naeryuk(s, CalcEquipBonuses{}), 105u);
}

TEST(MaxNaeRyuk, AllBonuses) {
    CalcBaseStats s; s.level = 30; s.simmek = 40;
    CalcEquipBonuses b;
    b.item_max_naeryuk = 50;
    b.set_dw_naeryuk = 30;
    b.ability_naeryuk_up = 20;
    b.shop_naeryuk = 5;
    b.avatar_naeryuk = 3;
    b.skill_naeryuk = 7;
    // base = 30*5 + 40*10 = 550
    // +115 = 665
    EXPECT_EQ(compute_max_naeryuk(s, b), 665u);
}

TEST(MaxNaeRyuk, UniqueNegativeClampedTo1) {
    CalcBaseStats s; s.level = 1; s.simmek = 0;
    CalcEquipBonuses b; b.unique_n_mp = -1000;
    EXPECT_EQ(compute_max_naeryuk(s, b), 1u);
}

// ---- tick_life_recovery ----
TEST(TickLifeRecovery, TickNotElapsed) {
    auto r = tick_life_recovery(1000, 0, 500, 1000, 0, false, MussangStage::Normal, 0);
    EXPECT_FALSE(r.updated);
    EXPECT_EQ(r.new_value, 500u);
}

TEST(TickLifeRecovery, FullHpNoChange) {
    auto r = tick_life_recovery(10000, 0, 1000, 1000, 0, false, MussangStage::Normal, 0);
    EXPECT_FALSE(r.updated);  // already full, base case short-circuits
    EXPECT_EQ(r.new_value, 1000u);
}

TEST(TickLifeRecovery, BaseOnePercent) {
    auto r = tick_life_recovery(10000, 0, 500, 1000, 0, false, MussangStage::Normal, 0);
    // base = 500 + 1000*0.01 = 510
    EXPECT_TRUE(r.updated);
    EXPECT_EQ(r.new_value, 510u);
    EXPECT_EQ(r.new_check_time, 10000u);
}

TEST(TickLifeRecovery, MussangNormalStage_15x) {
    auto r = tick_life_recovery(10000, 0, 500, 1000, 0, true, MussangStage::Normal, 0);
    // base = 500 + 1000*0.015 = 515
    EXPECT_TRUE(r.updated);
    EXPECT_EQ(r.new_value, 515u);
}

TEST(TickLifeRecovery, MussangHyunStage_20x) {
    auto r = tick_life_recovery(10000, 0, 500, 1000, 0, true, MussangStage::Hyun, 0);
    EXPECT_EQ(r.new_value, 520u);
}

TEST(TickLifeRecovery, WithUpLife) {
    auto r = tick_life_recovery(10000, 0, 0, 1000, 50, false, MussangStage::Normal, 0);
    // base = 0 + 1000*0.01 + 50 = 60
    EXPECT_EQ(r.new_value, 60u);
}

TEST(TickLifeRecovery, WithRecoverRate_50Percent) {
    auto r = tick_life_recovery(10000, 0, 500, 1000, 0, false, MussangStage::Normal, 50);
    // base = 500 + 10 = 510; *= 0.5 = 255
    EXPECT_EQ(r.new_value, 255u);
}

TEST(TickLifeRecovery, ClampedToMax) {
    auto r = tick_life_recovery(10000, 0, 999, 1000, 1000, true, MussangStage::Hyun, 0);
    // base = 999 + 20 + 1000 = 2019; clamp to 1000
    EXPECT_EQ(r.new_value, 1000u);
}

TEST(TickShieldRecovery, BaseOnePercent) {
    auto r = tick_shield_recovery(10000, 0, 500, 1000, 0, false, MussangStage::Normal, 0);
    EXPECT_EQ(r.new_value, 510u);
}

TEST(TickShieldRecovery, MussangHyunStage_20x) {
    auto r = tick_shield_recovery(10000, 0, 500, 1000, 0, true, MussangStage::Hyun, 0);
    EXPECT_EQ(r.new_value, 520u);
}

TEST(TickNaeRyukRecovery, BaseOnePercent) {
    auto r = tick_naeryuk_recovery(10000, 0, 500, 1000, 0, false, MussangStage::Normal, 0);
    EXPECT_EQ(r.new_value, 510u);
}

TEST(TickNaeRyukRecovery, MussangHwaStage_15x) {
    auto r = tick_naeryuk_recovery(10000, 0, 500, 1000, 0, true, MussangStage::Hwa, 0);
    EXPECT_EQ(r.new_value, 515u);
}

// ---- tick_life_ungi ----
TEST(TickLifeUngi, TickNotElapsed) {
    auto r = tick_life_ungi(1000, 0, 500, 1000, 0, 0.0f, 1.0f, false);
    EXPECT_FALSE(r.updated);
}

TEST(TickLifeUngi, Base5SecTick) {
    auto r = tick_life_ungi(6000, 0, 0, 1000, 0, 0.0f, 1.0f, false);
    // base = 10 + 1000*0.03 + 0 = 40 (legacy has +10 flat)
    EXPECT_TRUE(r.updated);
    EXPECT_EQ(r.new_value, 40u);
}

TEST(TickLifeUngi, WithUngiUpVal) {
    auto r = tick_life_ungi(6000, 0, 0, 1000, 50, 0.0f, 1.0f, false);
    // base = 10 + 30 + 50 = 90
    EXPECT_EQ(r.new_value, 90u);
}

TEST(TickLifeUngi, WithPlusRate) {
    auto r = tick_life_ungi(6000, 0, 0, 1000, 0, 0.05f, 1.0f, false);
    // base = 10 + 30 + 1000*0.05 = 10 + 30 + 50 = 90
    EXPECT_EQ(r.new_value, 90u);
}

TEST(TickLifeUngi, SnowHalvesPeriod) {
    auto r = tick_life_ungi(3000, 0, 0, 1000, 0, 0.0f, 1.0f, true);
    // period = 5000 * 0.5 = 2500; 3000 > 2500: tick fires
    EXPECT_TRUE(r.updated);
}

TEST(TickLifeUngi, SnowPeriodNotElapsed) {
    auto r = tick_life_ungi(2000, 0, 0, 1000, 0, 0.0f, 1.0f, true);
    // period = 2500; 2000 < 2500: no tick
    EXPECT_FALSE(r.updated);
}

TEST(TickLifeUngi, FastUngiSpeed) {
    auto r = tick_life_ungi(3000, 0, 0, 1000, 0, 0.0f, 2.0f, false);
    // period = 5000 * 0.5 = 2500; 3000 > 2500: tick fires
    EXPECT_TRUE(r.updated);
}

TEST(TickLifeUngi, ClampedToMax) {
    auto r = tick_life_ungi(6000, 0, 999, 1000, 1000, 0.0f, 1.0f, false);
    // base = 999 + 30 + 1000 = 2029; clamp to 1000
    EXPECT_EQ(r.new_value, 1000u);
}

TEST(TickShieldUngi, Base5SecTick) {
    auto r = tick_shield_ungi(6000, 0, 0, 1000, 0, 0.0f, 1.0f, false);
    EXPECT_EQ(r.new_value, 40u);
}

TEST(TickShieldUngi, WithUngiUpVal) {
    auto r = tick_shield_ungi(6000, 0, 0, 1000, 50, 0.0f, 1.0f, false);
    EXPECT_EQ(r.new_value, 90u);
}

TEST(TickNaeRyukUngi, Base5SecTick) {
    auto r = tick_naeryuk_ungi(6000, 0, 0, 1000, 0, 0.0f, 1.0f, false, false, MussangStage::Normal);
    EXPECT_EQ(r.new_value, 40u);
}

TEST(TickNaeRyukUngi, WithMussangMode_15x) {
    auto r = tick_naeryuk_ungi(6000, 0, 0, 1000, 0, 0.0f, 1.0f, false, true, MussangStage::Normal);
    // base = 40 * 1.5 = 60
    EXPECT_EQ(r.new_value, 60u);
}

TEST(TickNaeRyukUngi, WithMussangMode_20x) {
    auto r = tick_naeryuk_ungi(6000, 0, 0, 1000, 0, 0.0f, 1.0f, false, true, MussangStage::Hyun);
    // base = 40 * 2.0 = 80
    EXPECT_EQ(r.new_value, 80u);
}

