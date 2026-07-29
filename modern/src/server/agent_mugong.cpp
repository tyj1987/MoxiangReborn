#include "mxh/server/agent_mugong.hpp"
namespace mxh::server {
// MP_MUGONG on agent side mostly forwards; mugong option_syn validates level gate.
MugongAction classify_mugong(const MugongRequest& r){
    if (r.protocol==mugong_option_syn && r.required_level>0 && r.mugong_level<r.required_level){
        return {MugongActionKind::send_nack,mugong_option_nack,r.object_id,1u};
    }
    return {MugongActionKind::forward_to_map,r.protocol,r.object_id,0u};
}
}
namespace { [[maybe_unused]] constexpr int agent_mugong_translation_unit_anchor=0; }
