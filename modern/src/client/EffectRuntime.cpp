#include "EffectRuntime.hpp"

namespace mxh::client {

bool EffectRuntime::load(const std::filesystem::path& root, std::string* error) {
    clear();
    return m_catalog.load(root, error);
}

bool EffectRuntime::start(std::string_view effect_name,
                          std::uint32_t source_object_id,
                          std::uint32_t target_object_id,
                          std::uint64_t now_ms,
                          float tick_per_frame_ms) {
    const auto* summary = m_catalog.script(effect_name);
    if (!summary) return false;
    Instance instance;
    instance.effect_name = std::string(effect_name);
    instance.source_object_id = source_object_id;
    instance.target_object_id = target_object_id;
    if (!instance.timeline.start(*summary, now_ms, tick_per_frame_ms)) return false;
    m_instances.push_back(std::move(instance));
    return true;
}

bool EffectRuntime::start_by_id(std::uint32_t effect_id,
                                bool female,
                                std::uint32_t source_object_id,
                                std::uint32_t target_object_id,
                                std::uint64_t now_ms,
                                float tick_per_frame_ms) {
    const auto* name = m_catalog.effect_name(effect_id, female);
    return name && start(*name, source_object_id, target_object_id,
                         now_ms, tick_per_frame_ms);
}

void EffectRuntime::advance(
    std::uint64_t now_ms,
    const std::function<void(const RuntimeEffectEvent&)>& emit) {
    for (auto it = m_instances.begin(); it != m_instances.end();) {
        const auto events = it->timeline.advance(now_ms);
        if (emit) {
            for (const auto& trigger : events) {
                emit(RuntimeEffectEvent{it->effect_name,
                                        it->source_object_id,
                                        it->target_object_id, trigger,
                                        trigger.unit_kind, trigger.object_name,
                                        trigger.sound_name, trigger.texture_name,
                                        trigger.position, trigger.radius,
                                        trigger.color_index, trigger.coordinate,
                                        trigger.motion_index, trigger.percent,
                                        trigger.duration_ms, trigger.damage_kind,
                                        trigger.sound_id});
            }
        }
        if (!it->timeline.active()) it = m_instances.erase(it);
        else ++it;
    }
}

void EffectRuntime::clear() noexcept {
    m_instances.clear();
    m_catalog = {};
}

} // namespace mxh::client
