#include "mxh/server/agent_move.hpp"
namespace mxh::server {
// MP_MOVE on agent side is pass-through; agent ensures user is in a map before forwarding.
MoveAction classify_move(const MoveRequest& r){
    if (!r.user_in_map){
        return {MoveActionKind::drop_no_map,r.protocol,r.object_id};
    }
    return {MoveActionKind::forward_to_map,r.protocol,r.object_id};
}
}
namespace { [[maybe_unused]] constexpr int agent_move_translation_unit_anchor=0; }
