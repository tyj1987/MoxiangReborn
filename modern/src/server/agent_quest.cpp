#include "mxh/server/agent_quest.hpp"
namespace mxh::server {
// MP_QUEST on agent side is pass-through; agent forwards quest lifecycle to map server.
QuestAction classify_quest(const QuestRequest& r){
    return {QuestActionKind::forward_to_map,r.protocol,r.object_id,0u};
}
}
namespace { [[maybe_unused]] constexpr int agent_quest_translation_unit_anchor=0; }
