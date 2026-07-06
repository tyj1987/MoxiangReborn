// server.hpp - Server framework.
//
// Provides high-level handlers for the major game-server subsystems
// (Distribute, Agent, Map) using the net/crypto/proto/db stack.

#pragma once

#include "mxh/db/db_adapter.hpp"
#include "mxh/net/net.hpp"
#include "mxh/proto/protocol.hpp"

#include <functional>
#include <memory>
#include <string>

namespace mxh::server {

// Forward declare TcpServer so handlers can send replies.
namespace net_detail { class TcpServerBase; }

// Send-reply callback type. Used by handlers to reply to clients.
using ReplyFn = std::function<void(mxh::net::ConnectionId, const mxh::net::Message&)>;

// Forward declare TcpServer so handlers can use it in ReplyFn.
namespace net { class TcpServer; }

// LoginServer = phase 1 of client connection (Distribute).
// Validates user credentials and tells client which AgentServer to connect to.
class LoginHandler final : public mxh::net::IConnectionHandler {
public:
    LoginHandler(mxh::db::IDbAdapter& db,
                 std::string agent_addr,
                 std::uint16_t agent_port,
                 ReplyFn reply);
    ~LoginHandler() override = default;

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;

private:
    void handle_userconn(mxh::net::ConnectionId id,
                         const mxh::net::Message& msg);

    mxh::db::IDbAdapter& db_;
    std::string agent_addr_;
    std::uint16_t agent_port_;
    ReplyFn reply_;
};

// AgentHandler - serves the per-user agent connection.
// Returns character list and game-start entry points.
class AgentHandler final : public mxh::net::IConnectionHandler {
public:
    AgentHandler(mxh::db::IDbAdapter& db, ReplyFn reply);
    ~AgentHandler() override = default;

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;

private:
    void handle_userconn(mxh::net::ConnectionId id,
                         const mxh::net::Message& msg);

    mxh::db::IDbAdapter& db_;
    ReplyFn reply_;
};

}  // namespace mxh::server