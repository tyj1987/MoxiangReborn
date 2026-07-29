#include "mxh/server/agent_user.hpp"
namespace mxh::server {
bool insert_agent_user(AgentUserRecord& r,std::uint32_t key,std::uint32_t object_id){
    if (r.in_use){ return false; }
    r.info.dwAuthKey=key;
    r.info.dwObjectID=object_id;
    r.in_use=true;
    return true;
}
bool remove_agent_user(AgentUserRecord& r){
    if (!r.in_use){ return false; }
    r.in_use=false;
    r.info=AgentUserInfo{};
    return true;
}
bool assign_agent_user_map(AgentUserRecord& r,std::uint32_t channel){
    if (channel==0){ return false; }
    r.info.dwMapChannel=channel;
    return true;
}
bool toggle_agent_user_force_move(AgentUserRecord& r){
    r.info.bForceMove = r.info.bForceMove ? 0u : 1u;
    return r.info.bForceMove != 0u;
}
}
namespace { [[maybe_unused]] constexpr int agent_user_translation_unit_anchor=0; }
