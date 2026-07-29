// map_object.cpp - Phase 6.2 MapObject 1:1 port implementations.

#include "mxh/server/map_object.hpp"

namespace mxh::server {

void MapObject::do_die(Object* p_attacker) {
    // Legacy CMapObject::DoDie dispatches by object kind:
    //   CastleGate (kind 65) -> SIEGEWARMGR->DeleteCastleGate(GetID());
    // The siege manager is owned by a later Phase 6.x commit; here we
    // dispatch on kind so that the wire-format invariants are pinned.
    if (get_object_kind() == ObjectKind::CastleGate) {
        // CastleGate teardown is wired to SIEGEWARMGR once that module
        // is ported.  Intentionally a no-op for now.
    }
    (void)p_attacker;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int map_object_translation_unit_anchor = 0;
}
