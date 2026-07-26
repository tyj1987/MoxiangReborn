#include "mxh/server/jackpot_manager_agent.hpp"
namespace mxh::server {
void jackpot_init(JackpotState&s){s={};s.update_length=jackpot_db_update_length;}
void jackpot_start(JackpotState&s,std::uint16_t n){s.manager=(n==0);}
std::vector<JackpotAction> jackpot_process(JackpotState&s,std::uint32_t now){if(!s.manager||now-s.last_db_update<s.update_length)return {};s.last_db_update=now;return {{JackpotActionKind::load_db,0,s.total_money}};}
std::vector<JackpotAction> jackpot_set_total_money(JackpotState&s,std::uint32_t m){s.total_money=m;return {{JackpotActionKind::notify_agents,0,m},{JackpotActionKind::notify_users,0,m}};}
JackpotAction jackpot_notify_character(const JackpotState&s,std::uint32_t id){return {JackpotActionKind::notify_character,id,s.total_money};}
}
[[maybe_unused]] constexpr int jackpot_manager_agent_translation_unit_anchor=0;