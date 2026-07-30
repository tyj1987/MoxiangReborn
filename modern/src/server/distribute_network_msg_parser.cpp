// distribute_network_msg_parser.cpp

#include "mxh/server/distribute_network_msg_parser.hpp"

namespace mxh::server {

bool DistributeNetworkMsgParser::register_handler(std::uint8_t category,
                                                    std::uint16_t protocol,
                                                    DistributeHandler h) noexcept {
    if (!h) return false;
    handlers_[{category, protocol}] = std::move(h);
    return true;
}

DispatchStatus DistributeNetworkMsgParser::dispatch(std::uint32_t session_id,
                                                      std::uint8_t category,
                                                      std::uint16_t protocol,
                                                      std::uint32_t object_id,
                                                      const std::vector<std::uint8_t>& payload) noexcept {
    auto it = handlers_.find({category, protocol});
    if (it == handlers_.end()) return DispatchStatus::UnknownProtocol;
    auto status = it->second(session_id, protocol, object_id, category, payload);
    if (status == DispatchStatus::Handled) return DispatchStatus::Handled;
    return status;
}

std::size_t DistributeNetworkMsgParser::count_for_category(std::uint8_t category) const noexcept {
    std::size_t n = 0;
    for (const auto& kv : handlers_) {
        if (kv.first.first == category) ++n;
    }
    return n;
}

}  // namespace mxh::server
