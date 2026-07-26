#pragma once
#include <cstdint>
#include <vector>
namespace mxh::server {
inline constexpr std::uint32_t jackpot_db_update_length=60000u;
enum class JackpotActionKind : std::uint8_t { load_db, notify_agents, notify_users, notify_character };
struct JackpotAction { JackpotActionKind kind{}; std::uint32_t character_id=0,total_money=0; };
struct JackpotState { std::uint32_t total_money=0,update_length=jackpot_db_update_length,last_db_update=0; bool manager=false; };
void jackpot_init(JackpotState&);
void jackpot_start(JackpotState&,std::uint16_t server_number);
std::vector<JackpotAction> jackpot_process(JackpotState&,std::uint32_t now_ms);
std::vector<JackpotAction> jackpot_set_total_money(JackpotState&,std::uint32_t money);
JackpotAction jackpot_notify_character(const JackpotState&,std::uint32_t character_id);
}