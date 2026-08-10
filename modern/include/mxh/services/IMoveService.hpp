// mxh/services/IMoveService.hpp
// Phase 13.3 service interface for map teleport dialog (cMoveDialog).
//
// Tier 3 dialog (cMoveDialog) currently reads the teleport point list
// via legacy singletons (MAPINFO / GameIn->GetMoveDialog()). This
// service is the modern replacement: dialog code takes an
// IMoveService* and queries the player known-teleport points through
// it.
//
// The interface is read-only for the catalog (the list of known
// teleport points is driven by MP_MAPMOVE_SYN/ACK on the wire; the
// dialog only displays + dispatches the user selection) and exposes
// the town/saved discrimination needed by cMoveDialog::SelectMoveIdx
// (the dialog needs to know whether the selected index is a town
// teleport or a player-saved point to gate the request correctly).
//
// Write paths (addPoint, removePoint) are out of scope; they belong
// to the network layer that mediates with the map server.
//
// Usage pattern (from a future cMoveDialog::Refresh):
//   void cMoveDialog::Refresh() {
//     const auto* svc = m_moveService;
//     if (!svc) return;
//     for (std::size_t i = 0; i < svc->pointCount(); ++i) {
//       auto point = svc->getPoint(i);
//       if (!point) continue;
//       // ... render row, town vs saved discriminator
//     }
//   }

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace mxh::services {

// One teleport point entry. Mirrors the legacy MoveInfo layout
// (dwDBIdx + strName + bTown). The struct is intentionally
// trivial; the dialog renders the row and the service answers
// list queries.
struct MovePoint {
    std::uint32_t db_id = 0;
    std::string   name;
    bool          town  = false;
};

class IMoveService {
public:
    virtual ~IMoveService() = default;

    // ----- Catalog (read) -----

    // Number of known teleport points the player can choose from.
    // Must be in sync with getPoint() (returning a non-nullopt for
    // any i in [0, pointCount())).
    virtual std::size_t pointCount() const noexcept = 0;

    // Return the point entry at index i, or std::nullopt if i
    // is out of range.
    virtual std::optional<MovePoint> getPoint(std::size_t i) const noexcept = 0;

    // True if `db_id` is in the player's known teleport list.
    // Useful for guarding MP_MAPMOVE_SYN so the dialog does not
    // dispatch a teleport to a point that has not been unlocked.
    virtual bool isKnownPoint(std::uint32_t db_id) const noexcept = 0;

    // True if the teleport list contains at least one town
    // teleport (for cMoveDialog to enable the town tab).
    virtual bool hasTownPoint() const noexcept = 0;

    // True if the teleport list contains at least one saved
    // (player-discovered) teleport (for cMoveDialog to enable the
    // saved tab).
    virtual bool hasSavedPoint() const noexcept = 0;
};

}  // namespace mxh::services
