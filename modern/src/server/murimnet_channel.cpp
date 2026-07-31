#include "mxh/server/murimnet_channel.hpp"
#include <algorithm>
namespace mxh::server {
bool MurimNetChannel::create(const MnChannelCreateInfo& i){if(i.channel_index==0||i.max_players==0)return false;release();m_info=i;return true;}
void MurimNetChannel::release()noexcept{for(auto* p:m_players)if(p&&p->location_index==m_info.channel_index){p->location=MnPlayerLocation::None;p->location_index=0;}m_players.clear();}
bool MurimNetChannel::contains(std::uint32_t id)const noexcept{return std::any_of(m_players.begin(),m_players.end(),[&](auto* p){return p&&p->id==id;});}
bool MurimNetChannel::player_in(MnRoomPlayer& p){if(p.id==0||contains(p.id)||m_players.size()>=m_info.max_players)return false;p.location=MnPlayerLocation::Channel;p.location_index=m_info.channel_index;m_players.push_back(&p);return true;}
bool MurimNetChannel::player_out(MnRoomPlayer& p){auto it=std::find(m_players.begin(),m_players.end(),&p);if(it==m_players.end())return false;m_players.erase(it);p.location=MnPlayerLocation::None;p.location_index=0;return true;}
void MurimNetChannel::for_each_member(const std::function<void(const MnRoomPlayer&)>& fn)const{for(auto* p:m_players)if(p&&fn)fn(*p);}
bool MurimNetChannelManager::init(std::uint32_t max,const MnChannelCreateInfo& def){release();if(max==0)return false;m_maxChannels=max;auto d=def;if(d.channel_index==0){d.channel_index=1;d.max_players=16;d.title="Default Channel";}m_defaultChannel=create_channel(d);return m_defaultChannel!=nullptr;}
void MurimNetChannelManager::release()noexcept{for(auto& c:m_channels)if(c)c->release();m_channels.clear();m_defaultChannel=nullptr;m_maxChannels=0;}
MurimNetChannel* MurimNetChannelManager::create_channel(MnChannelCreateInfo i){if(m_maxChannels==0||m_channels.size()>=m_maxChannels)return nullptr;if(i.channel_index==0){for(std::uint32_t id=1;id<=m_maxChannels;++id){if(!get_channel(id)){i.channel_index=id;break;}}}if(i.channel_index==0||get_channel(i.channel_index))return nullptr;auto c=std::make_unique<MurimNetChannel>();if(!c->create(i))return nullptr;auto* out=c.get();m_channels.push_back(std::move(c));return out;}
bool MurimNetChannelManager::delete_channel(std::uint32_t id){auto it=std::find_if(m_channels.begin(),m_channels.end(),[&](auto& c){return c&&c->channel_index()==id;});if(it==m_channels.end())return false;if(it->get()==m_defaultChannel)m_defaultChannel=nullptr;(*it)->release();m_channels.erase(it);return true;}
MurimNetChannel* MurimNetChannelManager::get_channel(std::uint32_t id)noexcept{for(auto& c:m_channels)if(c&&c->channel_index()==id)return c.get();return nullptr;}
bool MurimNetChannelManager::enter_default(MnRoomPlayer& p){return m_defaultChannel&&m_defaultChannel->player_in(p);}
bool MurimNetChannelManager::exit(MnRoomPlayer& p){if(p.location_index==0)return false;auto* c=get_channel(p.location_index);return c&&c->player_out(p);}
}
