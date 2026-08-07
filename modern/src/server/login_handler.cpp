// login_handler.cpp - Login (Distribute) server handler.
//
// Phase 4 demo: handles MP_USERCONN_REQUEST_LOGIN using IDbAdapter
// for credential check. Tells client which AgentServer to connect to.
// Phase 4.3: adds version negotiation before login.

#include "mxh/server/server.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

// Phase 7.6: File-based debug logging (capturable when running as background process).
static std::ofstream g_dbg_log;
static void dbg_log(const std::string& msg) {
    if (!g_dbg_log.is_open()) {
        g_dbg_log.open("d:/墨香全套源代码（源码+资源+客户端+服务端+教程）/modern/scratch/login_debug.log",
                      std::ios::app);
    }
    g_dbg_log << msg << std::endl;
}

namespace mxh::server {

namespace {

// Build a version ACK message.
// Payload: [server_version: u16] [encryption_required: u8]
mxh::net::Message make_version_ack(bool encryption_required) {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = mxh::proto::kModernNotifyVersionAck;
    m.payload.resize(3);
    m.payload[0] = static_cast<std::uint8_t>(mxh::proto::kProtocolVersion & 0xFF);
    m.payload[1] = static_cast<std::uint8_t>((mxh::proto::kProtocolVersion >> 8) & 0xFF);
    m.payload[2] = encryption_required ? 1 : 0;
    return m;
}

// Build a version NACK message.
// Payload: [server_version: u16] [reason: u8]
mxh::net::Message make_version_nack(mxh::proto::VersionRejectReason reason) {
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    m.header.protocol = mxh::proto::kModernNotifyVersionNack;
    m.payload.resize(3);
    m.payload[0] = static_cast<std::uint8_t>(mxh::proto::kProtocolVersion & 0xFF);
    m.payload[1] = static_cast<std::uint8_t>((mxh::proto::kProtocolVersion >> 8) & 0xFF);
    m.payload[2] = static_cast<std::uint8_t>(reason);
    return m;
}

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
                           ReplyFn reply,
                           bool use_legacy_framing,
                           bool use_hsel,
                           std::function<void(mxh::net::ConnectionId,
                                              const mxh::net::Message&)>
                               direct_send)
    : db_(db), agent_addr_(std::move(agent_addr)),
      agent_port_(agent_port), reply_(std::move(reply)),
      use_legacy_framing_(use_legacy_framing),
      use_hsel_(use_hsel),
      hsel_(use_hsel, std::move(direct_send)) {}

mxh::net::IEncryptor* LoginHandler::encryptor_for(
    mxh::net::ConnectionId id) {
    return hsel_.encryptor_for(id);
}

bool LoginHandler::on_connect(mxh::net::ConnectionId id,
                              const std::string& remote_addr) {
    std::cout << "[Login] client connected from " << remote_addr << "\n";
    dbg_log("[on_connect] id=" + std::to_string(id.value) + " remote=" + remote_addr
            + " legacy=" + (use_legacy_framing_ ? "yes" : "no"));

    // Phase 7.6: In legacy mode, immediately send MP_USERCONN_DIST_CONNECTSUCCESS
    // with a unique auth key in dwObjectID. The client stores this as m_DistAuthKey
    // and later passes it to the AgentServer as proof of Distribute authentication.
    if (use_legacy_framing_) {
        std::uint32_t auth_key;
        {
            std::lock_guard<std::mutex> lk(auth_mu_);
            auth_key = next_auth_key_++;
            auth_keys_[id.value] = auth_key;
        }
        if (use_hsel_) {
            // Phase R-1 handshake: deliver the resolved HselInit FIRST
            // in plaintext (cipher reset to pass-through), then re-arm
            // so every subsequent reply is HSEL-encrypted.
            hsel_.handshake(id, static_cast<std::uint8_t>(
                                    mxh::proto::Category::UserConn));
        }
        mxh::net::Message msg;
        msg.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        msg.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::DistConnectSuccess);
        msg.header.object_id = auth_key;  // Client reads this as m_DistAuthKey
        reply_(id, msg);
        std::cout << "[Login] legacy: sent DistConnectSuccess auth_key=" << auth_key << "\n";
        dbg_log("[on_connect] sent DistConnectSuccess auth_key=" + std::to_string(auth_key));
    }
    return true;
}

void LoginHandler::on_disconnect(mxh::net::ConnectionId id,
                                 mxh::net::NetError reason) {
    std::cout << "[Login] client disconnected (id=" << id.value
              << " reason=" << mxh::net::to_string(reason) << ")\n";
    // Clean up version state.
    std::lock_guard<std::mutex> lk(version_mu_);
    version_verified_.erase(id.value);
    hsel_.on_disconnect(id);
}

void LoginHandler::on_message(mxh::net::ConnectionId id,
                              const mxh::net::Message& msg) {
    dbg_log("[on_message] id=" + std::to_string(id.value) + " cat=" + std::to_string(msg.header.category)
            + " proto=" + std::to_string(msg.header.protocol) + " obj=" + std::to_string(msg.header.object_id)
            + " payload=" + std::to_string(msg.payload.size()));
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

    // Phase 7.6: Legacy 4DyuchiNET protocol support.
    // Original client sends MP_USERCONN_LOGIN_SYN (protocol=1) directly.
    // Modern protocol requires version check first.
    if (use_legacy_framing_) {
        dbg_log("[handle_userconn] legacy mode, proto=" + std::to_string((int)proto));
        // Legacy mode: skip version check, handle login directly.
        if (proto == mxh::proto::UserConnProtocol::RequestLogin) {
            handle_legacy_login(id, msg);
        } else {
            std::cout << "[Login] legacy: unexpected proto=" << (int)proto << "\n";
        }
        return;
    }

    // Modern mode: version check required.
    if (msg.header.protocol == mxh::proto::kModernCheckVersion) {
        handle_version_check(id, msg);
        return;
    }

    // All other messages require prior version verification.
    {
        std::lock_guard<std::mutex> lk(version_mu_);
        if (version_verified_.find(id.value) == version_verified_.end()) {
            std::cout << "[Login] rejecting unverified client (id=" << id.value
                      << " proto=" << (int)proto << ")\n";
            reply_(id, make_version_nack(mxh::proto::VersionRejectReason::TooOld));
            return;
        }
    }

    if (proto == mxh::proto::UserConnProtocol::RequestLogin) {
        handle_login(id, msg);
    } else {
        std::cout << "[Login] unexpected userconn proto="
                  << (int)msg.header.protocol << "\n";
    }
}

void LoginHandler::handle_version_check(mxh::net::ConnectionId id,
                                        const mxh::net::Message& msg) {
    // Parse client version from payload: [client_version: u16] [client_name: u8 len + bytes]
    std::uint16_t client_version = 0;
    if (msg.payload.size() >= 2) {
        client_version = static_cast<std::uint16_t>(msg.payload[0])
                       | (static_cast<std::uint16_t>(msg.payload[1]) << 8);
    }

    std::string client_name;
    if (msg.payload.size() >= 3) {
        std::uint8_t name_len = msg.payload[2];
        if (msg.payload.size() >= std::size_t(3 + name_len)) {
            client_name.assign(reinterpret_cast<const char*>(msg.payload.data() + 3),
                               name_len);
        }
    }

    std::cout << "[Login] CheckVersion from client v" << client_version;
    if (!client_name.empty()) std::cout << " ('" << client_name << "')";
    std::cout << ", server v" << mxh::proto::kProtocolVersion
              << " (min v" << mxh::proto::kMinProtocolVersion << ")\n";

    // Version compatibility check.
    if (client_version < mxh::proto::kMinProtocolVersion) {
        std::cout << "[Login] client too old (" << client_version
                  << " < " << mxh::proto::kMinProtocolVersion << ")\n";
        reply_(id, make_version_nack(mxh::proto::VersionRejectReason::TooOld));
        return;
    }
    if (client_version > mxh::proto::kProtocolVersion) {
        std::cout << "[Login] client too new (" << client_version
                  << " > " << mxh::proto::kProtocolVersion << ")\n";
        reply_(id, make_version_nack(mxh::proto::VersionRejectReason::TooNew));
        return;
    }

    // Version OK — mark as verified and send ACK.
    {
        std::lock_guard<std::mutex> lk(version_mu_);
        version_verified_.insert(id.value);
    }
    std::cout << "[Login] version OK, sending VersionAck\n";
    reply_(id, make_version_ack(/*encryption_required=*/false));
}

void LoginHandler::handle_login(mxh::net::ConnectionId id,
                                const mxh::net::Message& msg) {
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

void LoginHandler::handle_legacy_login(mxh::net::ConnectionId id,
                                       const mxh::net::Message& msg) {
    // Phase 7.6: Handle original 4DyuchiNET MSG_LOGIN_SYN format.
    // Original format: MSGBASE(8B) + AuthKey(4B) + id(17B) + pw(17B)
    // MsgHeader is already parsed, so payload contains:
    //   [AuthKey: u32] [id: char[17]] [pw: char[17]]
    
    // Minimum payload: AuthKey(4) + id(17) + pw(17) = 38 bytes
    if (msg.payload.size() < 38) {
        std::cout << "[Login] legacy: payload too short (" << msg.payload.size() << ")\n";
        return;
    }
    
    // Extract AuthKey (4 bytes, little-endian)
    std::uint32_t auth_key = 0;
    std::memcpy(&auth_key, msg.payload.data(), sizeof(auth_key));
    
    // Extract id (17 bytes, null-terminated)
    char id_buf[18] = {};
    std::memcpy(id_buf, msg.payload.data() + 4, 17);
    id_buf[17] = '\0';
    std::string user_id(id_buf);
    
    // Extract pw (17 bytes, null-terminated)
    char pw_buf[18] = {};
    std::memcpy(pw_buf, msg.payload.data() + 21, 17);
    pw_buf[17] = '\0';
    std::string password(pw_buf);
    
    std::cout << "[Login] legacy: auth_key=" << auth_key
              << " id='" << user_id << "'\n";
    dbg_log("[handle_legacy_login] auth_key=" + std::to_string(auth_key)
            + " id='" + user_id + "' payload_size=" + std::to_string(msg.payload.size()));
    
    // Query database
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> params{mxh::db::bind(user_id)};
    auto q = db_.query(
        "SELECT id, pw, userlevel FROM chr_log_info WHERE id = ?", params, rs);
    
    bool ok = false;
    std::uint8_t user_level = 0;
    if (q.ok() && !rs.empty()) {
        const auto& row = rs.rows[0];
        auto db_pw = std::get<std::string>(row[1]);
        ok = (db_pw == password);
        user_level = static_cast<std::uint8_t>(std::get<std::int64_t>(row[2]));
    }
    
    if (ok) {
        std::cout << "[Login] legacy: auth OK for '" << user_id
                  << "', sending ACK (" << agent_addr_ << ":" << agent_port_ << ")\n";
        // Build MSG_LOGIN_ACK: MSGBASE(8B) + agentip(16B) + agentport(2B) + userIdx(4B) + cbUserLevel(1B)
        mxh::net::Message reply_msg;
        reply_msg.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        reply_msg.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::NotifyUserLoginAck);  // Protocol=2 (ACK)
        reply_msg.header.object_id = 0;
        std::cout << "[Login] legacy: reply_msg.header total_size=" << reply_msg.total_size() << "\n";
        
        // Payload: agentip + agentport + userIdx + cbUserLevel
        reply_msg.payload.resize(16 + 2 + 4 + 1);
        std::memset(reply_msg.payload.data(), 0, reply_msg.payload.size());
        
        // agentip (16 bytes, null-terminated)
        std::memcpy(reply_msg.payload.data(), agent_addr_.c_str(),
                    std::min(agent_addr_.size(), std::size_t(15)));
        
        // agentport (2 bytes, little-endian)
        reply_msg.payload[16] = static_cast<std::uint8_t>(agent_port_ & 0xFF);
        reply_msg.payload[17] = static_cast<std::uint8_t>((agent_port_ >> 8) & 0xFF);
        
        // userIdx (4 bytes, little-endian) - placeholder
        reply_msg.payload[18] = 1;
        
        // cbUserLevel (1 byte)
        reply_msg.payload[22] = user_level;
        
        std::cout << "[Login] legacy: calling reply_ with payload_size=" << reply_msg.payload.size() << "\n";
        dbg_log("[handle_legacy_login] sending ACK payload=" + std::to_string(reply_msg.payload.size())
                + " total=" + std::to_string(reply_msg.total_size()));
        reply_(id, reply_msg);
        dbg_log("[handle_legacy_login] reply_ returned OK");
    } else {
        std::cout << "[Login] legacy: auth FAIL for '" << user_id << "'\n";
        // Send NACK with same header, empty payload
        mxh::net::Message nack_msg;
        nack_msg.header = msg.header;
        reply_(id, nack_msg);
    }
}

}  // namespace mxh::server
