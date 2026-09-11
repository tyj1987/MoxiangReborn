#include "mxh/server/server.hpp"
#include <algorithm>
#include <cstring>
#include <utility>

namespace mxh::server {
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
        rejected = dx * dx + dz * dz > 5000.0f * 5000.0f || !fixed_tiles_ ||
            (protocol == mxh::proto::MoveProtocol::OneTarget && fixed_tiles_->blocked(x, z)) ||
            fixed_tiles_->trace(info.pos_x, info.pos_z, x, z).collision;
        if (rejected) {
            accepted_x = static_cast<std::uint16_t>(std::clamp(info.pos_x, 0.0f, 65535.0f));
            accepted_z = static_cast<std::uint16_t>(std::clamp(info.pos_z, 0.0f, 65535.0f));
        } else {
            info.pos_x = x; info.pos_z = z;
            auto& state = ri->second.actor.state();
            state.pos_x = x; state.pos_z = z;
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
