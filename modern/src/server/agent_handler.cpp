// agent_handler.cpp - AgentServer handler (per-user agent).
//
// Phase 4 demo: handles MP_USERCONN_CHARACTERLIST_SYN → returns
// character list from DB. Limited implementation; full Agent logic
// is in [Server]Agent/AgentNetworkMsgParser.cpp (50K+ LOC).

#include "mxh/server/server.hpp"

#include <cstring>
#include <iostream>

namespace mxh::server {

namespace {

// Payload format for CHARACTERLIST_ACK (simplified):
//   [u8: count] [for each char: u16 name_len + name bytes + u16 level + ...]
// We just send count + 1 dummy char for the demo.
mxh::net::Message make_char_list_ack() {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListAck);
    m.payload.resize(5);
    m.payload[0] = 1;  // count = 1
    // dummy character: level = 1
    m.payload[1] = 1; m.payload[2] = 0;
    return m;
}

}  // namespace

AgentHandler::AgentHandler(mxh::db::IDbAdapter& db, ReplyFn reply)
    : db_(db), reply_(std::move(reply)) {}

bool AgentHandler::on_connect(mxh::net::ConnectionId /*id*/,
                              const std::string& remote_addr) {
    std::cout << "[Agent] client connected from " << remote_addr << "\n";
    return true;
}

void AgentHandler::on_disconnect(mxh::net::ConnectionId id,
                                 mxh::net::NetError reason) {
    std::cout << "[Agent] client disconnected (id=" << id.value
              << " reason=" << mxh::net::to_string(reason) << ")\n";
}

void AgentHandler::on_message(mxh::net::ConnectionId id,
                              const mxh::net::Message& msg) {
    auto cat = static_cast<mxh::proto::Category>(msg.header.category);
    if (cat == mxh::proto::Category::UserConn) {
        handle_userconn(id, msg);
    } else {
        std::cout << "[Agent] unhandled category: "
                  << mxh::proto::category_name(cat)
                  << " proto=" << (int)msg.header.protocol << "\n";
    }
}

void AgentHandler::handle_userconn(mxh::net::ConnectionId id,
                                   const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::UserConnProtocol>(msg.header.protocol);
    if (proto == mxh::proto::UserConnProtocol::CharacterListSyn) {
        std::cout << "[Agent] CharacterListSyn → sending dummy list\n";
        reply_(id, make_char_list_ack());
    } else {
        std::cout << "[Agent] unhandled userconn proto="
                  << (int)msg.header.protocol << "\n";
    }
}

}  // namespace mxh::server