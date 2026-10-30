#pragma once
#include "mxh/db/modern_shop_state.hpp"

namespace mxh::db {
struct ReviveProgress {
    std::uint16_t level = 1;
    std::uint32_t money = 0;
    std::uint64_t experience = 0;
};
enum class ReviveCommit { Committed, Rejected, Uncertain };

// Owns the transaction; never call inside another transaction. The server must
// fence the live session until this returns and drain on Uncertain.
inline ReviveCommit commit_revive_state(IDbAdapter& db, std::uint32_t player,
    std::uint32_t account, const ModernShopState& old_record,
    const ReviveProgress& before, const ReviveProgress& after,
    const LegacyShopAppearanceRows& shop, PersistedVitals vitals,
    const std::optional<std::vector<PersistedPet>>& pets = std::nullopt) {
    if (!player || !account || !before.level || !after.level ||
        before.experience > 0x7fffffffffffffffULL || after.experience > 0x7fffffffffffffffULL)
        return ReviveCommit::Rejected;
    bool began=false, commit_attempted=false;
    try {
        if (!db.begin_transaction().ok()) return ReviveCommit::Uncertain;
        began=true;
        const auto update=db.execute(
            "UPDATE modern_player_state SET level=?,exp=?,money=? "
            "WHERE player_id=? AND level=? AND exp=? AND money=?",
            {bind(static_cast<std::int64_t>(after.level)),bind(static_cast<std::int64_t>(after.experience)),
             bind(static_cast<std::int64_t>(after.money)),bind(static_cast<std::int64_t>(player)),
             bind(static_cast<std::int64_t>(before.level)),bind(static_cast<std::int64_t>(before.experience)),
             bind(static_cast<std::int64_t>(before.money))});
        bool ok=update.ok() && update.rows_affected==1;
        if (ok) {
            const auto identity=db.execute("UPDATE character_info SET level=? WHERE chrid=? AND userid=?",
                {bind(static_cast<std::int64_t>(after.level)),bind(static_cast<std::int64_t>(player)),
                 mxh::db::bind(std::to_string(account))});
            ok=identity.ok() && identity.rows_affected==1;
        }
        if (ok) ok=save_modern_shop_state(db,player,account,old_record,shop,vitals,pets);
        if (ok) {
            commit_attempted=true;
            if (db.commit().ok()) return ReviveCommit::Committed;
        }
    } catch (...) {
        // Begin/commit exceptions may leave ownership or durability unknown.
    }
    bool rolled_back=false;
    if (began) { try { rolled_back=db.rollback().ok(); } catch (...) {} }
    return began && rolled_back && !commit_attempted ? ReviveCommit::Rejected : ReviveCommit::Uncertain;
}
} // namespace mxh::db
