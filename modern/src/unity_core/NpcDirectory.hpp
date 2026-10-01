#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <cmath>
#include <string>
#include "mxh/compat/map_change_catalog.hpp"

namespace mxh::unity {

// Owner-thread state derived exclusively from this session's NpcAdd messages.
// Keep the original name bytes: MapChange.bin indexes objects in legacy encoding.
class NpcDirectory final {
public:
    struct Entry {
        std::uint16_t kind;
        std::uint16_t x;
        std::uint16_t z;
        std::string raw_name;
    };

    explicit NpcDirectory(std::size_t capacity = 4096) : m_capacity(capacity) {}

    bool upsert(std::uint32_t id, Entry entry) {
        if (id == 0 || entry.raw_name.size() > 64 || entry.raw_name.find('\0') != std::string::npos) return false;
        const auto found = m_entries.find(id);
        if (found != m_entries.end()) {
            found->second = entry;
            return true;
        }
        if (m_entries.size() >= m_capacity) return false;
        m_entries.emplace(id, entry);
        return true;
    }

    std::optional<Entry> find(std::uint32_t id) const {
        const auto found = m_entries.find(id);
        if (found == m_entries.end()) return std::nullopt;
        return found->second;
    }

    void erase(std::uint32_t id) { m_entries.erase(id); }
    std::optional<std::uint16_t> destination(std::uint32_t id, std::uint16_t map,
        const mxh::compat::MapChangeCatalog& catalog) const {
        const auto npc = find(id);
        if (!npc || npc->raw_name.empty()) return std::nullopt;
        std::optional<std::uint16_t> result;
        for (const auto& route : catalog.entries) {
            if (route.current_map_num != map || route.object_name != npc->raw_name) continue;
            if (route.move_map_num == 0 || route.move_map_num == map) return std::nullopt;
            if (result && *result != route.move_map_num) return std::nullopt;
            result = route.move_map_num;
        }
        // The audited PlayDH static NPC names and route names use different
        // locales. Only an exact, unique map/coordinate join may recover them.
        if (!result) {
            std::size_t matches = 0;
            for (const auto& route : catalog.entries) {
                if (route.current_map_num != map || route.current_x != npc->x || route.current_z != npc->z) continue;
                ++matches;
                if (route.move_map_num == 0 || route.move_map_num == map) return std::nullopt;
                result = route.move_map_num;
            }
            if (matches != 1) return std::nullopt;
        }
        return result;
    }
    bool in_range(std::uint32_t id, float x, float z, float radius) const {
        const auto npc = find(id);
        if (!npc || !std::isfinite(x) || !std::isfinite(z) ||
            !std::isfinite(radius) || radius < 0) return false;
        const double dx = static_cast<double>(npc->x) - x;
        const double dz = static_cast<double>(npc->z) - z;
        return dx * dx + dz * dz <= static_cast<double>(radius) * radius;
    }
    void clear() noexcept { m_entries.clear(); }

private:
    const std::size_t m_capacity;
    std::unordered_map<std::uint32_t, Entry> m_entries;
};

} // namespace mxh::unity
