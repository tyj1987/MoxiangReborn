#pragma once
#include "mxh/server/skill_delay_manager.hpp"
#include <cstdint>
namespace mxh::server {
enum class AgentSkillActionKind : std::uint8_t { forward_to_map, send_start_nack };
struct AgentSkillAction { AgentSkillActionKind kind{}; std::uint32_t character_id=0,skill_index=0; std::uint8_t category=22,protocol=2,error=0; };
AgentSkillAction process_agent_skill_user(SkillDelayManager&,std::uint32_t character_id,std::uint32_t skill_index,std::uint32_t now_ms);
AgentSkillAction process_agent_skill_server(SkillDelayManager&,std::uint32_t character_id,std::uint32_t skill_index,std::uint32_t now_ms);
AgentSkillAction process_agent_skill_other(std::uint32_t character_id,std::uint32_t skill_index);
}