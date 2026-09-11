#include "mxh/server/server.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/game/player_move_speed.hpp"
#include <atomic>
#include <limits>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <utility>

namespace mxh::server {
namespace {
std::uint64_t next_movement_epoch() {
    static std::atomic<std::uint64_t> next{1};
    auto value=next.load();
    while (value != std::numeric_limits<std::uint64_t>::max()) {
        if (next.compare_exchange_weak(value,value+1)) return value;
    }
    return 0; // Never recycle an epoch after overflow.
}
}
bool MapHandler::set_timed_movement_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(players_mu_);
    if (!connected_players_.empty() || (enabled && !fixed_tiles_)) return false;
    timed_movement_enabled_=enabled; return true;
}
std::optional<mxh::proto::movement::State> MapHandler::movement_state_locked(
    std::uint32_t player,mxh::proto::movement::StateKind kind,std::uint64_t now) {
    auto& rt=player_runtimes_.at(player);
    if (!rt.movement_epoch || rt.movement_state_sequence==std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
    mxh::proto::movement::State state;
    state.kind=kind; state.epoch=rt.movement_epoch;
    state.command_sequence=rt.movement_command_sequence;
    state.state_sequence=++rt.movement_state_sequence; state.server_time_ms=now;
    state.x=rt.movement.position().x; state.z=rt.movement.position().z;
    if (rt.movement.moving()) {
        const auto route=rt.movement.remaining_route();
        state.count=static_cast<std::uint8_t>(route.size()); state.speed=rt.movement.speed();
        for (std::size_t i=0;i<route.size();++i)
            state.points[i]={static_cast<std::uint16_t>(route[i].x),static_cast<std::uint16_t>(route[i].z)};
    }
    rt.movement_last_publish=now;
    return state;
}
void MapHandler::send_movement_state(std::uint32_t player,std::uint64_t connection,
    const mxh::proto::movement::State& state,bool observers) {
    auto payload=mxh::proto::movement::encode(state); if (!payload) return;
    mxh::net::Message message;
    message.header.category=static_cast<std::uint8_t>(mxh::proto::Category::Move);
    message.header.protocol=mxh::proto::movement::owner_state_protocol;
    message.header.object_id=player; message.payload=std::move(*payload);
    reply_(mxh::net::ConnectionId{connection},message);
    if (observers) {
        message.header.protocol=mxh::proto::movement::observer_state_protocol;
        if (state.kind==mxh::proto::movement::StateKind::Corrected) {
            auto visible=state; visible.kind=mxh::proto::movement::StateKind::Snapshot;
            message.payload=*mxh::proto::movement::encode(visible);
        }
        broadcast_except(player,message);
    }
}
void MapHandler::handle_timed_move(mxh::net::ConnectionId id,const mxh::net::Message& message) {
    namespace wire=mxh::proto::movement;
    const bool hello=message.header.protocol==wire::hello_protocol;
    if (hello && (message.payload.size()!=wire::hello_payload.size() ||
        !std::equal(message.payload.begin(),message.payload.end(),wire::hello_payload.begin()))) return;
    const auto command=hello?std::optional<wire::Command>{}:wire::decode_command(message.payload);
    if (!hello && !command) return;
    std::optional<wire::State> response;
    bool observers=false;
    {
        std::lock_guard<std::mutex> lock(players_mu_);
        if (!timed_movement_enabled_) return;
        const auto player=message.header.object_id;
        const auto pi=connected_players_.find(player); const auto ri=player_runtimes_.find(player);
        if (pi==connected_players_.end() || ri==player_runtimes_.end() || pi->second.conn_id!=id.value) return;
        const auto now=movement_now(); materialize_player_position_locked(player,now);
        auto& rt=ri->second;
        if (hello) {
            if (!rt.movement_epoch) rt.movement_epoch=next_movement_epoch();
            response=movement_state_locked(player,wire::StateKind::Snapshot,now);
        } else {
            if (!rt.movement_epoch || command->epoch!=rt.movement_epoch ||
                command->sequence<=rt.movement_command_sequence ||
                rt.movement_state_sequence==std::numeric_limits<std::uint64_t>::max()) return;
            rt.movement_command_sequence=command->sequence;
            bool accepted=rt.actor.is_active() && rt.actor.is_alive() && fixed_tiles_.has_value();
            auto from=rt.movement.position();
            std::array<mxh::game::MovementPoint,wire::max_points> route{};
            for (std::size_t i=0;i<command->count;++i) {
                const auto p=command->points[i]; route[i]={float(p.x),float(p.z)};
                if (accepted && ((command->kind==wire::CommandKind::OneTarget && fixed_tiles_->blocked(p.x,p.z)) ||
                    fixed_tiles_->trace(from.x,from.z,p.x,p.z).collision)) accepted=false;
                from=route[i];
            }
            auto kind=wire::StateKind::Corrected;
            if (accepted && command->kind==wire::CommandKind::Stop) {
                accepted=rt.movement.stop(route[0],now)==mxh::game::MovementStopResult::Accepted;
                if (accepted) kind=wire::StateKind::Stopped;
            } else if (accepted) {
                // Experimental baseline: original InitMove starts in ordinary Run.
                // Runtime lightness/Titan/status mode integration remains gated work.
                const auto speed=mxh::game::player_move_speed(mxh::game::PlayerMoveSpeedInput{});
                accepted=speed && rt.movement.start_route({route.data(),command->count},*speed,now);
                if (accepted) kind=wire::StateKind::Started;
            }
            if (!accepted) rt.movement.halt(now);
            materialize_player_position_locked(player,now);
            response=movement_state_locked(player,kind,now);
            observers=true; // Observers must also see an authoritative halt after rejection.
        }
    }
    if (response) send_movement_state(message.header.object_id,id.value,*response,observers);
}
std::uint64_t MapHandler::movement_now() const {
    if(movement_clock_) return movement_clock_();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
bool MapHandler::set_movement_clock_for_test(std::function<std::uint64_t()> clock) {
    std::lock_guard<std::mutex> lock(players_mu_);
    if(!clock || !connected_players_.empty()) return false;
    movement_clock_=std::move(clock); return true;
}
bool MapHandler::reset_player_position_locked(std::uint32_t id,float x,float z,std::uint64_t now) {
    const auto info=connected_players_.find(id); const auto runtime=player_runtimes_.find(id);
    if(info==connected_players_.end() || runtime==player_runtimes_.end()) return false;
    if(!runtime->second.movement.reset({x,z},now)) return false;
    info->second.pos_x=x; info->second.pos_z=z;
    runtime->second.actor.state().pos_x=x; runtime->second.actor.state().pos_z=z;
    return true;
}
void MapHandler::materialize_player_position_locked(std::uint32_t id,std::uint64_t now) {
    const auto info=connected_players_.find(id); const auto runtime=player_runtimes_.find(id);
    if(info==connected_players_.end() || runtime==player_runtimes_.end()) return;
    auto& motion=runtime->second.movement;
    if(!motion.initialized()) {
        if(!motion.reset({info->second.pos_x,info->second.pos_z},now)) return;
    }
    // Combat/message entry already materialized the position before changing
    // life/lifecycle. Do not add elapsed travel after that state change.
    if(!runtime->second.actor.is_active() || !runtime->second.actor.is_alive())
        (void)motion.reset(motion.position(),now);
    const auto position=motion.advance(now);
    info->second.pos_x=position.x; info->second.pos_z=position.z;
    runtime->second.actor.state().pos_x=position.x; runtime->second.actor.state().pos_z=position.z;
}
void MapHandler::materialize_positions() {
    struct Pending { std::uint32_t player; std::uint64_t connection; mxh::proto::movement::State state; };
    std::vector<Pending> pending;
    {
        std::lock_guard<std::mutex> lock(players_mu_);
        const auto now=movement_now();
        for(const auto& [id,info]:connected_players_) {
            const auto ri=player_runtimes_.find(id); if (ri==player_runtimes_.end()) continue;
            auto& rt=ri->second;
            const bool was_moving=rt.movement.moving();
            const auto old_count=rt.movement.remaining_route().size();
            materialize_player_position_locked(id,now);
            const bool transition=old_count!=rt.movement.remaining_route().size() || was_moving!=rt.movement.moving();
            const bool due=now>=rt.movement_last_publish && now-rt.movement_last_publish>=100;
            if (rt.movement_epoch && (transition || ((was_moving || rt.movement.moving()) && due))) {
                const auto state=movement_state_locked(id,mxh::proto::movement::StateKind::Snapshot,now);
                if (state) pending.push_back({id,info.conn_id,*state});
            }
        }
    }
    for (const auto& item:pending) send_movement_state(item.player,item.connection,item.state,true);
}
bool MapHandler::start_player_trajectory_for_test(std::uint32_t id,float x,float z,float speed) {
    std::lock_guard<std::mutex> lock(players_mu_);
    const auto runtime=player_runtimes_.find(id); if(runtime==player_runtimes_.end()) return false;
    const auto now=movement_now(); materialize_player_position_locked(id,now);
    return runtime->second.movement.start({x,z},speed,now);
}

bool MapHandler::load_kyunggong_catalog(const std::filesystem::path& path, std::string& error) {
    error.clear();
    std::error_code ec;
    const auto bytes=std::filesystem::file_size(path,ec);
    if(ec || bytes>256*1024) { error="lightness resource missing or oversized"; return false; }
    auto decoded=mxh::compat::read_mh_bin(path);
    if(!decoded) { error="lightness resource could not be decoded"; return false; }
    auto catalog=mxh::game::KyungGongCatalog::parse(
        std::string_view(reinterpret_cast<const char*>(decoded.value.data.data()),decoded.value.data.size()),error);
    if(!catalog) return false;
    std::lock_guard<std::mutex> lock(players_mu_);
    if(!connected_players_.empty()) { error="cannot replace lightness data with active players"; return false; }
    kyunggong_catalog_=std::move(*catalog); return true;
}
std::optional<mxh::game::KyungGongInfo> MapHandler::kyunggong_info(std::uint16_t id) {
    std::lock_guard<std::mutex> lock(players_mu_);
    const auto* row=kyunggong_catalog_.find(id);
    return row?std::optional<mxh::game::KyungGongInfo>(*row):std::nullopt;
}

bool MapHandler::install_fixed_tiles(FixedTileMap tiles) {
    std::lock_guard<std::mutex> lock(players_mu_);
    if (!tiles.width() || !tiles.height() || !connected_players_.empty()) return false;
    fixed_tiles_ = std::move(tiles);
    return true;
}

void MapHandler::handle_move(mxh::net::ConnectionId id, const mxh::net::Message& msg) {
    if (msg.header.protocol==mxh::proto::movement::hello_protocol ||
        msg.header.protocol==mxh::proto::movement::command_protocol) {
        handle_timed_move(id,msg); return;
    }
    const auto protocol = static_cast<mxh::proto::MoveProtocol>(msg.header.protocol);
    // Warp/Correction/Init are server state, never client movement authority.
    if (protocol != mxh::proto::MoveProtocol::OneTarget &&
        protocol != mxh::proto::MoveProtocol::Target &&
        protocol != mxh::proto::MoveProtocol::Stop) return;
    if (msg.payload.size() != 4) return;
    const auto player = msg.header.object_id;
    bool rejected = false;
    std::uint16_t accepted_x = 0, accepted_z = 0;
    {
        std::lock_guard<std::mutex> lock(players_mu_);
        const auto pi = connected_players_.find(player);
        const auto ri = player_runtimes_.find(player);
        if (pi == connected_players_.end() || ri == player_runtimes_.end() ||
            pi->second.conn_id != id.value) return;
        if (ri->second.movement_epoch) return; // No legacy teleport fallback after negotiation.
        auto& info = pi->second;
        std::uint16_t x = 0, z = 0;
        std::memcpy(&x, msg.payload.data(), 2);
        std::memcpy(&z, msg.payload.data() + 2, 2);
        const float dx = static_cast<float>(x) - info.pos_x;
        const float dz = static_cast<float>(z) - info.pos_z;
        // Keep the existing coarse bound for every client movement, including
        // Stop. Time/speed authority is a separate remaining migration gate.
        rejected = !ri->second.actor.is_active() || !ri->second.actor.is_alive() ||
            dx * dx + dz * dz > 5000.0f * 5000.0f || !fixed_tiles_ ||
            (protocol == mxh::proto::MoveProtocol::OneTarget && fixed_tiles_->blocked(x, z)) ||
            fixed_tiles_->trace(info.pos_x, info.pos_z, x, z).collision;
        if (rejected) {
            accepted_x = static_cast<std::uint16_t>(std::clamp(info.pos_x, 0.0f, 65535.0f));
            accepted_z = static_cast<std::uint16_t>(std::clamp(info.pos_z, 0.0f, 65535.0f));
        } else {
            (void)reset_player_position_locked(player,x,z,movement_now());
        }
    }
    if (rejected) {
        mxh::net::Message correction;
        correction.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
        correction.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction);
        correction.header.object_id = player;
        correction.payload.resize(4);
        std::memcpy(correction.payload.data(), &accepted_x, 2);
        std::memcpy(correction.payload.data() + 2, &accepted_z, 2);
        reply_(id, correction);
        return;
    }
    broadcast_except(player, msg);
}
} // namespace mxh::server
