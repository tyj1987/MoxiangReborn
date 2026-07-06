// login_handler.cpp - Login (Distribute) server handler.
//
// Phase 4 demo: handles MP_USERCONN_REQUEST_LOGIN using IDbAdapter
// for credential check. Tells client which AgentServer to connect to.

#include "mxh/server/server.hpp"

#include <cstring>
#include <iostream>

namespace mxh::server {

namespace {

mxh::net::Message make_login_ack(std::uint16_t agent_port,
                                 const std::string& agent_addr) {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NotifyUserLoginAck);
    m.payload.resize(1 + 2 + agent_addr.size() + 1);
    m.payload[0] = 1;
    m.payload[1] = static_cast<std::uint8_t>(agent_port & 0xFF);
    m.payload[2] = static_cast<std::uint8_t>((agent_port >> 8) & 0xFF);
    std::memcpy(m.payload.data() + 3, agent_addr.c_str(), agent_addr.size());
    m.payload[3 + agent_addr.size()] = 0;
    return m;
}

mxh::net::Message make_login_nack() {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NotifyUserLoginNack);
    m.payload.resize(1);
    m.payload[0] = 0;
    return m;
}

}  // namespace

LoginHandler::LoginHandler(mxh::db::IDbAdapter& db,
                           std::string agent_addr,
                           std::uint16_t agent_port,
                           ReplyFn reply)
    : db_(db), agent_addr_(std::move(agent_addr)),
      agent_port_(agent_port), reply_(std::move(reply)) {}

bool LoginHandler::on_connect(mxh::net::ConnectionId /*id*/,
                              const std::string& remote_addr) {
    std::cout << "[Login] client connected from " << remote_addr << "\n";
    return true;
}

void LoginHandler::on_disconnect(mxh::net::ConnectionId id,
                                 mxh::net::NetError reason) {
    std::cout << "[Login] client disconnected (id=" << id.value
              << " reason=" << mxh::net::to_string(reason) << ")\n";
}

void LoginHandler::on_message(mxh::net::ConnectionId id,
                              const mxh::net::Message& msg) {
    auto cat = static_cast<mxh::proto::Category>(msg.header.category);
    if (cat == mxh::proto::Category::UserConn) {
        handle_userconn(id, msg);
    } else {
        std::cout << "[Login] unhandled category: "
                  << mxh::proto::category_name(cat)
                  << " proto=" << (int)msg.header.protocol << "\n";
    }
}

void LoginHandler::handle_userconn(mxh::net::ConnectionId id,
                                   const mxh::net::Message& msg) {
    auto proto = static_cast<mxh::proto::UserConnProtocol>(msg.header.protocol);
    if (proto != mxh::proto::UserConnProtocol::RequestLogin) {
        std::cout << "[Login] unexpected userconn proto="
                  << (int)msg.header.protocol << "\n";
        return;
    }

    if (msg.payload.size() < 4) {
        std::cout << "[Login] payload too short\n";
        return;
    }
    std::uint16_t id_len = static_cast<std::uint16_t>(msg.payload[0])
                         | (static_cast<std::uint16_t>(msg.payload[1]) << 8);
    if (msg.payload.size() < std::size_t(2 + id_len + 2)) return;
    std::string user_id(reinterpret_cast<const char*>(msg.payload.data() + 2),
                        id_len);
    std::uint16_t pw_len = static_cast<std::uint16_t>(msg.payload[2 + id_len])
                         | (static_cast<std::uint16_t>(msg.payload[3 + id_len]) << 8);
    if (msg.payload.size() < std::size_t(2 + id_len + 2 + pw_len)) return;
    std::string password(reinterpret_cast<const char*>(
                             msg.payload.data() + 2 + id_len + 2),
                         pw_len);

    std::cout << "[Login] RequestLogin id='" << user_id << "'\n";

    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> params{mxh::db::bind(user_id)};
    auto q = db_.query(
        "SELECT id, pw, userlevel FROM chr_log_info WHERE id = ?", params, rs);

    bool ok = false;
    if (q.ok() && !rs.empty()) {
        const auto& row = rs.rows[0];
        auto db_pw = std::get<std::string>(row[1]);
        ok = (db_pw == password);
    }

    if (ok) {
        std::cout << "[Login] auth OK for '" << user_id << "', sending ACK ("
                  << agent_addr_ << ":" << agent_port_ << ")\n";
        reply_(id, make_login_ack(agent_port_, agent_addr_));
    } else {
        std::cout << "[Login] auth FAIL for '" << user_id << "', sending NACK\n";
        reply_(id, make_login_nack());
    }
}

}  // namespace mxh::server