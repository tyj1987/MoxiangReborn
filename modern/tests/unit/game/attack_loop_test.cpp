#include "mxh/game/battle.hpp"
#include <gtest/gtest.h>
#include <array>
#include <cstdint>
namespace {
struct AttackRequest { std::uint32_t min_damage, max_damage, mana, cost; std::int32_t random_gap; };
std::array<std::uint8_t, 12> encode_result(const mxh::game::DamageResult& r) {
 std::array<std::uint8_t,12> b{}; auto put=[&](int o,std::uint32_t v){for(int i=0;i<4;++i)b[o+i]=static_cast<std::uint8_t>(v>>(8*i));};
 put(0,r.damage); put(4,r.mana_after); put(8,r.executed?1u:0u); return b;
}
mxh::game::DamageResult decode_result(const std::array<std::uint8_t,12>& b) {
 auto get=[&](int o){std::uint32_t v=0;for(int i=0;i<4;++i)v|=static_cast<std::uint32_t>(b[o+i])<<(8*i);return v;};
 return {get(0),0,get(4),get(8)!=0};
}
}
TEST(AttackLoopE2E, ClientServerClientPreservesDamageAndMana) {
 AttackRequest request{10,20,50,7,3};
 mxh::game::BattleContext server{}; server.attacker_min_damage=request.min_damage; server.attacker_max_damage=request.max_damage; server.attacker_mana=request.mana; server.mana_cost=request.cost; server.random_gap=request.random_gap;
 auto server_result=mxh::game::resolve_physical_attack(server);
 auto client_result=decode_result(encode_result(server_result));
 EXPECT_TRUE(client_result.executed); EXPECT_EQ(client_result.damage,13u); EXPECT_EQ(client_result.mana_after,43u);
}
TEST(AttackLoopE2E, ClientSeesRejectedAttackWhenManaIsInsufficient) {
 mxh::game::BattleContext server{}; server.attacker_min_damage=10; server.attacker_max_damage=20; server.attacker_mana=6; server.mana_cost=7;
 auto result=decode_result(encode_result(mxh::game::resolve_physical_attack(server)));
 EXPECT_FALSE(result.executed); EXPECT_EQ(result.damage,0u); EXPECT_EQ(result.mana_after,6u);
}
