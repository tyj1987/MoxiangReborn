#pragma once
// Tool-only live probe. No synthetic packets, actor edits, or item grants.
#include "mxh/server/fixed_tile_map.hpp"
#include <sqlite3.h>
#include <bcrypt.h>
#include <fstream>
#include <cmath>
#include <set>

inline std::string probe_sha(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),{}};
    if(!input || bytes.empty() || bytes.size()>16*1024*1024)return {};
    BCRYPT_ALG_HANDLE algorithm{};unsigned char digest[32]{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    auto status=BCryptHash(algorithm,nullptr,0,bytes.data(),static_cast<ULONG>(bytes.size()),digest,32);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(status<0)return {};
    std::string result;const char* hex="0123456789abcdef";
    for(auto b:digest){result+=hex[b>>4];result+=hex[b&15];}return result;
}

inline bool map17_sources(const std::filesystem::path& root) {
    for(const auto& entry:std::vector<std::pair<std::string,std::string>>{
        {"MonsterList.bin","fb7ee93e66ea9321577fe4a5031e98689d346b6fdbd5852e05d69ad7a952eebe"},
        {"Server/Monster_17.bin","222945b5ec068231b9e0b79df5a1ce5e9be1d57118ffe0fa1c156a0056d54ce9"},
        {"MonsterDropItemList.bin","a65baf6ec5ffae8e3af8ec2c9fd9a4878a8773f69624aea1cafa594fc07e858e"},
        {"Map/17.ttb","e6e85c1736a5ae0b0ef49bc91baa3eb3626e6ab51cd76a9e2521215b310106ec"}}) {
        const auto actual=probe_sha(root/"Resource"/entry.first);
        LOG("Map17 source=%s sha256=%s",entry.first.c_str(),actual.c_str());
        if(actual!=entry.second){LOG("FAIL: Map17 source hash mismatch");return false;}
    }
    return true;
}

inline bool probe_log_has(const std::filesystem::path& path,const std::string& needle) {
    std::fflush(stderr);
    std::ifstream in(path);std::string line;
    while(std::getline(in,line))if(line.find(needle)!=std::string::npos)return true;
    return false;
}

#include "map17_evidence.hpp"

inline bool map17_combat_probe(mxh::client::CInGameState& game,mxh::client::CEngine& engine,
    std::uint32_t character,int timeout,const std::filesystem::path& root,
    const std::string& db,const std::filesystem::path& log) {
    using Clock=std::chrono::steady_clock;
    const auto started=Clock::now();
    const auto deadline=started+std::chrono::seconds(timeout*15);
    LOG("Map17 probe deadline_ms=%d",timeout*15000);
    const auto pump=[&](){game.Process();std::this_thread::sleep_for(std::chrono::milliseconds(25));};
    while(game.monsters().size()<216 && Clock::now()<deadline)pump();
    if(game.game_info().map_num!=17 || game.monsters().size()!=216){LOG("FAIL: Map17 admission/population");return false;}
    for(const auto& item:game.game_info().items.Inventory)
        if(!mxh::game::is_empty_slot(item)){LOG("FAIL: probe requires fresh empty carried inventory");return false;}
    std::string error;
    auto tiles=mxh::server::FixedTileMap::load(root/"Resource/Map/17.ttb",error);
    if(!tiles){LOG("FAIL: Map17 collision load");return false;}
    const int width=tiles->width(),height=tiles->height();
    const auto cell=[&](float x,float z){return static_cast<int>(z/50)*width+static_cast<int>(x/50);};
    if(tiles->blocked(game.local_x(),game.local_z())){LOG("FAIL: initial position blocked");return false;}
    std::unordered_map<int,mxh::client::MonsterAddInfo> goals;
    for(const auto& m:game.monsters())if(m.monster_kind==1 && m.current_life && !tiles->blocked(m.position_x,m.position_z))
        goals.emplace(cell(m.position_x,m.position_z),m);
    std::vector<int> previous(width*height,-1),queue;
    int start=cell(game.local_x(),game.local_z()),found=-1;
    previous[start]=start;queue.push_back(start);
    for(std::size_t cursor=0;cursor<queue.size() && Clock::now()<deadline;++cursor){
        int current=queue[cursor],x=current%width,z=current/width;
        if(goals.count(current)){found=current;break;}
        for(auto [dx,dz]:{std::pair{1,0},std::pair{-1,0},std::pair{0,1},std::pair{0,-1}}){
            int nx=x+dx,nz=z+dz;if(nx<0||nz<0||nx>=width||nz>=height)continue;
            int next=nz*width+nx;
            if(previous[next]>=0||tiles->blocked(nx*50+25,nz*50+25))continue;
            previous[next]=current;queue.push_back(next);
        }
    }
    if(found<0){LOG("FAIL: no reachable live kind1");return false;}
    const auto chosen=goals.at(found); // Copy: Process can invalidate monster vector iterators.
    std::vector<int> path;
    for(int n=found;n!=start;n=previous[n])path.push_back(n);
    std::reverse(path.begin(),path.end());
    LOG("Map17 target=%u kind=1 initial_hp=%u path_cells=%zu",chosen.object_id,chosen.current_life,path.size());
    for(int n:path){
        if(Clock::now()>=deadline){LOG("FAIL: movement deadline");return false;}
        const auto x=static_cast<std::uint16_t>((n%width)*50+25),z=static_cast<std::uint16_t>((n/width)*50+25);
        if(tiles->trace(game.local_x(),game.local_z(),x,z).collision){LOG("FAIL: route segment blocked after correction");return false;}
        game.send_move(x,z,mxh::proto::MoveProtocol::OneTarget);pump();
    }
    const auto click=[&](float x,float z){
        // Existing public smoke camera control; only rotates the logical
        // projection, never moves the actor or selects a target directly.
        game.set_camera_yaw(std::atan2(x-game.local_x(),z-game.local_z()));
        float sx=0,sy=0;
        if(!mxh::client::project_npc_to_screen(game.local_x(),game.local_z(),game.camera_yaw(),x,z,sx,sy) ||
            !std::isfinite(sx)||!std::isfinite(sy)||sx<0||sx>=800||sy<0||sy>=600)return false;
        const int px=static_cast<int>(std::lround(sx)),py=static_cast<int>(std::lround(sy));
        game.OnMouseButton(true,true,px,py);game.OnMouseButton(true,false,px,py);return true;
    };
    bool hit=false,life_changed=false,killed=false,missing_logged=false;
    mxh::client::GroundDropInfo drop{};auto next_attack=Clock::now();
    while(Clock::now()<deadline){
        pump();
        for(const auto& event:game.drain_effect_events()) {
            if(event.target_object_id!=chosen.object_id)continue;
            if(event.kind==mxh::client::EffectEventKind::Hit)hit=true;
            // Death is emitted by the authoritative LifeNotify transition.
            // Neither disappearance nor a damage feedback event proves death.
            if(event.kind==mxh::client::EffectEventKind::Death){
                killed=true;LOG("Map17 authoritative death target=%u",chosen.object_id);
            }
        }
        auto target=std::find_if(game.monsters().begin(),game.monsters().end(),[&](const auto&m){return m.object_id==chosen.object_id;});
        if(target==game.monsters().end()){
            if(!missing_logged){LOG("Map17 target absent id=%u death_confirmed=%d",chosen.object_id,killed);missing_logged=true;}
        }
        else {
            life_changed|=target->current_life<chosen.current_life;
            if(target->current_life && Clock::now()>=next_attack){
                if(!click(target->position_x,target->position_z)){LOG("FAIL: target projection outside clickable canvas");return false;}
                if(game.last_attack_target()!=0 && game.last_attack_target()!=chosen.object_id){LOG("FAIL: click selected another monster");return false;}
                next_attack=Clock::now()+std::chrono::milliseconds(850);
            }
        }
        for(const auto& candidate:game.ground_drops())if(candidate.source_monster_id==chosen.object_id){drop=candidate;break;}
        if(killed && hit && drop.object_id)break;
    }
    if(!hit || !killed || !drop.object_id || (drop.item_id!=8000 && drop.item_id!=8007 && drop.item_id!=8500) || drop.count!=1){
        LOG("Map17 combat elapsed_ms=%lld deadline_reached=%d",static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count()),Clock::now()>=deadline);
        LOG("FAIL: kill/natural-drop hit=%d life_change=%d killed=%d source=%u drop=%u item=%u",hit,life_changed,killed,drop.source_monster_id,drop.object_id,drop.item_id);
        LOG("combat state player_hp=%u local=%u,%u last_attack=%u skill_error=%s",game.game_info().life,game.local_x(),game.local_z(),game.last_attack_target(),game.last_skill_error().c_str());return false;
    }
    LOG("Map17 natural drop=%u source=%u item=%u count=%u",drop.object_id,drop.source_monster_id,drop.item_id,drop.count);
    const float dx=drop.position_x-game.local_x(),dz=drop.position_z-game.local_z();
    if(dx*dx+dz*dz>500*500 || !click(drop.position_x,drop.position_z) || game.pending_pickup_drop()!=drop.object_id){
        LOG("FAIL: pickup range/click did not select matched drop");return false;
    }
    const std::string receipt="CInGameState: picked up drop="+std::to_string(drop.object_id)+" item="+
        std::to_string(drop.item_id)+" count="+std::to_string(drop.count)+" inventory=";
    mxh::game::ItemBase acquired{};
    while(Clock::now()<deadline){
        pump();
        for(const auto& item:game.game_info().items.Inventory)
            if(item.dwDBIdx && item.wIconIdx==drop.item_id && item.ItemParam==drop.count)acquired=item;
        if(acquired.dwDBIdx && probe_log_has(log,receipt) && probe_sqlite_item(db,character,acquired))break;
    }
    if(!acquired.dwDBIdx || !probe_log_has(log,receipt) || !probe_sqlite_item(db,character,acquired)){
        LOG("FAIL: matching PickupAck/DBID/count not confirmed");return false;
    }
    LOG("Map17 pickup confirmed dbid=%u item=%u quantity=%u slot=%u",acquired.dwDBIdx,acquired.wIconIdx,acquired.ItemParam,acquired.Position);
    game.send_gameout_syn();
    while(Clock::now()<deadline && !probe_log_has(log,"CInGameState: GameOutAck"))pump();
    if(!probe_log_has(log,"CInGameState: GameOutAck")){LOG("FAIL: GameOutAck deadline");return false;}
    game.Release();
    if(!probe_sqlite_item(db,character,acquired)){LOG("FAIL: read-only SQLite after GameOut");return false;}
    mxh::client::CInGameState relog;relog.Start(&engine,character,17);
    while(Clock::now()<deadline && !relog.is_in_game()){relog.Process();std::this_thread::sleep_for(std::chrono::milliseconds(25));}
    bool matched=false;
    if(relog.is_in_game() && relog.game_info().map_num==17)
        for(const auto& item:relog.game_info().items.Inventory)
            if(item.dwDBIdx==acquired.dwDBIdx && item.wIconIdx==acquired.wIconIdx && item.ItemParam==acquired.ItemParam && item.Position==acquired.Position)matched=true;
    matched=matched && probe_sqlite_item(db,character,acquired);relog.Release();
    LOG("Map17 persistence same_dbid_quantity=%s visual_acceptance=false",matched?"PASS":"FAIL");
    return matched;
}
