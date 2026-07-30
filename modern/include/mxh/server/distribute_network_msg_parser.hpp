// distribute_network_msg_parser.hpp - 1:1 port of legacy [Server]Distribute/DistributeNetworkMsgParser.h.
//
// Dispatches MP_USERCONN/MP_MOVE/MP_CHAR/... categories on Distribute
// server. Modern port keeps the dispatch table as a flat vector of
// (protocol, handler) entries; tests verify routing without needing
// a live wire.

#pragma once

#include "mxh/proto/protocol.hpp"
#include "mxh/net/net.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace mxh::server {

// Handler return (legacy returns void; modern uses enum for test coverage).
enum class DispatchStatus : std::uint8_t {
    Handled        = 0,
    UnknownProtocol = 1,
    InvalidSession  = 2,
};

// Handler signature (legacy free function pointer; modern std::function).
using DistributeHandler = std::function<DispatchStatus(
    std::uint32_t session_id,
    std::uint16_t protocol,
    std::uint32_t object_id,
    std::uint32_t category,
    const std::vector<std::uint8_t>& payload)>;

class DistributeNetworkMsgParser final {
public:
    // Register a handler for (category, protocol).
    bool register_handler(std::uint8_t category, std::uint16_t protocol,
                            DistributeHandler h) noexcept;
    // Dispatch a packet.
    DispatchStatus dispatch(std::uint32_t session_id,
                              std::uint8_t category, std::uint16_t protocol,
                              std::uint32_t object_id,
                              const std::vector<std::uint8_t>& payload) noexcept;
    std::size_t handler_count() const noexcept { return handlers_.size(); }

    // Convenience: number of handlers per category (test introspection).
    std::size_t count_for_category(std::uint8_t category) const noexcept;

private:
    std::map<std::pair<std::uint8_t, std::uint16_t>, DistributeHandler> handlers_;
};

}  // namespace mxh::server
