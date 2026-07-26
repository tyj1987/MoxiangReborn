#include "mxh/server/traffic_log.hpp"
namespace mxh::server {
namespace {std::size_t bucket(std::uint16_t p){return p==1?0:p==2?1:p==3?2:3;} }
void traffic_clear(TrafficLog&t){t.receive_size.fill(0);t.send_size.fill(0);t.receive_num.fill(0);t.send_num.fill(0);for(auto&a:t.move_receive_size)a.fill(0);for(auto&a:t.move_receive_num)a.fill(0);for(auto&a:t.move_send_size)a.fill(0);for(auto&a:t.move_send_num)a.fill(0);}
void traffic_add_receive(TrafficLog&t,std::uint32_t c,std::uint32_t n){if(c>=traffic_mp_max)return;t.receive_size[c]+=n+40;++t.receive_num[c];}
void traffic_add_send(TrafficLog&t,std::uint32_t c,std::uint32_t n){if(c>=traffic_mp_max)return;t.send_size[c]+=n+40;++t.send_num[c];}
void traffic_add_move_receive(TrafficLog&t,std::uint32_t id,std::uint16_t p,std::uint32_t n){auto x=id<2000000?0u:1u;auto b=bucket(p);t.move_receive_size[x][b]+=n;++t.move_receive_num[x][b];}
void traffic_add_move_send(TrafficLog&t,std::uint32_t id,std::uint16_t p,std::uint32_t n){auto x=id<2000000?0u:1u;auto b=bucket(p);t.move_send_size[x][b]+=n;++t.move_send_num[x][b];}
void traffic_add_user(TrafficLog&t,std::uint32_t id,std::uint32_t c){auto it=t.users.find(id);if(it==t.users.end())t.users.emplace(id,TrafficUser{c,id,0,0,0,0,true});else it->second.login=true;}
void traffic_remove_user(TrafficLog&t,std::uint32_t id,std::uint32_t elapsed){auto it=t.users.find(id);if(it==t.users.end()||!it->second.login)return;it->second.login_time+=elapsed;it->second.valued=0;it->second.unvalued=0;it->second.login=false;}
TrafficAction traffic_add_user_packet(TrafficLog&t,std::uint32_t id,std::uint8_t c){auto it=t.users.find(id);if(it==t.users.end()||!it->second.login)return TrafficAction::none;auto&u=it->second;if(c==0||c>=traffic_mp_max)++u.unvalued;else ++u.valued;++u.total_packets;if((u.unvalued>=t.unvalued_limit)||(u.valued>=t.valued_limit))return TrafficAction::disconnect;return TrafficAction::none;}
}
[[maybe_unused]] constexpr int traffic_log_translation_unit_anchor=0;