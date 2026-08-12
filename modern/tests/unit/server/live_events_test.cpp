#include "mxh/server/live_events.hpp"
#include "mxh/db/db_adapter.hpp"
#include <gtest/gtest.h>
TEST(LiveEvents, AppliesOnlyCurrentRowsAndClampsCombinedMultipliers) {
    auto db = mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path = ":memory:"; ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_live_event(event_type TEXT,config_json TEXT,starts_at TEXT,ends_at TEXT,enabled INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_live_event VALUES('experience_multiplier','{\"multiplier\":2}','2026-01-01T00:00:00Z','2027-01-01T00:00:00Z',1)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_live_event VALUES('experience_multiplier','{\"multiplier\":8}','2026-01-01T00:00:00Z','2027-01-01T00:00:00Z',1)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_live_event VALUES('drop_multiplier','{\"multiplier\":2.5}','2026-01-01T00:00:00Z','2027-01-01T00:00:00Z',1)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_live_event VALUES('announcement','{\"message\":\"Double XP\"}','2026-01-01T00:00:00Z','2027-01-01T00:00:00Z',1)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_live_event VALUES('drop_multiplier','{\"multiplier\":9}','2025-01-01T00:00:00Z','2025-02-01T00:00:00Z',1)").ok());
    const auto snapshot = mxh::server::load_active_live_events(*db, "2026-08-12T00:00:00Z");
    EXPECT_DOUBLE_EQ(snapshot.experience_multiplier, 10.0); EXPECT_DOUBLE_EQ(snapshot.drop_multiplier, 2.5);
    ASSERT_EQ(snapshot.announcements.size(), 1u); EXPECT_EQ(snapshot.announcements[0], "Double XP");
}
TEST(LiveEvents, MissingTablePreservesOneToOneDefaults) {
    auto db = mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path = ":memory:"; ASSERT_TRUE(db->connect(cfg).ok());
    const auto snapshot = mxh::server::load_active_live_events(*db, "2026-08-12T00:00:00Z");
    EXPECT_DOUBLE_EQ(snapshot.experience_multiplier, 1.0); EXPECT_DOUBLE_EQ(snapshot.drop_multiplier, 1.0); EXPECT_TRUE(snapshot.announcements.empty());
}
