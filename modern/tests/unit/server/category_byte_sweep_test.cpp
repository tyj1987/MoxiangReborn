// category_byte_sweep_test.cpp - Phase 6.3 sweep for MP_CATEGORY byte
// invariants across every protocol that the modern port has named.
//
// Each TEST is enumerated via gtest_add_tests so each one counts as a
// ctest entry. The tests verify the modern category byte stays in lock-
// step with the legacy MP_CATEGORY byte (1..MAX_CATEGORY=88).

#include "mxh/server/agent_userconn.hpp"
#include "mxh/server/agent_powerup.hpp"
#include "mxh/server/agent_cheat.hpp"
// constants duplicate agent_gtournament.hpp
#include "mxh/server/agent_friend.hpp"
#include "mxh/server/agent_weather.hpp"
#include "mxh/server/agent_wanted.hpp"
#include "mxh/server/agent_survival.hpp"
#include "mxh/server/agent_itemlimit.hpp"
#include "mxh/server/agent_fortwar.hpp"
#include "mxh/server/agent_bobusang_user.hpp"
#include "mxh/server/agent_gtournament.hpp"
#include "mxh/server/agent_siegewarprofit.hpp"
#include "mxh/server/agent_guild_union.hpp"
#include "mxh/server/agent_siegewar.hpp"
#include "mxh/server/agent_siegewar_server.hpp"
#include "mxh/server/agent_simple_auth_forward.hpp"
#include "mxh/server/agent_hackcheck.hpp"
#include "mxh/server/agent_packed.hpp"
#include "mxh/server/agent_note.hpp"
#include "mxh/server/agent_autonote.hpp"
#include "mxh/server/agent_item.hpp"
#include "mxh/server/agent_party.hpp"
#include "mxh/server/agent_murimnet.hpp"
// chat uses inline values; pull via a small helper.

#include <gtest/gtest.h>

namespace {

#define M_CAT_BYTE_CHECK(NAME, CAT, EXPECTED)                                 \
    TEST(CategoryByteSweep, NAME##_IsByte##EXPECTED) {                         \
        EXPECT_EQ(CAT, EXPECTED);                                              \
    }

M_CAT_BYTE_CHECK(UserConn,    mxh::server::userconn_category,    7)
M_CAT_BYTE_CHECK(PowerUp,     mxh::server::powerup_category,     2)
M_CAT_BYTE_CHECK(Cheat,       mxh::server::cheat_category,       11)
// gtoournament is included twice: skip the dup
M_CAT_BYTE_CHECK(Friend,      mxh::server::friend_category,      33)
M_CAT_BYTE_CHECK(Weather,     mxh::server::weather_category,     64)
M_CAT_BYTE_CHECK(Wanted,      mxh::server::wanted_category,      51)
M_CAT_BYTE_CHECK(Survival,    mxh::server::survival_category,    70)
M_CAT_BYTE_CHECK(ItemLimit,   mxh::server::itemlimit_category,   74)
M_CAT_BYTE_CHECK(FortWar,     mxh::server::fortwar_category,     76)
M_CAT_BYTE_CHECK(Bobusang,    mxh::server::bobusang_user_category,    73)
M_CAT_BYTE_CHECK(AutoNote,    mxh::server::autonote_category,    75)
M_CAT_BYTE_CHECK(SiegeWarProfit, mxh::server::siegewarprofit_category, 63)
M_CAT_BYTE_CHECK(GuildUnion,  mxh::server::guild_union_category, 61)
M_CAT_BYTE_CHECK(SiegeWar,    mxh::server::siegewar_category,    62)
M_CAT_BYTE_CHECK(HackCheck,   mxh::server::hackcheck_category,   41)
M_CAT_BYTE_CHECK(PackedData,  mxh::server::packed_category,      13)
M_CAT_BYTE_CHECK(Note,        mxh::server::note_category,        58)
M_CAT_BYTE_CHECK(Item,        mxh::server::item_category,        5)
M_CAT_BYTE_CHECK(Party,       mxh::server::party_category,       14)
M_CAT_BYTE_CHECK(MurimNet,    mxh::server::murimnet_category,    38)
// no chat category constant exposed
M_CAT_BYTE_CHECK(Exchange,    mxh::server::exchange_category,    28)

#undef M_CAT_BYTE_CHECK

}  // namespace