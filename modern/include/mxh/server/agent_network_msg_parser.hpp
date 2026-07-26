#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace mxh::server {

enum class AgentMessageDirection : std::uint8_t { from_user, from_server };
struct AgentMessage { std::uint32_t connection_index = 0; std::uint8_t category = 0; std::uint8_t protocol = 0; std::span<const std::uint8_t> payload{}; };
struct AgentDispatchResult { bool accepted = false; bool handled = false; std::string_view parser_name{}; };
using AgentMessageHandler = std::function<void(const AgentMessage&)>;

class AgentNetworkDispatcher {
public:
    AgentNetworkDispatcher();
    void register_handler(std::uint8_t category, std::uint8_t protocol, AgentMessageDirection direction, AgentMessageHandler handler);
    AgentDispatchResult dispatch(const AgentMessage& message, AgentMessageDirection direction) const;
    std::size_t handler_count() const noexcept;
    static std::string_view parser_name(std::uint8_t category, AgentMessageDirection direction);
private:
    struct Route { std::uint8_t category; std::uint8_t protocol; AgentMessageDirection direction; AgentMessageHandler handler; };
    std::vector<Route> routes_;
};

} // namespace mxh::server