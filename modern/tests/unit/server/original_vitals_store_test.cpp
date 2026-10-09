#include "mxh/server/original_vitals_store.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

namespace {

using mxh::db::DbError;
using mxh::db::DbResult;
using mxh::db::ResultSet;
using mxh::server::OriginalVitalsCommitState;
using mxh::server::OriginalVitalsLoadState;
using mxh::server::OriginalVitalsRecord;

class MockVitalsDb final : public mxh::db::IDbAdapter {
public:
    struct Row {
        std::optional<std::int64_t> hp, shield, mp;
        std::int64_t dead = 0, initialized = 0, revision = 0;
    };

    std::optional<Row> stored;
    int companion_value = 4;
    bool fail_begin = false, fail_cas = false, fail_companion = false;
    bool fail_commit = false, fail_rollback = false;
    bool commit_despite_error = false;
    int begin_calls = 0, commit_calls = 0, rollback_calls = 0, execute_calls = 0;

    DbResult connect(const mxh::db::ConnectionConfig&) override { return {}; }
    void disconnect() override {}
    bool is_connected() const noexcept override { return true; }
    std::string backend_name() const noexcept override { return "mock-no-database"; }

    DbResult query(std::string_view, std::span<const mxh::db::Bind>, ResultSet& out) override {
        out = {};
        if (!stored) return {};
        auto field = [](std::optional<std::int64_t> v) -> mxh::db::Value {
            return v ? mxh::db::Value{*v} : mxh::db::Value{std::monostate{}};
        };
        out.rows.push_back({field(stored->hp), field(stored->shield), field(stored->mp),
                            stored->dead, stored->initialized, stored->revision});
        return {};
    }

    DbResult execute(std::string_view sql, std::span<const mxh::db::Bind> args) override {
        ++execute_calls;
        if (sql.find("INSERT INTO modern_player_vitals") != std::string_view::npos) {
            if (stored || !in_transaction) return {DbError::ConstraintViolation, "duplicate or no tx"};
            staged = Row{};
            return {DbError::Ok, {}, 1};
        }
        if (sql.find("UPDATE modern_player_vitals") != std::string_view::npos) {
            if (fail_cas) return {DbError::IoError, "write failed"};
            if (!in_transaction || !staged) return {DbError::Unknown, "no row or tx"};
            const auto value = [&](std::size_t i) { return std::get<std::int64_t>(args[i].value); };
            if (sql.find("initialized=0") != std::string_view::npos) {
                if (staged->initialized != 0 || staged->revision != 0) return {DbError::Ok, {}, 0};
                staged->hp = value(0); staged->shield = value(1); staged->mp = value(2);
                staged->dead = 0; staged->initialized = 1; staged->revision = 1;
                return {DbError::Ok, {}, 1};
            }
            if (staged->initialized != 1 || staged->revision != value(5) ||
                staged->hp != value(6) || staged->shield != value(7) ||
                staged->mp != value(8) || staged->dead != value(9)) {
                return {DbError::Ok, {}, 0};
            }
            staged->hp = value(0); staged->shield = value(1); staged->mp = value(2);
            staged->dead = value(3); ++staged->revision;
            return {DbError::Ok, {}, 1};
        }
        if (sql == "COMPANION_ITEM_WRITE") {
            if (fail_companion) return {DbError::IoError, "item write failed"};
            staged_companion = 3;
            return {DbError::Ok, {}, 1};
        }
        return {DbError::QuerySyntaxError, "unexpected mock statement"};
    }

    DbResult begin_transaction() override {
        ++begin_calls;
        if (fail_begin) return {DbError::IoError, "begin failed"};
        in_transaction = true;
        staged = stored;
        staged_companion = companion_value;
        return {};
    }
    DbResult commit() override {
        ++commit_calls;
        if (fail_commit && !commit_despite_error) return {DbError::IoError, "unknown commit"};
        stored = staged;
        companion_value = staged_companion;
        in_transaction = false;
        return fail_commit ? DbResult{DbError::IoError, "lost commit acknowledgement"} : DbResult{};
    }
    DbResult rollback() override {
        ++rollback_calls;
        if (fail_rollback) return {DbError::IoError, "rollback unknown"};
        staged.reset();
        in_transaction = false;
        return {};
    }

private:
    bool in_transaction = false;
    std::optional<Row> staged;
    int staged_companion = 4;
};

OriginalVitalsRecord ready(std::int64_t revision = 1) {
    return {100004, 125, 245, 125, false, revision};
}

TEST(OriginalVitalsStore, MissingPendingAndMalformedRowsFailClosed) {
    MockVitalsDb db;
    EXPECT_EQ(mxh::server::load_original_vitals(db, 100004).state,
              OriginalVitalsLoadState::Missing);
    db.stored = MockVitalsDb::Row{};
    EXPECT_EQ(mxh::server::load_original_vitals(db, 100004).state,
              OriginalVitalsLoadState::Pending);
    db.stored->hp = 125;
    EXPECT_EQ(mxh::server::load_original_vitals(db, 100004).state,
              OriginalVitalsLoadState::Malformed);
    db.stored = MockVitalsDb::Row{125, 245, 125, 1, 1, 1};
    EXPECT_EQ(mxh::server::load_original_vitals(db, 100004).state,
              OriginalVitalsLoadState::Malformed);  // dead cannot have positive HP
    db.stored = MockVitalsDb::Row{0, 0, 0, 1, 1, 4};
    auto loaded = mxh::server::load_original_vitals(db, 100004);
    ASSERT_EQ(loaded.state, OriginalVitalsLoadState::Ready);
    ASSERT_TRUE(loaded.record);
    EXPECT_TRUE(loaded.record->dead);
    EXPECT_EQ(loaded.record->revision, 4);
}

TEST(OriginalVitalsStore, PendingInitializationHappensExactlyOnce) {
    MockVitalsDb db;
    ASSERT_TRUE(db.begin_transaction().ok());
    EXPECT_EQ(mxh::server::insert_pending_original_vitals(db, 100004).rows_affected, 1);
    ASSERT_TRUE(db.commit().ok());
    EXPECT_EQ(mxh::server::initialize_original_vitals(db, ready()),
              OriginalVitalsCommitState::Committed);
    ASSERT_TRUE(db.stored);
    EXPECT_EQ(db.stored->hp, 125);
    EXPECT_EQ(mxh::server::initialize_original_vitals(db, ready()),
              OriginalVitalsCommitState::KnownNotCommitted);
    EXPECT_EQ(db.stored->revision, 1);
}

TEST(OriginalVitalsStore, VitalsAndItemWriteCommitOrRollbackTogether) {
    MockVitalsDb db;
    db.stored = MockVitalsDb::Row{125, 245, 125, 0, 1, 1};
    auto after = ready(2);
    after.hp = 80;
    const auto item_write = [](mxh::db::IDbAdapter& adapter) {
        return adapter.execute("COMPANION_ITEM_WRITE");
    };
    db.fail_companion = true;
    EXPECT_EQ(mxh::server::commit_original_vitals(db, ready(), after, item_write),
              OriginalVitalsCommitState::KnownNotCommitted);
    ASSERT_TRUE(db.stored);
    EXPECT_EQ(db.stored->hp, 125);
    EXPECT_EQ(db.companion_value, 4);
    db.fail_companion = false;
    EXPECT_EQ(mxh::server::commit_original_vitals(db, ready(), after, item_write),
              OriginalVitalsCommitState::Committed);
    EXPECT_EQ(db.stored->hp, 80);
    EXPECT_EQ(db.stored->revision, 2);
    EXPECT_EQ(db.companion_value, 3);
}

TEST(OriginalVitalsStore, StaleRevisionAndFailedRollbackDoNotClaimSuccess) {
    MockVitalsDb db;
    db.stored = MockVitalsDb::Row{125, 245, 125, 0, 1, 2};
    auto after = ready(2);
    after.hp = 80;
    EXPECT_EQ(mxh::server::commit_original_vitals(db, ready(), after),
              OriginalVitalsCommitState::KnownNotCommitted);
    EXPECT_EQ(db.stored->hp, 125);
    db.fail_rollback = true;
    EXPECT_EQ(mxh::server::commit_original_vitals(db, ready(), after),
              OriginalVitalsCommitState::Unknown);
}

TEST(OriginalVitalsStore, LostCommitReplyRemainsUnknownEvenIfReadbackSeesIt) {
    MockVitalsDb db;
    db.stored = MockVitalsDb::Row{125, 245, 125, 0, 1, 1};
    db.fail_commit = true;
    db.commit_despite_error = true;
    auto after = ready(2);
    after.hp = 0;
    after.dead = true;
    mxh::server::OriginalVitalsSession session{ready()};
    EXPECT_EQ(mxh::server::commit_original_vitals_for_session(db, session, after),
              OriginalVitalsCommitState::Unknown);
    EXPECT_TRUE(session.quarantined);
    EXPECT_FALSE(session.can_publish());
    const auto observed = mxh::server::load_original_vitals(db, 100004);
    ASSERT_EQ(observed.state, OriginalVitalsLoadState::Ready);
    EXPECT_EQ(observed.record->revision, 2);
    EXPECT_TRUE(observed.record->dead);
    const auto calls_before = db.begin_calls;
    EXPECT_EQ(mxh::server::commit_original_vitals_for_session(db, session, after),
              OriginalVitalsCommitState::Unknown);
    EXPECT_EQ(db.begin_calls, calls_before);
    // Readback does not unlock the old session; a fresh GameIn must construct
    // a new one after checking all persisted fields.
}

TEST(OriginalVitalsStore, RevisionBoundAndInconsistentDeathNeverOpenTransaction) {
    MockVitalsDb db;
    auto old = ready(std::numeric_limits<std::int64_t>::max() - 1);
    auto next = old;
    next.revision += 1;
    EXPECT_EQ(mxh::server::commit_original_vitals(db, old, next),
              OriginalVitalsCommitState::KnownNotCommitted);
    EXPECT_EQ(db.begin_calls, 0);
    auto inconsistent = ready(2);
    inconsistent.hp = 0;
    EXPECT_EQ(mxh::server::commit_original_vitals(db, ready(), inconsistent),
              OriginalVitalsCommitState::KnownNotCommitted);
    EXPECT_EQ(db.begin_calls, 0);
}

}  // namespace
