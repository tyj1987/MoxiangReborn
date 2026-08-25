#pragma once

#include "effect_catalog.hpp"

#include <cstdint>
#include <vector>

namespace mxh::game {

struct EffectTimelineEvent {
    std::uint64_t due_ms = 0;
    std::size_t trigger_index = 0;
    EffectScriptSummary::Trigger trigger;
};

// Deterministic execution clock for a decoded BEFF script.  The caller owns
// the authoritative tick-per-frame value; no default frame duration is
// invented here.  `advance` is monotonic and each trigger is emitted once.
class EffectTimeline {
public:
    bool start(const EffectScriptSummary& script,
               std::uint64_t start_ms,
               float tick_per_frame_ms) noexcept;
    std::vector<EffectTimelineEvent> advance(std::uint64_t now_ms) noexcept;
    void reset() noexcept;

    bool active() const noexcept { return m_active; }
    std::size_t next_trigger() const noexcept { return m_nextTrigger; }
    std::uint64_t start_ms() const noexcept { return m_startMs; }

private:
    struct Scheduled {
        std::uint64_t due_ms = 0;
        std::size_t index = 0;
        EffectScriptSummary::Trigger trigger;
    };
    std::vector<Scheduled> m_schedule;
    std::size_t m_nextTrigger = 0;
    std::uint64_t m_startMs = 0;
    std::uint64_t m_lastNowMs = 0;
    bool m_active = false;
};

} // namespace mxh::game
