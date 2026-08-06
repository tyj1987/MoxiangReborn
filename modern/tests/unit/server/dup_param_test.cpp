// dup_param_test.cpp - 1:1 data-plane tests for the legacy
// CShopItemManager::AddDupParam / DeleteDupParam / IsDupAble from
// [Server]Map/ShopItemManager.cpp:2286-2620. Locks the per-bit OR /
// XOR-when-both-set / block semantics across the 5 dup categories
// (charm / herb / incantation / sundries / pet_equip).

#include <mxh/server/dup_param.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
using namespace mxh::server;















namespace {

// Test lookup that maps index -> Param bitset. Index 0 / unmapped
// returns 0 (matches the legacy "no SHOPITEMDUP entry" path).
class FixedLookup final : public DupParamLookup {
public:
    std::map<std::uint32_t, std::uint32_t> table;
    std::uint32_t dup_param_for(std::uint32_t index) const noexcept override {
        auto it = table.find(index);
        if (it == table.end()) return 0;
        return it->second;
    }
};

}  // namespace

// ---------------------------------------------------------------------
// AddDupParam
// ---------------------------------------------------------------------

TEST(AddDupParam, NoIndicesIsNoOp) {
    DupCounters c;
    DupParamIndices idx;
    FixedLookup lookup;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, 0u);
    EXPECT_EQ(c.herb, 0u);
    EXPECT_EQ(c.incantation, 0u);
    EXPECT_EQ(c.sundries, 0u);
    EXPECT_EQ(c.pet_equip, 0u);
    EXPECT_FALSE(fx.set_b_street_stall);
    EXPECT_FALSE(fx.clear_b_street_stall);
}

TEST(AddDupParam, CharmBitsOrIntoCounter) {
    DupCounters c;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage | charm_dup::NaegongDamage |
                        charm_dup::Reinforce;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, charm_dup::WoigongDamage | charm_dup::NaegongDamage |
                          charm_dup::Reinforce);
}

TEST(AddDupParam, CharmOrIsIdempotent) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;  // bit already set
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, charm_dup::WoigongDamage);
}

TEST(AddDupParam, HerbBitsOrIntoCounter) {
    DupCounters c;
    DupParamIndices idx;
    idx.mugong_num = 200;
    FixedLookup lookup;
    lookup.table[200] = herb_dup::Life | herb_dup::Shield;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.herb, herb_dup::Life | herb_dup::Shield);
}

TEST(AddDupParam, IncantationBitsOrIntoCounter) {
    DupCounters c;
    DupParamIndices idx;
    idx.mugong_type = 300;
    FixedLookup lookup;
    lookup.table[300] = incantation_dup::MemoryMove | incantation_dup::ProtectAll;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.incantation, incantation_dup::MemoryMove | incantation_dup::ProtectAll);
}

TEST(AddDupParam, SundriesBitsOrAndTriggersStreetStall) {
    DupCounters c;
    DupParamIndices idx;
    idx.life_recover = 400;
    FixedLookup lookup;
    lookup.table[400] = sundries_dup::StreetStall;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.sundries, sundries_dup::StreetStall);
    EXPECT_TRUE(fx.set_b_street_stall);
    EXPECT_FALSE(fx.clear_b_street_stall);
}

TEST(AddDupParam, SundriesBitsWithoutStreetStallHasNoSideEffect) {
    DupCounters c;
    DupParamIndices idx;
    idx.life_recover = 400;
    FixedLookup lookup;
    lookup.table[400] = 0;  // no bits
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.sundries, 0u);
    EXPECT_FALSE(fx.set_b_street_stall);
}

TEST(AddDupParam, PetEquipBitsOrIntoCounter) {
    DupCounters c;
    DupParamIndices idx;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[500] = pet_equip_dup::PomanRing;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.pet_equip, pet_equip_dup::PomanRing);
}

TEST(AddDupParam, AllFiveCategoriesTogether) {
    DupCounters c;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;
    lookup.table[200] = herb_dup::Life;
    lookup.table[300] = incantation_dup::MemoryMove;
    lookup.table[400] = sundries_dup::StreetStall;
    lookup.table[500] = pet_equip_dup::PomanRing;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, charm_dup::WoigongDamage);
    EXPECT_EQ(c.herb, herb_dup::Life);
    EXPECT_EQ(c.incantation, incantation_dup::MemoryMove);
    EXPECT_EQ(c.sundries, sundries_dup::StreetStall);
    EXPECT_EQ(c.pet_equip, pet_equip_dup::PomanRing);
    EXPECT_TRUE(fx.set_b_street_stall);
}

TEST(AddDupParam, LookupReturnsZeroIsNoOp) {
    DupCounters c;
    DupParamIndices idx;
    idx.all_plus_value = 999;  // not in lookup table
    FixedLookup lookup;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, 0u);
}

// ---------------------------------------------------------------------
// DeleteDupParam
// ---------------------------------------------------------------------

TEST(DeleteDupParam, NoIndicesIsNoOp) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    FixedLookup lookup;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, charm_dup::WoigongDamage);
    EXPECT_FALSE(fx.set_b_street_stall);
    EXPECT_FALSE(fx.clear_b_street_stall);
}

TEST(DeleteDupParam, CharmBitsClearedWhenBothSet) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, 0u);
}

TEST(DeleteDupParam, CharmBitsUnchangedWhenCounterBitAbsent) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::NaegongDamage;  // different bit
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, charm_dup::WoigongDamage);  // unchanged
}

TEST(DeleteDupParam, CharmBitsSubsetCleared) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage | charm_dup::NaegongDamage;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;  // only clear one bit
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, charm_dup::NaegongDamage);
}

TEST(DeleteDupParam, HerbBitsCleared) {
    DupCounters c;
    c.herb = herb_dup::Life | herb_dup::Shield;
    DupParamIndices idx;
    idx.mugong_num = 200;
    FixedLookup lookup;
    lookup.table[200] = herb_dup::Life;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.herb, herb_dup::Shield);
}

TEST(DeleteDupParam, IncantationBitsCleared) {
    DupCounters c;
    c.incantation = incantation_dup::MemoryMove | incantation_dup::ProtectAll;
    DupParamIndices idx;
    idx.mugong_type = 300;
    FixedLookup lookup;
    lookup.table[300] = incantation_dup::MemoryMove;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.incantation, incantation_dup::ProtectAll);
}

TEST(DeleteDupParam, SundriesStreetStallClearedAndTriggersSideEffect) {
    DupCounters c;
    c.sundries = sundries_dup::StreetStall;
    DupParamIndices idx;
    idx.life_recover = 400;
    FixedLookup lookup;
    lookup.table[400] = sundries_dup::StreetStall;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.sundries, 0u);
    EXPECT_TRUE(fx.clear_b_street_stall);
    EXPECT_FALSE(fx.set_b_street_stall);
}

TEST(DeleteDupParam, SundriesStreetStallAbsentNoSideEffect) {
    DupCounters c;
    DupParamIndices idx;
    idx.life_recover = 400;
    FixedLookup lookup;
    lookup.table[400] = sundries_dup::StreetStall;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.sundries, 0u);
    EXPECT_FALSE(fx.clear_b_street_stall);
    EXPECT_FALSE(fx.set_b_street_stall);
}

TEST(DeleteDupParam, PetEquipBitsCleared) {
    DupCounters c;
    c.pet_equip = pet_equip_dup::PomanRing;
    DupParamIndices idx;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[500] = pet_equip_dup::PomanRing;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.pet_equip, 0u);
}

TEST(DeleteDupParam, AllFiveCategoriesTogether) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    c.herb = herb_dup::Life;
    c.incantation = incantation_dup::MemoryMove;
    c.sundries = sundries_dup::StreetStall;
    c.pet_equip = pet_equip_dup::PomanRing;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;
    lookup.table[200] = herb_dup::Life;
    lookup.table[300] = incantation_dup::MemoryMove;
    lookup.table[400] = sundries_dup::StreetStall;
    lookup.table[500] = pet_equip_dup::PomanRing;
    SundrySideEffects fx;
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, 0u);
    EXPECT_EQ(c.herb, 0u);
    EXPECT_EQ(c.incantation, 0u);
    EXPECT_EQ(c.sundries, 0u);
    EXPECT_EQ(c.pet_equip, 0u);
    EXPECT_TRUE(fx.clear_b_street_stall);
}

// ---------------------------------------------------------------------
// IsDupAble
// ---------------------------------------------------------------------

TEST(IsDupAble, EmptyCountersAllDupEnabled) {
    DupCounters c;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;
    lookup.table[200] = herb_dup::Life;
    lookup.table[300] = incantation_dup::MemoryMove;
    lookup.table[400] = sundries_dup::StreetStall;
    lookup.table[500] = pet_equip_dup::PomanRing;
    EXPECT_TRUE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, CharmOverlapBlocks) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;
    EXPECT_FALSE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, CharmNoOverlapAllows) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::NaegongDamage;  // different bit
    EXPECT_TRUE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, HerbOverlapBlocks) {
    DupCounters c;
    c.herb = herb_dup::Life | herb_dup::Shield;
    DupParamIndices idx;
    idx.mugong_num = 200;
    FixedLookup lookup;
    lookup.table[200] = herb_dup::Shield;
    EXPECT_FALSE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, IncantationOverlapBlocks) {
    DupCounters c;
    c.incantation = incantation_dup::MemoryMove;
    DupParamIndices idx;
    idx.mugong_type = 300;
    FixedLookup lookup;
    lookup.table[300] = incantation_dup::MemoryMove;
    EXPECT_FALSE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, SundriesOverlapBlocks) {
    DupCounters c;
    c.sundries = sundries_dup::StreetStall;
    DupParamIndices idx;
    idx.life_recover = 400;
    FixedLookup lookup;
    lookup.table[400] = sundries_dup::StreetStall;
    EXPECT_FALSE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, PetEquipOverlapBlocks) {
    DupCounters c;
    c.pet_equip = pet_equip_dup::PomanRing;
    DupParamIndices idx;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[500] = pet_equip_dup::PomanRing;
    EXPECT_FALSE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, NoIndicesReturnsTrue) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    DupParamIndices idx;
    FixedLookup lookup;
    EXPECT_TRUE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, LookupReturnsZeroIsTrue) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;  // counter has bits
    DupParamIndices idx;
    idx.all_plus_value = 999;  // not in lookup
    FixedLookup lookup;
    EXPECT_TRUE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, AnyCategoryOverlapBlocks) {
    // Counter has WoigongDamage charm bit; charm dup_param has the same
    // bit -> overlap detected by the charm block -> is_dup_able false.
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    c.herb = 0;
    c.incantation = 0;
    c.sundries = 0;
    c.pet_equip = 0;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;  // overlap with charm counter
    lookup.table[200] = 0;
    lookup.table[300] = incantation_dup::MemoryMove;
    lookup.table[400] = 0;
    lookup.table[500] = 0;
    EXPECT_FALSE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, NoOverlapReturnsTrueWithAllIndicesSet) {
    // All 5 indices set, but the counter has no bits anywhere, so the
    // data plane should find no overlap and return true.
    DupCounters c;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage;
    lookup.table[200] = herb_dup::Life;
    lookup.table[300] = incantation_dup::MemoryMove;
    lookup.table[400] = sundries_dup::StreetStall;
    lookup.table[500] = pet_equip_dup::PomanRing;
    EXPECT_TRUE(is_dup_able(c, idx, lookup));
}

TEST(IsDupAble, DisjointBitsAllAcrossFiveCategoriesReturnsTrue) {
    DupCounters c;
    c.charm = charm_dup::WoigongDamage;
    c.herb = herb_dup::Life;
    c.incantation = incantation_dup::MemoryMove;
    c.sundries = sundries_dup::StreetStall;
    c.pet_equip = pet_equip_dup::PomanRing;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    // Different bits than what's already in the counter.
    lookup.table[100] = charm_dup::NaegongDamage;
    lookup.table[200] = herb_dup::Shield;
    lookup.table[300] = incantation_dup::ProtectAll;
    lookup.table[400] = 0;
    lookup.table[500] = 0;
    EXPECT_TRUE(is_dup_able(c, idx, lookup));
}

// ---------------------------------------------------------------------
// Round-trip: AddDupParam then DeleteDupParam returns counters to zero.
// ---------------------------------------------------------------------

TEST(DupParamRoundTrip, AddThenDeleteReturnsToZero) {
    DupCounters c;
    DupParamIndices idx;
    idx.all_plus_value = 100;
    idx.mugong_num = 200;
    idx.mugong_type = 300;
    idx.life_recover = 400;
    idx.life_recover_rate = 500;
    FixedLookup lookup;
    lookup.table[100] = charm_dup::WoigongDamage | charm_dup::NaegongDamage;
    lookup.table[200] = herb_dup::Life | herb_dup::Shield;
    lookup.table[300] = incantation_dup::MemoryMove;
    lookup.table[400] = sundries_dup::StreetStall;
    lookup.table[500] = pet_equip_dup::PomanRing;
    SundrySideEffects fx;
    add_dup_param(c, idx, lookup, fx);
    EXPECT_NE(c.charm, 0u);
    EXPECT_NE(c.herb, 0u);
    EXPECT_NE(c.incantation, 0u);
    EXPECT_NE(c.sundries, 0u);
    EXPECT_NE(c.pet_equip, 0u);
    delete_dup_param(c, idx, lookup, fx);
    EXPECT_EQ(c.charm, 0u);
    EXPECT_EQ(c.herb, 0u);
    EXPECT_EQ(c.incantation, 0u);
    EXPECT_EQ(c.sundries, 0u);
    EXPECT_EQ(c.pet_equip, 0u);
}
