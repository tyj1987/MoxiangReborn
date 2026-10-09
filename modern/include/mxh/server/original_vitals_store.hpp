#pragma once

#include "mxh/db/db_adapter.hpp"

#include <cstdint>
#include <functional>
#include <limits>
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

// LifeAck and ShieldAck carry signed 32-bit changes to an authoritative
// uint32 value. Reject an unrepresentable change before its DB transaction.
[[nodiscard]] inline std::optional<std::int32_t> original_vitals_wire_delta(
    std::uint32_t before, std::uint32_t after) noexcept {
    const auto value = static_cast<std::int64_t>(after) - before;
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max()) return std::nullopt;
    return static_cast<std::int32_t>(value);
}

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

// A known failure requires a confirmed rollback. Conflict means that the
// persisted revision or values no longer match this session, so it must be
// quarantined despite the confirmed rollback. Commit errors, lost replies,
// and failed rollback are unknown and also quarantine the old session.
enum class OriginalVitalsCommitState {
    Committed,
    KnownNotCommitted,
    Conflict,
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

using OriginalVitalsCompanionWrite =
    std::function<mxh::db::DbResult(mxh::db::IDbAdapter&)>;

// Initialize exactly one pending row. Companion progression normalization is
// in the same transaction. A caller must re-read on uncertain outcomes; this
// routine never reconstructs a missing older player's state.
[[nodiscard]] OriginalVitalsCommitState initialize_original_vitals(
    mxh::db::IDbAdapter& db, const OriginalVitalsRecord& initial,
    const OriginalVitalsCompanionWrite& companion = {});

// The optional companion writes items, equipment, or progression in the SAME
// transaction. Its caller must check its own expected row counts and convert
// a no-op or partial write into a non-Ok DbResult; this generic API cannot
// assume every companion changes exactly one row. Nothing is published in
// memory or on the wire until Committed.
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
