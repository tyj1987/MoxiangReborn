#include "cfrienddialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(FriendDialog, AddsSelectsAndWhispers){cFriendDialog d;d.AddFriend({1,"Alice",FriendStatus::Online});FriendEntry got{};d.SetWhisperCallback([&](const auto&f){got=f;});ASSERT_TRUE(d.Select(0));EXPECT_TRUE(d.WhisperSelected());EXPECT_EQ(got.name,"Alice");}
TEST(FriendDialog, PreventsDuplicatesAndUpdatesStatus){cFriendDialog d;d.AddFriend({1,"Alice",{}});d.AddFriend({1,"Duplicate",FriendStatus::Busy});EXPECT_EQ(d.Friends().size(),1u);EXPECT_TRUE(d.UpdateStatus(1,FriendStatus::Busy));EXPECT_EQ(d.Friends()[0].status,FriendStatus::Busy);}
TEST(FriendDialog, RemovesAndRejectsUnknown){cFriendDialog d;d.AddFriend({1,"Alice",{}});EXPECT_TRUE(d.RemoveFriend(1));EXPECT_FALSE(d.RemoveFriend(1));EXPECT_FALSE(d.WhisperSelected());}

#include "mxh/services/IFriendService.hpp"
#include <string>
namespace {
// In-memory mock friend service for service-mode tests.
struct MockFriendService final : mxh::services::IFriendService {
 std::vector<mxh::services::FriendEntry> roster;
 std::size_t friendCount() const noexcept override { return roster.size(); }
 std::optional<mxh::services::FriendEntry> getFriend(std::size_t i) const noexcept override {
  if (i >= roster.size()) return std::nullopt;
  return roster[i];
 }
 bool isFriend(std::uint32_t id) const noexcept override {
  for (const auto& f : roster) if (f.id == id) return true;
  return false;
 }
 bool isFriendByName(const std::string& name) const noexcept override {
  for (const auto& f : roster) if (f.name == name) return true;
  return false;
 }
 std::optional<mxh::services::FriendStatus> getStatus(std::uint32_t id) const noexcept override {
  for (const auto& f : roster) if (f.id == id) return f.status;
  return std::nullopt;
 }
};
}

TEST(FriendDialog, ServiceIsFriendChecksRoster) {
  MockFriendService svc;
  svc.roster.push_back({1, "Alice", mxh::services::FriendStatus::Online});
  svc.roster.push_back({2, "Bob",   mxh::services::FriendStatus::Offline});
  cFriendDialog d;
  d.SetFriendService(&svc);
  EXPECT_EQ(d.GetFriendService(), &svc);
  EXPECT_TRUE(d.IsFriendOnline(1));
  EXPECT_FALSE(d.IsFriendOnline(2));  // offline
  EXPECT_FALSE(d.IsFriendOnline(99)); // not in roster
}

TEST(FriendDialog, ServiceGatesWhisperOnRoster) {
  MockFriendService svc;
  svc.roster.push_back({1, "Alice", mxh::services::FriendStatus::Online});
  cFriendDialog d;
  d.SetFriendService(&svc);
  d.AddFriend({1, "Alice", FriendStatus::Online}); // also populates local snapshot
  ASSERT_TRUE(d.Select(0));
  int calls = 0;
  FriendEntry got{};
  d.SetWhisperCallback([&](const FriendEntry& f) { ++calls; got = f; });
  EXPECT_TRUE(d.WhisperSelected());
  EXPECT_EQ(calls, 1);
  EXPECT_EQ(got.name, "Alice");
  // Service says Alice is not in roster (we simulate a removal). Whisper rejected.
  svc.roster.clear();
  EXPECT_FALSE(d.WhisperSelected());
  EXPECT_EQ(calls, 1);  // callback not invoked again
}

TEST(FriendDialog, ServiceClearFallsBackToLocalSnapshot) {
  MockFriendService svc;
  svc.roster.push_back({1, "Alice", mxh::services::FriendStatus::Online});
  cFriendDialog d;
  d.AddFriend({1, "Alice", FriendStatus::Offline}); // local-only (not Online)
  d.SetFriendService(&svc);
  // Service-bound: IsFriendOnline returns true (service has Alice Online)
  EXPECT_TRUE(d.IsFriendOnline(1));
  d.SetFriendService(nullptr);
  // Cleared: falls back to local snapshot (Alice is Offline locally)
  EXPECT_FALSE(d.IsFriendOnline(1));
}

TEST(FriendDialog, ServiceReflectsLivePresence) {
  MockFriendService svc;
  svc.roster.push_back({1, "Alice", mxh::services::FriendStatus::Offline});
  cFriendDialog d;
  d.SetFriendService(&svc);
  EXPECT_FALSE(d.IsFriendOnline(1));
  // Live presence change (e.g. friend logs in).
  svc.roster[0].status = mxh::services::FriendStatus::Online;
  EXPECT_TRUE(d.IsFriendOnline(1));
  // And back to Busy (still "online" for whisper purposes but we check Online specifically).
  svc.roster[0].status = mxh::services::FriendStatus::Busy;
  EXPECT_FALSE(d.IsFriendOnline(1));  // Busy != Online
}