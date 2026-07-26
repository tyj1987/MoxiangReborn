#include "mxh/server/agent_skill.hpp"
namespace mxh::server {
AgentSkillAction process_agent_skill_user(SkillDelayManager&m,std::uint32_t c,std::uint32_t i,std::uint32_t n){if(add_skill_use(m,c,i,n,false))return {AgentSkillActionKind::forward_to_map,c,i,22,0,0};return {AgentSkillActionKind::send_start_nack,c,i,22,2,0};}
AgentSkillAction process_agent_skill_server(SkillDelayManager&m,std::uint32_t c,std::uint32_t i,std::uint32_t n){add_skill_use(m,c,i,n,true);return {AgentSkillActionKind::forward_to_map,c,i,22,0,0};}
AgentSkillAction process_agent_skill_other(std::uint32_t c,std::uint32_t i){return {AgentSkillActionKind::forward_to_map,c,i,22,0,0};}
}
[[maybe_unused]] constexpr int agent_skill_translation_unit_anchor=0;