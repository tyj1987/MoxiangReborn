#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace mxh::server {
inline constexpr std::size_t max_query = 76u;
enum class AgentDbQuery : std::uint16_t { character_base=1, create_character=2, login_check_delete=3, delete_character=4, name_check=5, friend_add=12, friend_list=19, note_list=21, wanted_delete=24, gm_ban_character=26, gm_update_user_level=27, gm_where_is=28, gm_login=29, gm_power_list=30, jackpot_total_money=39, field_war_check_money=40, field_war_add_money=41, chase_find_user=42, character_slot=43, update_user_state=47, punish_list_load=52, punish_count_add=54 };
struct AgentDbResult { std::uint16_t query_id=0; std::uint32_t connection_index=0; int result=0; std::vector<std::vector<std::string_view>> rows; };
using AgentDbHandler=std::function<void(const AgentDbResult&)>;
class AgentDbDispatcher {
public:
 void register_handler(std::uint16_t query_id, AgentDbHandler handler);
 bool dispatch(const AgentDbResult& result) const;
 std::size_t handler_count() const noexcept;
 bool has_slot(std::uint16_t query_id) const noexcept;
private:
 std::vector<std::pair<std::uint16_t,AgentDbHandler>> handlers_;
};
}