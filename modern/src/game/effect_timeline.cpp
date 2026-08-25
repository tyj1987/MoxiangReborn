#include "mxh/game/effect_timeline.hpp"

#include <algorithm>
#include <charconv>
#include <string_view>

namespace mxh::game {
namespace {

bool parse_delay(std::string_view token, float tick_ms,
                 std::uint64_t& out) noexcept {
    if (token.empty()) return false;
    std::uint64_t value = 0;
    const auto first = token.front() == 'f' || token.front() == 'F'
        ? token.substr(1) : token;
    if (first.empty()) return false;
    const auto result = std::from_chars(first.data(), first.data() + first.size(), value);
    if (result.ec != std::errc{} || result.ptr != first.data() + first.size()) return false;
    if (token.front() == 'f' || token.front() == 'F') {
        value = static_cast<std::uint64_t>(static_cast<double>(value) * tick_ms);
    }
    out = value;
    return true;
}

} // namespace

bool EffectTimeline::start(const EffectScriptSummary& script,
                           std::uint64_t start_ms,
                           float tick_per_frame_ms) noexcept {
    reset();
    if (!script.decoded || tick_per_frame_ms == 0 ||
        script.triggers.size() != script.trigger_count) return false;
    m_schedule.reserve(script.triggers.size());
    m_units = script.units;
    for (std::size_t i = 0; i < script.triggers.size(); ++i) {
        std::uint64_t delay = 0;
        if (!parse_delay(script.triggers[i].time_token, tick_per_frame_ms, delay)) {
            reset();
            return false;
        }
        m_schedule.push_back({start_ms + delay, i, script.triggers[i]});
    }
    std::stable_sort(m_schedule.begin(), m_schedule.end(),
        [](const Scheduled& a, const Scheduled& b) {
            return a.due_ms < b.due_ms;
        });
    m_startMs = start_ms;
    m_lastNowMs = start_ms;
    m_active = true;
    return true;
}

std::vector<EffectTimelineEvent> EffectTimeline::advance(std::uint64_t now_ms) noexcept {
    std::vector<EffectTimelineEvent> out;
    if (!m_active) return out;
    // A frame arriving before the timeline clock (or an out-of-order clock
    // sample) must not release a future trigger early.  Keep the last clock
    // and wait for the next monotonic sample instead of clamping forward.
    if (now_ms < m_lastNowMs) return out;
    m_lastNowMs = now_ms;
    while (m_nextTrigger < m_schedule.size() &&
           m_schedule[m_nextTrigger].due_ms <= now_ms) {
        const auto& item = m_schedule[m_nextTrigger++];
        EffectTimelineEvent event{item.due_ms, item.index, item.trigger};
        if (item.trigger.unit < m_units.size()) {
            event.unit_kind = m_units[item.trigger.unit].kind;
            event.sound_id = m_units[item.trigger.unit].sound_id;
        }
        out.push_back(std::move(event));
    }
    if (m_nextTrigger == m_schedule.size()) m_active = false;
    return out;
}

void EffectTimeline::reset() noexcept {
    m_schedule.clear();
    m_units.clear();
    m_nextTrigger = 0;
    m_startMs = 0;
    m_lastNowMs = 0;
    m_active = false;
}

} // namespace mxh::game
