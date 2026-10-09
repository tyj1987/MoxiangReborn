#pragma once

#include "mxh/db/db_adapter.hpp"

#include <cstdint>
#include <functional>
#include <optional>

namespace mxh::server {

// The pending row is created in the same transaction as CharacterMake. It is
// deliberately not usable as a full-health login state until Map initializes
// it from the newly created character's verified base/equipment attributes.
struct OriginalVitalsRecord {
    std::uint32_t player_id = 0;
    std::uint32_t hp = 0;
    std::uint32_t shield = 0;
    std::uint32_t mp = 0;
    bool dead = false;
    std::int64_t revision = 0;
};

enum class OriginalVitalsLoadState {
    Ready,
    Pending,
    Missing,
    Malformed,
    DbError,
};

struct OriginalVitalsLoadResult {
    OriginalVitalsLoadState state = OriginalVitalsLoadState::DbError;
    std::optional<OriginalVitalsRecord> record;
};

// A known failure requires a confirmed rollback. Commit errors, lost replies,
// and failed rollback are unknown; the caller must quarantine its old session
// until a fresh login reads player ID, revision, and all four vitals fields.
enum class OriginalVitalsCommitState {
    Committed,
    KnownNotCommitted,
    Unknown,
};

[[nodiscard]] bool valid_original_vitals_record(const OriginalVitalsRecord& record) noexcept;

struct OriginalVitalsSession {
    OriginalVitalsRecord record;
    bool quarantined = false;

    [[nodiscard]] bool can_publish() const noexcept {
        return !quarantined && valid_original_vitals_record(record);
    }
};

[[nodiscard]] OriginalVitalsLoadResult load_original_vitals(
    mxh::db::IDbAdapter& db, std::uint32_t player_id);

// Call only inside Agent's already-open CharacterMake transaction.
[[nodiscard]] mxh::db::DbResult insert_pending_original_vitals(
    mxh::db::IDbAdapter& db, std::uint32_t player_id);

// Initialize exactly one pending row. A caller must re-read on uncertain
// outcomes; this routine never reconstructs a missing older player's state.
[[nodiscard]] OriginalVitalsCommitState initialize_original_vitals(
    mxh::db::IDbAdapter& db, const OriginalVitalsRecord& initial);

using OriginalVitalsCompanionWrite =
    std::function<mxh::db::DbResult(mxh::db::IDbAdapter&)>;

// The optional companion writes items, equipment, or progression in the SAME
// transaction. Nothing is published in memory or on the wire until Committed.
[[nodiscard]] OriginalVitalsCommitState commit_original_vitals(
    mxh::db::IDbAdapter& db,
    const OriginalVitalsRecord& expected,
    const OriginalVitalsRecord& candidate,
    const OriginalVitalsCompanionWrite& companion = {});

// The session must be newly constructed from a fresh GameIn read; an unknown
// outcome stays quarantined even if a subsequent read sees the candidate.
[[nodiscard]] OriginalVitalsCommitState commit_original_vitals_for_session(
    mxh::db::IDbAdapter& db,
    OriginalVitalsSession& session,
    const OriginalVitalsRecord& candidate,
    const OriginalVitalsCompanionWrite& companion = {});

}  // namespace mxh::server
