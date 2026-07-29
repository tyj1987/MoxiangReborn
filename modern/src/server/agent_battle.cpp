#include "mxh/server/agent_battle.hpp"
namespace mxh::server {
// MP_BATTLE on agent side is pass-through to map server (battle state lives on map).
BattleAction classify_battle(const BattleRequest& r){
    return {BattleActionKind::forward_to_map,r.protocol,r.object_id};
}
}
namespace { [[maybe_unused]] constexpr int agent_battle_translation_unit_anchor=0; }
