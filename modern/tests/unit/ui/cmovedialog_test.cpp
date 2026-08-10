#include "cmovedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MoveDialog, SelectsAndConfirmsDestination){cMoveDialog d;d.AddMoveInfo({1,"Town",true});d.AddMoveInfo({2,"Saved",false});d.SetTownMoveView(true);EXPECT_TRUE(d.SelectMoveIdx(0));MovePoint selected{};d.SetMoveCallback([&](const auto&p){selected=p;return true;});EXPECT_TRUE(d.MapMoveOK());EXPECT_EQ(selected.db_id,1u);}
TEST(MoveDialog, FiltersTownAndSavedViews){cMoveDialog d;d.AddMoveInfo({1,"Town",true});d.AddMoveInfo({2,"Saved",false});d.SetTownMoveView(true);EXPECT_FALSE(d.SelectMoveIdx(1));d.SetTownMoveView(false);EXPECT_TRUE(d.SelectMoveIdx(1));}
TEST(MoveDialog, UpdatesDeletesAndRejectsMissingSelection){cMoveDialog d;d.AddMoveInfo({1,"Old",false});EXPECT_TRUE(d.UpdateMoveInfo({1,"New",false}));EXPECT_EQ(d.Points()[0].name,"New");EXPECT_TRUE(d.DeleteMoveInfo(1));EXPECT_FALSE(d.MapMoveOK());}

#include "mxh/services/IMoveService.hpp"
namespace {
// In-memory mock teleport service for service-mode tests.
struct MockMoveService final : mxh::services::IMoveService {
 std::vector<mxh::services::MovePoint> points;
 std::size_t pointCount() const noexcept override { return points.size(); }
 std::optional<mxh::services::MovePoint> getPoint(std::size_t i) const noexcept override {
  if (i >= points.size()) return std::nullopt;
  return points[i];
 }
 bool isKnownPoint(std::uint32_t db_id) const noexcept override {
  for (const auto& p : points) if (p.db_id == db_id) return true;
  return false;
 }
 bool hasTownPoint() const noexcept override {
  for (const auto& p : points) if (p.town) return true;
  return false;
 }
 bool hasSavedPoint() const noexcept override {
  for (const auto& p : points) if (!p.town) return true;
  return false;
 }
};
}

TEST(MoveDialog, ServicePointCountReflectsCatalog) {
  MockMoveService svc;
  svc.points.push_back({1, "Town A", true});
  svc.points.push_back({2, "Saved B", false});
  cMoveDialog d;
  EXPECT_EQ(d.PointCount(), 0u);  // no service bound, no local points
  d.SetMoveService(&svc);
  EXPECT_EQ(d.PointCount(), 2u);  // service count is live
  svc.points.push_back({3, "Town C", true});
  EXPECT_EQ(d.PointCount(), 3u);  // catalog mutation reflects immediately
}

TEST(MoveDialog, ServiceGatesMapMoveOnKnownPoint) {
  MockMoveService svc;
  svc.points.push_back({1, "Town", true});
  cMoveDialog d;
  d.AddMoveInfo({1, "Town", true});
  d.SetMoveService(&svc);
  d.SetTownMoveView(true);
  ASSERT_TRUE(d.SelectMoveIdx(0));
  int calls = 0;
  MovePoint got{};
  d.SetMoveCallback([&](const MovePoint& p) { ++calls; got = p; return true; });
  EXPECT_TRUE(d.MapMoveOK());
  EXPECT_EQ(calls, 1);
  EXPECT_EQ(got.db_id, 1u);
  // Remove the point from the service. MapMoveOK rejects (no callback fired).
  svc.points.clear();
  EXPECT_FALSE(d.MapMoveOK());
  EXPECT_EQ(calls, 1);
}

TEST(MoveDialog, ServiceHidesEmptyTabsViaHasTownHasSaved) {
  MockMoveService svc;
  svc.points.push_back({1, "Town", true});  // only town; no saved
  cMoveDialog d;
  d.AddMoveInfo({1, "Town", true});
  d.SetMoveService(&svc);
  d.SetTownMoveView(true);
  EXPECT_TRUE(d.SelectMoveIdx(0));  // town visible (service has town)
  d.SetTownMoveView(false);
  EXPECT_FALSE(d.SelectMoveIdx(0)); // saved view: service has no saved points
  // Add a saved point to the service and the saved view opens.
  d.AddMoveInfo({2, "Saved", false});
  svc.points.push_back({2, "Saved", false});
  d.SetTownMoveView(false);
  EXPECT_TRUE(d.SelectMoveIdx(1));
}

TEST(MoveDialog, ServiceClearFallsBackToLocalSnapshot) {
  MockMoveService svc;
  svc.points.push_back({1, "Town", true});
  cMoveDialog d;
  d.AddMoveInfo({1, "Town", true});
  d.SetMoveService(&svc);
  EXPECT_EQ(d.PointCount(), 1u); // service has 1 point
  d.SetMoveService(nullptr);
  EXPECT_EQ(d.PointCount(), 1u); // falls back to local m_points
  d.SetTownMoveView(true);
  EXPECT_TRUE(d.SelectMoveIdx(0)); // legacy path still works
}