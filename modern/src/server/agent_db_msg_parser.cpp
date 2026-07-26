#include "mxh/server/agent_db_msg_parser.hpp"
#include <algorithm>
namespace mxh::server {
void AgentDbDispatcher::register_handler(std::uint16_t id, AgentDbHandler h){if(id>=max_query||!h)return;auto it=std::find_if(handlers_.begin(),handlers_.end(),[id](const auto&x){return x.first==id;});if(it==handlers_.end())handlers_.push_back({id,std::move(h)});else it->second=std::move(h);}
bool AgentDbDispatcher::dispatch(const AgentDbResult&r)const{if(r.query_id>=max_query)return false;auto it=std::find_if(handlers_.begin(),handlers_.end(),[&](const auto&x){return x.first==r.query_id;});if(it==handlers_.end())return true;it->second(r);return true;}
std::size_t AgentDbDispatcher::handler_count()const noexcept{return handlers_.size();}
bool AgentDbDispatcher::has_slot(std::uint16_t id)const noexcept{return id<max_query;}
}
[[maybe_unused]] constexpr int agent_db_msg_parser_translation_unit_anchor=0;