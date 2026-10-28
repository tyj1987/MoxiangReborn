#include "mxh/server/original_vitals_store.hpp"

#include <limits>
#include <variant>

namespace mxh::server {
namespace {

constexpr auto kMaxRevision = std::numeric_limits<std::int64_t>::max() - 1;

bool read_i64(const mxh::db::Value& value, std::int64_t& out) noexcept {
    const auto* number = std::get_if<std::int64_t>(&value);
    if (!number) return false;
    out = *number;
    return true;
}

bool read_u32(const mxh::db::Value& value, std::uint32_t& out) noexcept {
    std::int64_t number = 0;
    if (!read_i64(value, number) || number < 0 ||
        number > std::numeric_limits<std::uint32_t>::max()) return false;
    out = static_cast<std::uint32_t>(number);
    return true;
}

void retire_connection(mxh::db::IDbAdapter& db) noexcept {
    try { db.disconnect(); } catch (...) {}
}

OriginalVitalsCommitState confirmed_rollback_or_unknown(
    mxh::db::IDbAdapter& db,
    OriginalVitalsCommitState confirmed = OriginalVitalsCommitState::KnownNotCommitted) {
    try {
        if (db.rollback().ok()) return confirmed;
    } catch (...) {
    }
    retire_connection(db);
    return OriginalVitalsCommitState::Unknown;
}

OriginalVitalsCommitState uncertain_commit(mxh::db::IDbAdapter& db) noexcept {
    // The server may have committed despite a lost reply. Rollback is cleanup,
    // never proof that the write was not committed. Retire the shared adapter
    // before another player can inherit a possibly open transaction.
    try { (void)db.rollback(); } catch (...) {}
    retire_connection(db);
    return OriginalVitalsCommitState::Unknown;
}

}  // namespace

bool valid_original_vitals_record(const OriginalVitalsRecord& record) noexcept {
    return record.player_id != 0 && record.revision >= 1 &&
           record.revision <= kMaxRevision && record.dead == (record.hp == 0);
}

OriginalVitalsLoadResult load_original_vitals(
    mxh::db::IDbAdapter& db, std::uint32_t player_id) {
    if (player_id == 0) return {OriginalVitalsLoadState::Malformed, std::nullopt};
    mxh::db::ResultSet rows;
    mxh::db::DbResult query;
    try {
        query = db.query(
            "SELECT hp,shield,mp,dead,initialized,revision "
            "FROM modern_player_vitals WHERE player_id=?",
            {mxh::db::bind(static_cast<std::int64_t>(player_id))}, rows);
    } catch (...) {
        return {OriginalVitalsLoadState::DbError, std::nullopt};
    }
    if (!query.ok()) return {OriginalVitalsLoadState::DbError, std::nullopt};
    if (rows.rows.empty()) return {OriginalVitalsLoadState::Missing, std::nullopt};
    if (rows.rows.size() != 1 || rows.rows.front().size() != 6) {
        return {OriginalVitalsLoadState::Malformed, std::nullopt};
    }
    const auto& row = rows.rows.front();
    std::int64_t dead = 0, initialized = 0, revision = 0;
    if (!read_i64(row[3], dead) || !read_i64(row[4], initialized) ||
        !read_i64(row[5], revision) || dead < 0 || dead > 1 ||
        initialized < 0 || initialized > 1) {
        return {OriginalVitalsLoadState::Malformed, std::nullopt};
    }
    if (initialized == 0) {
        if (dead != 0 || revision != 0 ||
            !std::holds_alternative<std::monostate>(row[0]) ||
            !std::holds_alternative<std::monostate>(row[1]) ||
            !std::holds_alternative<std::monostate>(row[2])) {
            return {OriginalVitalsLoadState::Malformed, std::nullopt};
        }
        return {OriginalVitalsLoadState::Pending, std::nullopt};
    }
    OriginalVitalsRecord record;
    record.player_id = player_id;
    record.dead = dead == 1;
    record.revision = revision;
    if (!read_u32(row[0], record.hp) || !read_u32(row[1], record.shield) ||
        !read_u32(row[2], record.mp) || !valid_original_vitals_record(record)) {
        return {OriginalVitalsLoadState::Malformed, std::nullopt};
    }
    return {OriginalVitalsLoadState::Ready, record};
}

mxh::db::DbResult insert_pending_original_vitals(
    mxh::db::IDbAdapter& db, std::uint32_t player_id) {
    if (player_id == 0) {
        return {mxh::db::DbError::ConstraintViolation, "zero player ID cannot own vitals"};
    }
    return db.execute(
        "INSERT INTO modern_player_vitals"
        "(player_id,hp,shield,mp,dead,initialized,revision,updated_at) "
        "VALUES(?,NULL,NULL,NULL,0,0,0,CURRENT_TIMESTAMP)",
        {mxh::db::bind(static_cast<std::int64_t>(player_id))});
}

OriginalVitalsCommitState initialize_original_vitals(
    mxh::db::IDbAdapter& db, const OriginalVitalsRecord& initial,
    const OriginalVitalsCompanionWrite& companion) {
    if (!valid_original_vitals_record(initial) || initial.revision != 1 || initial.dead) {
        return OriginalVitalsCommitState::KnownNotCommitted;
    }
    bool begun = false;
    bool committing = false;
    try {
        if (!db.begin_transaction().ok()) {
            retire_connection(db);  // Never roll back a transaction we did not begin.
            return OriginalVitalsCommitState::Unknown;
        }
        begun = true;
        const auto write = db.execute(
            "UPDATE modern_player_vitals SET hp=?,shield=?,mp=?,dead=0,initialized=1,"
            "revision=1,updated_at=CURRENT_TIMESTAMP "
            "WHERE player_id=? AND initialized=0 AND revision=0 AND dead=0 "
            "AND hp IS NULL AND shield IS NULL AND mp IS NULL",
            {mxh::db::bind(static_cast<std::int64_t>(initial.hp)),
             mxh::db::bind(static_cast<std::int64_t>(initial.shield)),
             mxh::db::bind(static_cast<std::int64_t>(initial.mp)),
             mxh::db::bind(static_cast<std::int64_t>(initial.player_id))});
        if (!write.ok() || write.rows_affected != 1) return confirmed_rollback_or_unknown(db);
        if (companion) {
            const auto result = companion(db);
            if (!result.ok()) return confirmed_rollback_or_unknown(db);
        }
        committing = true;
        return db.commit().ok() ? OriginalVitalsCommitState::Committed
                                : uncertain_commit(db);
    } catch (...) {
        if (committing) return uncertain_commit(db);
        if (begun) return confirmed_rollback_or_unknown(db);
        retire_connection(db);
        return OriginalVitalsCommitState::Unknown;
    }
}

OriginalVitalsCommitState commit_original_vitals(
    mxh::db::IDbAdapter& db,
    const OriginalVitalsRecord& expected,
    const OriginalVitalsRecord& candidate,
    const OriginalVitalsCompanionWrite& companion) {
    if (!valid_original_vitals_record(expected) ||
        !valid_original_vitals_record(candidate) ||
        expected.player_id != candidate.player_id ||
        expected.revision >= kMaxRevision ||
        candidate.revision != expected.revision + 1) {
        return OriginalVitalsCommitState::KnownNotCommitted;
    }
    bool begun = false;
    bool committing = false;
    try {
        if (!db.begin_transaction().ok()) {
            retire_connection(db);  // Never roll back a transaction we did not begin.
            return OriginalVitalsCommitState::Unknown;
        }
        begun = true;
        const auto write = db.execute(
            "UPDATE modern_player_vitals SET hp=?,shield=?,mp=?,dead=?,"
            "revision=revision+1,updated_at=CURRENT_TIMESTAMP "
            "WHERE player_id=? AND initialized=1 AND revision=? "
            "AND hp=? AND shield=? AND mp=? AND dead=? "
            "AND revision < 9223372036854775806",
            {mxh::db::bind(static_cast<std::int64_t>(candidate.hp)),
             mxh::db::bind(static_cast<std::int64_t>(candidate.shield)),
             mxh::db::bind(static_cast<std::int64_t>(candidate.mp)),
             mxh::db::bind(static_cast<std::int64_t>(candidate.dead)),
             mxh::db::bind(static_cast<std::int64_t>(expected.player_id)),
             mxh::db::bind(expected.revision),
             mxh::db::bind(static_cast<std::int64_t>(expected.hp)),
             mxh::db::bind(static_cast<std::int64_t>(expected.shield)),
             mxh::db::bind(static_cast<std::int64_t>(expected.mp)),
             mxh::db::bind(static_cast<std::int64_t>(expected.dead))});
        if (!write.ok()) return confirmed_rollback_or_unknown(db);
        if (write.rows_affected != 1) {
            return confirmed_rollback_or_unknown(db, OriginalVitalsCommitState::Conflict);
        }
        if (companion) {
            const auto other = companion(db);
            if (!other.ok()) return confirmed_rollback_or_unknown(db);
        }
        committing = true;
        return db.commit().ok() ? OriginalVitalsCommitState::Committed
                                : uncertain_commit(db);
    } catch (...) {
        if (committing) return uncertain_commit(db);
        if (begun) return confirmed_rollback_or_unknown(db);
        retire_connection(db);
        return OriginalVitalsCommitState::Unknown;
    }
}

OriginalVitalsCommitState commit_original_vitals_for_session(
    mxh::db::IDbAdapter& db,
    OriginalVitalsSession& session,
    const OriginalVitalsRecord& candidate,
    const OriginalVitalsCompanionWrite& companion) {
    if (!session.can_publish()) return OriginalVitalsCommitState::Unknown;
    const auto result = commit_original_vitals(db, session.record, candidate, companion);
    if (result == OriginalVitalsCommitState::Committed) {
        session.record = candidate;
    } else if (result == OriginalVitalsCommitState::Unknown ||
               result == OriginalVitalsCommitState::Conflict) {
        session.quarantined = true;
    }
    return result;
}

}  // namespace mxh::server
