// discard_avatar_item_test.cpp - 1:1 data-plane tests for the
// legacy CShopItemManager::DiscardAvatarItem data-plane half
// from [Server]Map/ShopItemManager.cpp. Locks the 4 no-op
// conditions + the clear-and-default-fill semantics across
// the 6-slot [Weared_Hair, Weared_Gum) default-fill range.

#include <mxh/server/discard_avatar_item.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

static AvatarEquipRow make_equip(std::uint8_t pos,
                              std::initializer_list<std::uint16_t> mask) {
    AvatarEquipRow e;
    e.position = pos;
    std::size_t i = 0;
    for (std::uint16_t v : mask) {
        if (i >= EAvatarCount) break;
        e.item[i++] = v;
    }
    return e;
}

static std::array<std::uint16_t, EAvatarCount> zero_avatar() {
    return {};
}

// ----- no-op conditions -----

TEST(DiscardAvatarItem, NullEquipIsNoOp) {
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(nullptr, 100, av);
    EXPECT_EQ(out, av);
}

TEST(DiscardAvatarItem, PositionOutOfRangeIsNoOp) {
    AvatarEquipRow e = make_equip(/*pos=*/99, {});
    auto av = zero_avatar();
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);
}

TEST(DiscardAvatarItem, MismatchedItemIdxIsNoOp) {
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[5] = 999;  // wrong item
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);
}

TEST(DiscardAvatarItem, ZeroInPositionIsNoOp) {
    // Legacy: if (pAvatar[Position] != ItemIdx) return;
    // av[pos] == 0 != ItemIdx, so no-op.
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();  // av[5] = 0
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);
}

// ----- clear-and-default-fill happy path -----

TEST(DiscardAvatarItem, MatchingSlotIsCleared) {
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[5], 0u);
}

TEST(DiscardAvatarItem, DefaultFillZerosBecomeOnes) {
    // Position 5 is a cosmetic slot, no default-fill there.
    AvatarEquipRow e = make_equip(/*pos=*/5, {0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0});
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(&e, 100, av);
    // pos[5] cleared, all 6 weared slots (12..17) filled to 1
    EXPECT_EQ(out[5], 0u);
    for (std::size_t n = 12; n < 18; ++n) {
        EXPECT_EQ(out[n], 1u);
    }
}

TEST(DiscardAvatarItem, NonZeroMaskedSlotsPreserve) {
    // equip.Item[12] = 5 (non-zero) -> avatar[12] should NOT be
    // overwritten to 1; legacy only fills when mask == 0.
    std::array<std::uint16_t, EAvatarCount> mask{};
    mask[12] = 5;
    AvatarEquipRow e;
    e.position = 5;
    e.item = mask;
    auto av = zero_avatar();
    av[5] = 100;
    av[12] = 999;  // pre-existing value
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[5], 0u);
    EXPECT_EQ(out[12], 999u);  // preserved
    for (std::size_t n = 13; n < 18; ++n) {
        EXPECT_EQ(out[n], 1u);
    }
}

TEST(DiscardAvatarItem, AllMaskedSlotsPreserve) {
    // All 6 weared slots have non-zero mask -> nothing changes
    // beyond the clear of position.
    std::array<std::uint16_t, EAvatarCount> mask{};
    for (std::size_t n = 12; n < 18; ++n) mask[n] = 100;
    AvatarEquipRow e;
    e.position = 5;
    e.item = mask;
    auto av = zero_avatar();
    av[5] = 100;
    for (std::size_t n = 12; n < 18; ++n) av[n] = 200;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[5], 0u);
    for (std::size_t n = 12; n < 18; ++n) {
        EXPECT_EQ(out[n], 200u);
    }
}

TEST(DiscardAvatarItem, NonWearedSlotsNotTouched) {
    // Cosmetic slots (0..11) outside position are never touched.
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[0] = 1000;
    av[1] = 1001;
    av[5] = 100;  // discarded
    av[11] = 1011;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[0], 1000u);
    EXPECT_EQ(out[1], 1001u);
    EXPECT_EQ(out[5], 0u);
    EXPECT_EQ(out[11], 1011u);
}

TEST(DiscardAvatarItem, WearedGumNotIncluded) {
    // Default-fill range is [12, 18) = [Weared_Hair, Weared_Gum).
    // Weared_Gwun (18), Weared_Do (19), etc. are NOT filled.
    AvatarEquipRow e = make_equip(/*pos=*/5, {});
    auto av = zero_avatar();
    av[5] = 100;
    av[18] = 0;  // would be filled if range was inclusive
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out[18], 0u);  // NOT filled (range is exclusive)
}

TEST(DiscardAvatarItem, PositionAtMaxIsOutOfRange) {
    // EAvatarCount = 24, which is the legacy eAvatar_Max sentinel.
    // pos = 24 is the sentinel, not a valid slot.
    AvatarEquipRow e = make_equip(/*pos=*/24, {});
    auto av = zero_avatar();
    av[5] = 100;
    auto out = discard_avatar_item(&e, 100, av);
    EXPECT_EQ(out, av);  // no-op
}
