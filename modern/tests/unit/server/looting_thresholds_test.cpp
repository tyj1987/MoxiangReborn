// looting_thresholds_test.cpp - 1:1 data-plane tests for the
// legacy CLootingManager::GetLootingChance / GetLootingItemNum /
// GetWearItemLootingRatio from [Server]Map/LootingManager.cpp.
// Locks all threshold boundaries for both locale variants.

#include <mxh/server/looting_thresholds.hpp>

#include <gtest/gtest.h>

#include <climits>
#include <cstdint>

using namespace mxh::server;

// ----- looting_chance: default locale (KR/CN/JP/TW) -----

TEST(LootingChanceDefault, Below100000Returns3) {
    EXPECT_EQ(looting_chance(0, LootingLocale::Default), 3);
    EXPECT_EQ(looting_chance(99999, LootingLocale::Default), 3);
}

TEST(LootingChanceDefault, At100000Returns4) {
    // boundary: < 100000 is tier 3; == 100000 is tier 4
    EXPECT_EQ(looting_chance(100000, LootingLocale::Default), 4);
    EXPECT_EQ(looting_chance(499999, LootingLocale::Default), 4);
}

TEST(LootingChanceDefault, EachTierUpTo10) {
    // Each tier uses strict-less-than. We test one representative
    // value per bucket. The boundary value falls into the next bucket.
    EXPECT_EQ(looting_chance(0,          LootingLocale::Default), 3);
    EXPECT_EQ(looting_chance(100000,     LootingLocale::Default), 4);
    EXPECT_EQ(looting_chance(500000,     LootingLocale::Default), 5);
    EXPECT_EQ(looting_chance(1000000,    LootingLocale::Default), 6);
    EXPECT_EQ(looting_chance(5000000,    LootingLocale::Default), 7);
    EXPECT_EQ(looting_chance(10000000,   LootingLocale::Default), 8);
    EXPECT_EQ(looting_chance(50000000,   LootingLocale::Default), 9);
    EXPECT_EQ(looting_chance(100000000,  LootingLocale::Default), 10);
    EXPECT_EQ(looting_chance(UINT32_MAX, LootingLocale::Default), 10);
}

TEST(LootingChanceDefault, EachBoundaryIsStrictLessThan) {
    // boundary value falls into next bucket
    EXPECT_EQ(looting_chance(49999999, LootingLocale::Default), 8);
    EXPECT_EQ(looting_chance(50000000, LootingLocale::Default), 9);
    EXPECT_EQ(looting_chance(99999999, LootingLocale::Default), 9);
    EXPECT_EQ(looting_chance(100000000, LootingLocale::Default), 10);
}

// ----- looting_chance: HK non-TW (tight) -----

TEST(LootingChanceHk, TightThresholds) {
    EXPECT_EQ(looting_chance(0, LootingLocale::Hk), 3);
    EXPECT_EQ(looting_chance(999, LootingLocale::Hk), 3);
    EXPECT_EQ(looting_chance(1000, LootingLocale::Hk), 4);
    EXPECT_EQ(looting_chance(4999, LootingLocale::Hk), 5);
    EXPECT_EQ(looting_chance(19999, LootingLocale::Hk), 7);
    EXPECT_EQ(looting_chance(99999, LootingLocale::Hk), 9);
    EXPECT_EQ(looting_chance(100000, LootingLocale::Hk), 10);
    EXPECT_EQ(looting_chance(UINT32_MAX, LootingLocale::Hk), 10);
}

// ----- looting_item_num: HK non-TW -----

TEST(LootingItemNumHk, FiveBuckets) {
    EXPECT_EQ(looting_item_num(0, LootingLocale::Hk), 1);
    EXPECT_EQ(looting_item_num(99999, LootingLocale::Hk), 1);
    EXPECT_EQ(looting_item_num(100000, LootingLocale::Hk), 2);
    EXPECT_EQ(looting_item_num(499999, LootingLocale::Hk), 3);
    EXPECT_EQ(looting_item_num(999999, LootingLocale::Hk), 4);
    EXPECT_EQ(looting_item_num(1000000, LootingLocale::Hk), 5);
    EXPECT_EQ(looting_item_num(UINT32_MAX, LootingLocale::Hk), 5);
}

// ----- looting_item_num: default (KR/CN/JP/TW) -----

TEST(LootingItemNumDefault, ZeroFameReturnsZero) {
    // legacy: bad_fame < 50 -> 0 (KR/CN/JP path)
    EXPECT_EQ(looting_item_num(0, LootingLocale::Default), 0);
    EXPECT_EQ(looting_item_num(49, LootingLocale::Default), 0);
}

TEST(LootingItemNumDefault, Boundary50Returns1) {
    // < 50 -> 0; == 50 falls into the < 100000000 bucket -> 1
    EXPECT_EQ(looting_item_num(50, LootingLocale::Default), 1);
}

TEST(LootingItemNumDefault, EachBucketUpTo5) {
    EXPECT_EQ(looting_item_num(100000000, LootingLocale::Default), 2);
    EXPECT_EQ(looting_item_num(400000000, LootingLocale::Default), 3);
    EXPECT_EQ(looting_item_num(700000000, LootingLocale::Default), 4);
    EXPECT_EQ(looting_item_num(1000000000, LootingLocale::Default), 5);
    EXPECT_EQ(looting_item_num(UINT32_MAX, LootingLocale::Default), 5);
}

// ----- wear_item_looting_ratio (locale-agnostic) -----

TEST(WearItemLootingRatio, ZeroFameIsZero) {
    EXPECT_EQ(wear_item_looting_ratio(0), 0);
}

TEST(WearItemLootingRatio, Sub50IsOne) {
    EXPECT_EQ(wear_item_looting_ratio(1), 1);
    EXPECT_EQ(wear_item_looting_ratio(49), 1);
}

TEST(WearItemLootingRatio, Boundary50Returns10) {
    EXPECT_EQ(wear_item_looting_ratio(50), 10);
    EXPECT_EQ(wear_item_looting_ratio(3999), 10);
}

TEST(WearItemLootingRatio, EachBucketUpTo100) {
    EXPECT_EQ(wear_item_looting_ratio(4000), 20);
    EXPECT_EQ(wear_item_looting_ratio(20000), 30);
    EXPECT_EQ(wear_item_looting_ratio(80000), 40);
    EXPECT_EQ(wear_item_looting_ratio(400000), 50);
    EXPECT_EQ(wear_item_looting_ratio(1600000), 60);
    EXPECT_EQ(wear_item_looting_ratio(8000000), 70);
    EXPECT_EQ(wear_item_looting_ratio(32000000), 85);
    EXPECT_EQ(wear_item_looting_ratio(100000000), 100);
    EXPECT_EQ(wear_item_looting_ratio(500000000), 100);
    EXPECT_EQ(wear_item_looting_ratio(UINT32_MAX), 100);
}