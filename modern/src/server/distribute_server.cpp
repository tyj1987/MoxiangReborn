// distribute_server.cpp - stub implementation; the live wire path lives
// in DistributeNetworkMsgParser + mxh::net Asio/IOCP.

#include "mxh/server/distribute_server.hpp"

namespace mxh::server {

bool DistributeServer::bind(std::uint16_t port) noexcept {
    port_ = port;
    state_.store(DistributeState::Booting);
    state_.store(DistributeState::Listening);
    return true;
}

void DistributeServer::run() noexcept {
    state_.store(DistributeState::Running);
    // The actual Asio loop is owned by MoxianDistribute entry point;
    // here we just keep the state observable. Real integration wires
    // DistributeNetworkMsgParser into mxh::net::TcpServer.
    while (state_.load() == DistributeState::Running) {
        // Yield - real impl uses io_context::run().
    }
    state_.store(DistributeState::Stopping);
    state_.store(DistributeState::Stopped);
}

void DistributeServer::stop() noexcept {
    state_.store(DistributeState::Stopping);
}

std::vector<std::uint32_t> DistributeServer::agent_endpoints() const noexcept {
    // Legacy uses g_pServerTable->GetAgentServer(...) per channel.
    // Modern port reads from MXHAgentServer table — stubbed here as empty.
    return {};
}

}  // namespace mxh::server
