#pragma once

#include "mxh/game/effect_catalog.hpp"
#include "mxh/game/effect_timeline.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::client {

struct RuntimeEffectEvent {
    std::string effect_name;
    std::uint32_t source_object_id = 0;
    std::uint32_t target_object_id = 0;
    mxh::game::EffectTimelineEvent trigger;
    std::string unit_kind;
    std::string object_name;
    std::string sound_name;
    std::string texture_name;
    std::array<float, 3> position{};
    float radius = 0.0f;
    std::uint32_t color_index = 0;
    std::uint32_t coordinate = 0;
    std::uint32_t motion_index = 0;
    float percent = 0.0f;
    std::uint32_t duration_ms = 0;
    std::string damage_kind;
    std::uint32_t sound_id = 0;
};

class EffectRuntime {
public:
    bool load(const std::filesystem::path& playdh_root,
              std::string* error = nullptr);
    void adopt_catalog(mxh::game::EffectCatalog catalog) noexcept;
    bool start(std::string_view effect_name,
               std::uint32_t source_object_id,
               std::uint32_t target_object_id,
               std::uint64_t now_ms,
               float tick_per_frame_ms);
    bool start_by_id(std::uint32_t effect_id,
                     bool female,
                     std::uint32_t source_object_id,
                     std::uint32_t target_object_id,
                     std::uint64_t now_ms,
                     float tick_per_frame_ms);
    void advance(std::uint64_t now_ms,
                 const std::function<void(const RuntimeEffectEvent&)>& emit);
    // Stop all active instances owned by a server skill object (or target
    // object when the protocol carries that identity).
    std::size_t stop_object(std::uint32_t object_id) noexcept;
    void clear() noexcept;
    const mxh::game::EffectCatalog& catalog() const noexcept { return m_catalog; }
    std::size_t active_count() const noexcept { return m_instances.size(); }

private:
    struct Instance {
        std::string effect_name;
        std::uint32_t source_object_id = 0;
        std::uint32_t target_object_id = 0;
        mxh::game::EffectTimeline timeline;
    };
    mxh::game::EffectCatalog m_catalog;
    std::vector<Instance> m_instances;
};

} // namespace mxh::client
