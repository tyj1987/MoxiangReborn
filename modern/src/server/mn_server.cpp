// mn_server.cpp

#include "mxh/server/mn_server.hpp"

namespace mxh::server {

bool MNServer::bind(std::uint16_t port) noexcept {
    port_ = port;
    state_.store(MnServerState::Booting);
    state_.store(MnServerState::Listening);
    return true;
}

void MNServer::stop() noexcept {
    state_.store(MnServerState::Stopped);
}

}  // namespace mxh::server
