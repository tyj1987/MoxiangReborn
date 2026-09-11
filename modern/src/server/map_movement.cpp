#include "mxh/server/server.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include <algorithm>
#include <cstring>
#include <chrono>
#include <utility>

namespace mxh::server {
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
    std::lock_guard<std::mutex> lock(players_mu_);
    const auto now=movement_now();
    for(const auto& [id,unused]:connected_players_) materialize_player_position_locked(id,now);
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
