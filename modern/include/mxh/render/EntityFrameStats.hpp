#pragma once
// DX-free per-frame entity visibility accounting for EntityScene::render().
// Kept header-only so it can be unit tested without D3D11 (see
// tests/unit/render/entity_frame_stats_test.cpp).
#include <cstdint>
#include <cstdio>
#include <string>

namespace mxh::gx {

enum class EntityFrameKind : std::uint8_t { Monster = 0, Npc = 1, Player = 2, Effect = 3 };

struct EntityKindFrameCounts {
    std::uint32_t total = 0;    // candidates considered this frame
    std::uint32_t drawn = 0;    // submitted to RenderMeshObject / RenderBox
    std::uint32_t culled = 0;   // rejected by the camera frustum
    std::uint32_t skipped = 0;  // model failed and placeholder rendering disabled

    friend bool operator==(const EntityKindFrameCounts&, const EntityKindFrameCounts&) = default;
};

struct EntityFrameStats {
    EntityKindFrameCounts kinds[4]{};

    void reset() noexcept { *this = EntityFrameStats{}; }
    EntityKindFrameCounts& at(EntityFrameKind k) noexcept {
        return kinds[static_cast<std::uint8_t>(k)];
    }
    const EntityKindFrameCounts& at(EntityFrameKind k) const noexcept {
        return kinds[static_cast<std::uint8_t>(k)];
    }
    void drawn(EntityFrameKind k) noexcept { ++at(k).total; ++at(k).drawn; }
    void culled(EntityFrameKind k) noexcept { ++at(k).total; ++at(k).culled; }
    void skipped(EntityFrameKind k) noexcept { ++at(k).total; ++at(k).skipped; }
    // A model that failed but is drawn as a debug placeholder box: the
    // skipped mark is converted into drawn/culled by the placeholder pass.
    void unskip(EntityFrameKind k) noexcept {
        auto& c = at(k);
        if (c.skipped == 0) return;
        --c.skipped; --c.total;
    }

    EntityKindFrameCounts sum() const noexcept {
        EntityKindFrameCounts s;
        for (const auto& c : kinds) {
            s.total += c.total; s.drawn += c.drawn;
            s.culled += c.culled; s.skipped += c.skipped;
        }
        return s;
    }

    friend bool operator==(const EntityFrameStats& a, const EntityFrameStats& b) {
        for (int i = 0; i < 4; ++i) if (!(a.kinds[i] == b.kinds[i])) return false;
        return true;
    }
};

// "[entity] frame drawn=M/N culled=C skipped=S monster=d/n/c/s npc=.. player=.. effect=.."
inline std::string formatEntityFrameStats(const EntityFrameStats& s) {
    const auto t = s.sum();
    const auto k = [&s](EntityFrameKind kind) { return s.at(kind); };
    const auto m = k(EntityFrameKind::Monster), n = k(EntityFrameKind::Npc),
               p = k(EntityFrameKind::Player), e = k(EntityFrameKind::Effect);
    char buf[384];
    std::snprintf(buf, sizeof(buf),
        "[entity] frame drawn=%u/%u culled=%u skipped=%u "
        "monster=%u/%u/%u/%u npc=%u/%u/%u/%u player=%u/%u/%u/%u effect=%u/%u/%u/%u",
        t.drawn, t.total, t.culled, t.skipped,
        m.drawn, m.total, m.culled, m.skipped, n.drawn, n.total, n.culled, n.skipped,
        p.drawn, p.total, p.culled, p.skipped, e.drawn, e.total, e.culled, e.skipped);
    return buf;
}

// Log throttle: emit when the stats change, but at most once per
// min_interval_ms; also emit a heartbeat every heartbeat_ms when stable.
class EntityFrameStatsThrottle {
public:
    explicit EntityFrameStatsThrottle(std::uint64_t min_interval_ms = 1000,
                                      std::uint64_t heartbeat_ms = 10000) noexcept
        : min_interval_ms_(min_interval_ms), heartbeat_ms_(heartbeat_ms) {}

    bool shouldLog(const EntityFrameStats& stats, std::uint64_t now_ms) noexcept {
        if (!has_last_) { commit(stats, now_ms); return true; }
        const std::uint64_t elapsed = now_ms >= last_ms_ ? now_ms - last_ms_ : 0;
        const bool changed = !(stats == last_);
        if ((changed && elapsed >= min_interval_ms_) || elapsed >= heartbeat_ms_) {
            commit(stats, now_ms);
            return true;
        }
        return false;
    }

private:
    void commit(const EntityFrameStats& s, std::uint64_t now) noexcept {
        last_ = s; last_ms_ = now; has_last_ = true;
    }
    std::uint64_t min_interval_ms_;
    std::uint64_t heartbeat_ms_;
    EntityFrameStats last_{};
    std::uint64_t last_ms_ = 0;
    bool has_last_ = false;
};

// Release smoke (MXH_GUI_SMOKE_EXIT=1) must fail instead of silently
// dropping entities when the entity-scene block throws.
inline bool entitySceneThrowFailsSmoke(const char* smoke_exit_env) noexcept {
    return smoke_exit_env && smoke_exit_env[0] == '1';
}

} // namespace mxh::gx
