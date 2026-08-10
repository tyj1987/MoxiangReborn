// mxh/src/services/MoveServiceImpl.hpp
// Phase 13.2: Real IMoveService implementation backed by an in-memory
// catalog of known teleport points.
//
// 1:1 quirk: legacy cMoveDialog reads the teleport list from the
// MAPINFO singleton + a per-player known-points table.  The modern
// service stores the same data (a vector<MovePoint>) and exposes the
// town/saved discriminator the dialog needs to gate the town vs saved
// tab.  The MapHandler seeds this catalog when the player logs in
// (via the MP_MAPMOVEINFO wire reply) and mutates it on every new
// teleport point discovery.
//
// Architecture:
//   - The service is per-player; the dialog holds the service for the
//     lifetime of the dialog; the MapHandler owns the catalog state.
//   - Read paths (pointCount / getPoint / isKnownPoint / hasTown /
//     hasSaved) are const noexcept + thread-safe (the dialog tick
//     thread reads; the orchestrator mutates under player_mu_).
//   - Write paths (setPoints / addPoint) are non-virtual and live on
//     the service so the orchestrator can stage updates without
//     going through the interface.

#pragma once

#include "mxh/services/IMoveService.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::services {

class MoveServiceImpl final : public IMoveService {
public:
    MoveServiceImpl() = default;
    explicit MoveServiceImpl(std::vector<MovePoint> points) noexcept
        : m_points(std::move(points)) {}

    // ----- Write path (orchestrator-only) -----

    // Replace the entire catalog.  Used on player login to seed from
    // the wire-format MP_MAPMOVEINFO reply.
    void setPoints(std::vector<MovePoint> points) noexcept {
        m_points = std::move(points);
    }

    // Add a single point (or replace an existing one with the same
    // db_id).  Used when the player discovers a new teleport point.
    void addPoint(MovePoint point) {
        if (point.db_id == 0) return;  // 1:1 quirk: db_id 0 is reserved
        for (auto& existing : m_points) {
            if (existing.db_id == point.db_id) {
                existing = std::move(point);
                return;
            }
        }
        m_points.push_back(std::move(point));
    }

    void clear() noexcept { m_points.clear(); }

    // ----- Read path (interface) -----

    std::size_t pointCount() const noexcept override {
        return m_points.size();
    }

    std::optional<MovePoint> getPoint(std::size_t i) const noexcept override {
        if (i >= m_points.size()) return std::nullopt;
        return m_points[i];
    }

    bool isKnownPoint(std::uint32_t db_id) const noexcept override {
        for (const auto& p : m_points) {
            if (p.db_id == db_id) return true;
        }
        return false;
    }

    bool hasTownPoint() const noexcept override {
        for (const auto& p : m_points) {
            if (p.town) return true;
        }
        return false;
    }

    bool hasSavedPoint() const noexcept override {
        for (const auto& p : m_points) {
            if (!p.town) return true;
        }
        return false;
    }

private:
    std::vector<MovePoint> m_points;
};

}  // namespace mxh::services
