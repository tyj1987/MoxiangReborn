#include "mxh/server/murimnet_player.hpp"
namespace mxh::server {
bool MurimNetPlayer::init(const MurimNetPlayerInfo& info)noexcept{if(info.player_id==0)return false;m_info=info;m_roomState={};m_roomState.id=info.player_id;return true;}
bool MurimNetPlayerManager::init(std::uint32_t maxPlayers)noexcept{release();if(maxPlayers==0)return false;m_maxPlayers=maxPlayers;return true;}
void MurimNetPlayerManager::release()noexcept{m_players.clear();m_maxPlayers=0;}
MurimNetPlayer* MurimNetPlayerManager::add_player(const MurimNetPlayerInfo& info){if(m_maxPlayers==0||m_players.size()>=m_maxPlayers||info.player_id==0||find_player(info.player_id))return nullptr;auto p=std::make_unique<MurimNetPlayer>();if(!p->init(info))return nullptr;auto* result=p.get();m_players.emplace(info.player_id,std::move(p));return result;}
bool MurimNetPlayerManager::delete_player(std::uint32_t id){return m_players.erase(id)!=0;}
MurimNetPlayer* MurimNetPlayerManager::find_player(std::uint32_t id)noexcept{auto it=m_players.find(id);return it==m_players.end()?nullptr:it->second.get();}
const MurimNetPlayer* MurimNetPlayerManager::find_player(std::uint32_t id)const noexcept{auto it=m_players.find(id);return it==m_players.end()?nullptr:it->second.get();}
}

