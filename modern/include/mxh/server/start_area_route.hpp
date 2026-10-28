#pragma once

// New-character StartArea registration.
//
// Original CN character creation persists map 17 (see the PR note for the
// evidence that is actually in this tree). The agent must not substitute
// default_map_num_ when that map has no route. Startup fails when the
// configured StartArea is absent from the configured map endpoints.

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

namespace mxh::server {

inline constexpr std::uint16_t kNewCharacterStartArea = 17;

inline std::string new_character_start_area_unregistered_message(
    std::uint16_t start_area) {
    return "new-character StartArea map " +
           std::to_string(static_cast<unsigned>(start_area)) +
           " is not registered";
}

// default_endpoint_configured is true when --map-server is present. That
// endpoint is the route for default_map_num. extra_maps are the map numbers
// from --map-server-map. An empty route set does not register StartArea.
inline std::optional<std::string> configured_start_area_registration_error(
    std::uint16_t start_area,
    bool default_endpoint_configured,
    std::uint16_t default_map_num,
    const std::uint16_t* extra_maps,
    std::size_t extra_count) {
    if (start_area != 0) {
        if (default_endpoint_configured && default_map_num == start_area) {
            return std::nullopt;
        }
        for (std::size_t index = 0; index < extra_count; ++index) {
            if (extra_maps[index] == start_area) return std::nullopt;
        }
    }
    return new_character_start_area_unregistered_message(start_area);
}

// Shared with AgentServer main. Returns 1 and writes one FATAL line that
// names the map when that StartArea has no configured route.
inline int agent_startup_exit_if_start_area_unregistered(
    std::uint16_t start_area,
    bool default_endpoint_configured,
    std::uint16_t default_map_num,
    const std::uint16_t* extra_maps,
    std::size_t extra_count) {
    const auto error = configured_start_area_registration_error(
        start_area, default_endpoint_configured, default_map_num,
        extra_maps, extra_count);
    if (!error) return 0;
    std::cerr << "FATAL: " << *error << std::endl;
    return 1;
}

}  // namespace mxh::server
