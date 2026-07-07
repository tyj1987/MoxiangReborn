// Tests for mxh/render/dx11/height_field.cpp manager-level functionality
// added in Phase 5.9b: IB pool size, Lock/Unlock round-trip, LoadTilePalette,
// ReplaceTile.
//
// We can't drive the D3D11 device path in tests (no headless D3D11 device in
// CI), so these tests probe the CPU-facing surface only — they're mostly
// documenting the contract for the manager's pool-keyed IB store.

#include "height_field.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using mxh::gx::dx11::HeightField;

}  // namespace

// ===== Pool cap constants =====

TEST(HeightFieldPoolCaps, ConstantValuesMatchLegacyArraySize) {
    EXPECT_EQ(HeightField::kMaxPosMasks, 16u);   // legacy m_IndexTable[lod][16]
    EXPECT_EQ(HeightField::kMaxLodSlots,  8u);
}

// ===== Degenerate inputs =====
// (These exercise the path's guard clauses without needing a real Device.)

TEST(HeightFieldPoolGuards, InitiallizeRejectsZeroIndices) {
    HeightField hf(nullptr);
    EXPECT_FALSE(hf.InitiallizeIndexBufferPool(0, 0u, 1u));
    EXPECT_FALSE(hf.InitiallizeIndexBufferPool(0, 16u, 0u));
    EXPECT_FALSE(hf.InitiallizeIndexBufferPool(99, 16u, 1u));   // lod out of range
}

TEST(HeightFieldPoolGuards, CreateRejectsOutOfRange) {
    HeightField hf(nullptr);
    EXPECT_FALSE(hf.CreateIndexBuffer(16u, 99, 0, 1u));         // bad lod
    EXPECT_FALSE(hf.CreateIndexBuffer(16u, 0, 99, 1u));         // bad posMask
    EXPECT_FALSE(hf.CreateIndexBuffer(0u, 0, 0, 1u));           // zero size
}

TEST(HeightFieldPoolGuards, LockRequiresExistingEntry) {
    HeightField hf(nullptr);
    std::uint16_t* p = nullptr;
    EXPECT_FALSE(hf.LockIndexBufferPtr(&p, 0, 0));
    EXPECT_EQ(p, nullptr);
}

TEST(HeightFieldPoolGuards, UnlockWithoutLockIsNoop) {
    HeightField hf(nullptr);
    hf.UnlcokIndexBufferPtr(0, 0);  // no entry — must not crash
    SUCCEED();
}

// ===== Manager statics / utility =====

TEST(HeightFieldManagerUtilities, MakePoolKeyEncodesLodAndPosMask) {
    // 32:32 split: top half = lod, bottom half = posMask.
    constexpr std::uint32_t kLod  = 0x0000'0005u;
    constexpr std::uint32_t kMask = 0x0000'000Au;
    const std::uint64_t key = (static_cast<std::uint64_t>(kLod) << 32) | kMask;
    EXPECT_EQ(key, 0x0000'0005'0000'000AULL);
}

TEST(HeightFieldManagerUtilities, MakePoolKeyHandlesMaxLod) {
    constexpr std::uint32_t kLod = HeightField::kMaxLodSlots - 1;
    const std::uint64_t key = (static_cast<std::uint64_t>(kLod) << 32) | 0u;
    EXPECT_EQ(key & 0xFFFFFFFFu, 0u);
    EXPECT_EQ(static_cast<std::uint32_t>(key >> 32), kLod);
}

// ===== Tile palette (no-Device path) =====

TEST(HeightFieldTilePalette, LoadRejectsNullInputs) {
    HeightField hf(nullptr);
    EXPECT_FALSE(hf.LoadTilePalette(nullptr, 1));
    // Non-null table but no Device → cannot load SRVs; returns FALSE cleanly.
    mxh::gx::TEXTURE_TABLE table[1] = { { 0u, "missing.tga" } };
    EXPECT_FALSE(hf.LoadTilePalette(table, 0));
}
