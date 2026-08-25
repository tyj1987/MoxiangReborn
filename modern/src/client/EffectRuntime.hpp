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
};

class EffectRuntime {
public:
    bool load(const std::filesystem::path& playdh_root,
              std::string* error = nullptr);
    bool start(std::string_view effect_name,
               std::uint32_t source_object_id,
               std::uint32_t target_object_id,
               std::uint64_t now_ms,
               std::uint32_t tick_per_frame_ms);
    bool start_by_id(std::uint32_t effect_id,
                     bool female,
                     std::uint32_t source_object_id,
                     std::uint32_t target_object_id,
                     std::uint64_t now_ms,
                     std::uint32_t tick_per_frame_ms);
    void advance(std::uint64_t now_ms,
                 const std::function<void(const RuntimeEffectEvent&)>& emit);
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
