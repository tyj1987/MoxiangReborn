#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>
namespace mxh::server {
inline constexpr std::size_t traffic_mp_max=96u;
enum class TrafficAction : std::uint8_t { none, disconnect };
struct TrafficUser { std::uint32_t connection_index=0,user_id=0,login_time=0,total_packets=0,valued=0,unvalued=0;bool login=false; };
struct TrafficLog { std::array<std::uint32_t,traffic_mp_max> receive_size{},send_size{},receive_num{},send_num{}; std::array<std::array<std::uint32_t,4>,2> move_receive_size{},move_receive_num{},move_send_size{},move_send_num{}; std::unordered_map<std::uint32_t,TrafficUser> users; std::uint32_t check_time=0,unvalued_limit=0,valued_limit=0; };
void traffic_clear(TrafficLog&);
void traffic_add_receive(TrafficLog&,std::uint32_t category,std::uint32_t length);
void traffic_add_send(TrafficLog&,std::uint32_t category,std::uint32_t length);
void traffic_add_move_receive(TrafficLog&,std::uint32_t object_id,std::uint16_t protocol,std::uint32_t length);
void traffic_add_move_send(TrafficLog&,std::uint32_t object_id,std::uint16_t protocol,std::uint32_t length);
void traffic_add_user(TrafficLog&,std::uint32_t user_id,std::uint32_t connection_index);
void traffic_remove_user(TrafficLog&,std::uint32_t user_id,std::uint32_t elapsed_ms);
TrafficAction traffic_add_user_packet(TrafficLog&,std::uint32_t user_id,std::uint8_t category);
}