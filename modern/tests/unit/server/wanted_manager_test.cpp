// wanted_manager_test.cpp

#include "mxh/server/wanted_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::WantedManager;
using mxh::server::WantNpcManager;
using mxh::server::WantNpcState;
}

TEST(WantedManager, RegisterAndFind) {
    WantedManager wm;
    EXPECT_TRUE(wm.register_target(100, "BadGuy", 5000, 1000));
    EXPECT_TRUE(wm.register_target(200, "BadGirl", 3000, 1000));
    EXPECT_EQ(wm.size(), 2u);
    auto* w = wm.find(1);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->target_id, 100u);
    EXPECT_STREQ(w->target_name, "BadGuy");
}

TEST(WantedManager, ClaimSetsKiller) {
    WantedManager wm;
    wm.register_target(100, "x", 1000, 1000);
    EXPECT_TRUE(wm.claim(1, 999));
    auto* w = wm.find(1);
    EXPECT_EQ(w->killer_id, 999u);
    EXPECT_FALSE(wm.claim(1, 1000));  // already claimed
    EXPECT_FALSE(wm.claim(99, 7));    // not found
}

TEST(WantedManager, Remove) {
    WantedManager wm;
    wm.register_target(100, "x", 100, 0);
    EXPECT_TRUE(wm.remove(1));
    EXPECT_EQ(wm.size(), 0u);
    EXPECT_FALSE(wm.remove(99));
}

TEST(WantedManager, TargetNameTruncatesAt16) {
    WantedManager wm;
    std::string big(30, 'a');
    wm.register_target(42, big, 100, 0);
    auto* w = wm.find(1);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(std::strlen(w->target_name), 16u);
}

TEST(WantNpcManager, RegisterAndSetState) {
    WantNpcManager mgr;
    EXPECT_TRUE(mgr.register_bounty(100, 7, 42, 1000));
    EXPECT_EQ(mgr.size(), 1u);
    auto list = mgr.list_for_player(100);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].state, WantNpcState::Progress);
    EXPECT_TRUE(mgr.set_state(100, 7, WantNpcState::Complete, 5000));
    auto list2 = mgr.list_for_player(100);
    EXPECT_EQ(list2[0].state, WantNpcState::Complete);
    EXPECT_EQ(list2[0].complete_ms, 5000u);
}

TEST(WantNpcManager, RefuseBadInputs) {
    WantNpcManager mgr;
    EXPECT_FALSE(mgr.register_bounty(0, 7, 42, 1000));   // bad player
    EXPECT_FALSE(mgr.register_bounty(7, 0, 42, 1000));   // bad npc
    EXPECT_FALSE(mgr.set_state(99, 7, WantNpcState::Complete, 1));  // not found
}

TEST(WantNpcManager, PerPlayerListIsolated) {
    WantNpcManager mgr;
    mgr.register_bounty(100, 1, 10, 0);
    mgr.register_bounty(100, 2, 10, 0);
    mgr.register_bounty(200, 3, 10, 0);
    EXPECT_EQ(mgr.list_for_player(100).size(), 2u);
    EXPECT_EQ(mgr.list_for_player(200).size(), 1u);
    EXPECT_EQ(mgr.list_for_player(999).size(), 0u);
}
