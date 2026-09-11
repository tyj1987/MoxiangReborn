#include "mxh/unity/unity_client.h"
#include "client/NetworkEventQueue.hpp"
#include "unity_core/NativeClientCore.hpp"
#include "client/CInGameState.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/game/hero_total_layout.hpp"
#include "mxh/net/net.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/server/hsel_session.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <latch>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <winsock2.h>

namespace {
using namespace std::chrono_literals;

void put_u16(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void put_u32(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint32_t value) {
    for (std::size_t i = 0; i < 4; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
}

void put_u64(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint64_t value) {
    put_u32(bytes, offset, static_cast<std::uint32_t>(value));
    put_u32(bytes, offset + 4, static_cast<std::uint32_t>(value >> 32));
}

std::uint16_t get_u16(const std::vector<std::uint8_t>& bytes,
                      std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t get_u32(const std::vector<std::uint8_t>& bytes,
                      std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(bytes[offset + i]) << (i * 8);
    return value;
}

std::uint16_t find_free_port() {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 0;
    const SOCKET socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle == INVALID_SOCKET) return 0;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (bind(socket_handle, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) != 0) {
        closesocket(socket_handle);
        return 0;
    }
    int length = sizeof(address);
    if (getsockname(socket_handle, reinterpret_cast<sockaddr*>(&address),
                    &length) != 0) {
        closesocket(socket_handle);
        return 0;
    }
    const auto port = ntohs(address.sin_port);
    closesocket(socket_handle);
    return port;
}

class ProtocolServer final : public mxh::net::IConnectionHandler {
public:
    enum class Role { Login, Agent };
    enum class CreateReply { Success, Reject, MismatchedList };

    ProtocolServer(Role role, bool hsel, std::uint16_t agent_port,
                   std::string wire_name, std::uint32_t game_user_id = 42,
                   std::uint16_t game_map = 10,
                   CreateReply create_reply = CreateReply::Success,
                   std::string create_wire_name = {},
                   bool initially_empty = false)
        : role_(role), hsel_(hsel, [this](mxh::net::ConnectionId id,
                                         const mxh::net::Message& message) {
              if (server_) (void)server_->send(id, message);
          }), agent_port_(agent_port), wire_name_(std::move(wire_name)),
          game_user_id_(game_user_id), game_map_(game_map),
          create_reply_(create_reply),
          create_wire_name_(std::move(create_wire_name)),
          initially_empty_(initially_empty) {}

    void attach(mxh::net::TcpServer* server) { server_ = server; }

    bool on_connect(mxh::net::ConnectionId id, const std::string&) override {
        if (!hsel_.handshake(id, static_cast<std::uint8_t>(
                                  mxh::proto::Category::UserConn)))
            return false;
        mxh::net::Message message{};
        message.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        if (role_ == Role::Login) {
            message.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::DistConnectSuccess);
            message.header.object_id = 0x12345678u;
        } else {
            message.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::AgentConnectSuccess);
        }
        return server_ && server_->send(id, message) == mxh::net::NetError::Ok;
    }

    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& message) override {
        if (role_ == Role::Login) handle_login(id, message);
        else handle_agent(id, message);
    }

    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError) override {
        hsel_.on_disconnect(id);
    }

    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id) override {
        return hsel_.encryptor_for(id);
    }

    std::atomic<std::uint32_t> valid_create_packets{0};
    std::atomic<std::uint32_t> valid_move_packets{0};

private:
    void send(mxh::net::ConnectionId id, std::uint8_t protocol,
              std::vector<std::uint8_t> payload = {},
              std::uint32_t object_id = 0) {
        mxh::net::Message reply{};
        reply.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        reply.header.protocol = protocol;
        reply.header.object_id = object_id;
        reply.payload = std::move(payload);
        if (server_) (void)server_->send(id, reply);
    }

    void handle_login(mxh::net::ConnectionId id,
                      const mxh::net::Message& message) {
        if (message.header.protocol != static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::RequestLogin) ||
            message.payload.size() != 38 ||
            message.payload[0] != 0x78 || message.payload[1] != 0x56 ||
            message.payload[2] != 0x34 || message.payload[3] != 0x12 ||
            std::memcmp(message.payload.data() + 4, "test", 4) != 0 ||
            std::memcmp(message.payload.data() + 21, "secret", 6) != 0) {
            send(id, static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::NotifyUserLoginNack));
            return;
        }
        std::vector<std::uint8_t> ack(23, 0);
        constexpr char host[] = "127.0.0.1";
        std::memcpy(ack.data(), host, sizeof(host) - 1);
        put_u16(ack, 16, agent_port_);
        put_u32(ack, 18, 42);
        ack[22] = 2;
        send(id, static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::NotifyUserLoginAck), std::move(ack));
    }

    void handle_agent(mxh::net::ConnectionId id,
                      const mxh::net::Message& message) {
        if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Move)) {
            if (message.header.object_id != 7001 || message.payload.size() != 4 ||
                (message.header.protocol != 13 && message.header.protocol != 8)) return;
            ++valid_move_packets;
            auto reply = message;
            // A normal own echo must not overwrite prediction, while remote
            // reports are delivered with their object identity and wire mode.
            reply.header.object_id = 8002;
            (void)server_->send(id, reply);
            reply.header.object_id = 7001;
            put_u16(reply.payload, 0, 9);
            (void)server_->send(id, reply);
            reply.header.protocol = 2;
            put_u16(reply.payload, 0, 120);
            put_u16(reply.payload, 2, 240);
            if (get_u16(message.payload, 0) == 65535) reply.payload.push_back(0);
            (void)server_->send(id, reply);
            return;
        }
        using mxh::proto::UserConnProtocol;
        const auto protocol = static_cast<UserConnProtocol>(message.header.protocol);
        if (protocol == UserConnProtocol::CharacterListSyn) {
            if (message.header.object_id != 42 || message.payload.size() != 8) {
                send(id, static_cast<std::uint8_t>(UserConnProtocol::CharacterListNack));
                return;
            }
            send_character_list(id, false);
        } else if (protocol == UserConnProtocol::CharacterMakeSyn) {
            bool valid = message.header.object_id == 42 &&
                         message.payload.size() == 59 &&
                         !create_wire_name_.empty() &&
                         create_wire_name_.size() <= 16 &&
                         std::memcmp(message.payload.data(),
                                     create_wire_name_.data(),
                                     create_wire_name_.size()) == 0 &&
                         message.payload[create_wire_name_.size()] == 0 &&
                         get_u32(message.payload, 17) == 42 &&
                         message.payload[21] == 1 &&
                         message.payload[22] == 0 &&
                         message.payload[23] == 4 &&
                         message.payload[24] == 3 &&
                         message.payload[25] == 17 &&
                         get_u32(message.payload, 26) == 0 &&
                         get_u16(message.payload, 32) == 21000 &&
                         get_u16(message.payload, 34) == 23010 &&
                         get_u16(message.payload, 36) == 27010 &&
                         message.payload[50] == 0xff &&
                         get_u32(message.payload, 51) == 0x3f800000u &&
                         get_u32(message.payload, 55) == 0x3f800000u;
            for (std::size_t slot = 0; slot < 10; ++slot) {
                if (slot != 1 && slot != 2 && slot != 3 &&
                    get_u16(message.payload, 30 + slot * 2) != 0)
                    valid = false;
            }
            if (!valid) {
                send(id, static_cast<std::uint8_t>(
                    UserConnProtocol::CharacterMakeNack));
                return;
            }
            valid_create_packets.fetch_add(1, std::memory_order_release);
            if (create_reply_ == CreateReply::Reject) {
                send(id, static_cast<std::uint8_t>(
                    UserConnProtocol::CharacterMakeNack));
            } else if (create_reply_ == CreateReply::MismatchedList) {
                send_character_list(id, false);
            } else {
                send_character_list(id, true);
            }
        } else if (protocol == UserConnProtocol::CharacterSelectSyn) {
            if (message.header.object_id != 7001 || message.payload.size() != 2) {
                send(id, static_cast<std::uint8_t>(UserConnProtocol::CharacterSelectNack));
                return;
            }
            send(id, static_cast<std::uint8_t>(UserConnProtocol::CharacterSelectAck),
                 {10}, 7001);
        } else if (protocol == UserConnProtocol::GameInSyn) {
            if (message.header.object_id != 7001 || !message.payload.empty()) {
                send(id, static_cast<std::uint8_t>(UserConnProtocol::GameInNack));
                return;
            }
            std::vector<std::uint8_t> ack(
                mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
            put_u32(ack, 0, 7001);
            put_u32(ack, 4, game_user_id_);
            std::memcpy(ack.data() + 8, wire_name_.data(),
                        std::min<std::size_t>(wire_name_.size(), 16));
            put_u32(ack, 35, 70001);
            put_u32(ack, 39, 80002);
            put_u16(ack, 75, 17);
            put_u16(ack, 77, game_map_);
            const auto hero = mxh::game::HERO_TOTAL_HERO_OFFSET;
            put_u16(ack, hero, 11);
            put_u16(ack, hero + 2, 12);
            put_u16(ack, hero + 4, 13);
            put_u16(ack, hero + 6, 14);
            put_u32(ack, hero + 8, 222);
            put_u32(ack, hero + 12, 333);
            put_u64(ack, hero + 22, 0x000000020000115Cull);
            put_u32(ack, hero + 36, 5555);
            put_u16(ack, mxh::game::HERO_TOTAL_MOVE_OFFSET, 101);
            put_u16(ack, mxh::game::HERO_TOTAL_MOVE_OFFSET + 2, 202);
            const auto time = mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET;
            put_u16(ack, time, 2026);
            put_u16(ack, time + 2, 9);
            put_u16(ack, time + 6, 11);
            put_u16(ack, time + 8, 20);
            send(id, static_cast<std::uint8_t>(UserConnProtocol::GameInAck),
                 std::move(ack), 7001);
        }
    }

    void send_character_list(mxh::net::ConnectionId id, bool with_created) {
        using mxh::proto::UserConnProtocol;
        std::vector<std::uint8_t> ack(889, 0);
        const auto initial_count = initially_empty_ ? 0u : 1u;
        put_u32(ack, 0, initial_count + (with_created ? 1u : 0u));
        if (!initially_empty_) {
            put_u32(ack, 14, 7001);
            put_u32(ack, 18, 42);
            std::memcpy(ack.data() + 22, wire_name_.data(),
                        std::min<std::size_t>(wire_name_.size(), 16));
            ack[189 + 16] = 1;
            ack[189 + 17] = 2;
            ack[189 + 18] = 3;
            put_u16(ack, 189 + 40, 17);
            put_u16(ack, 189 + 42, 10);
        }
        if (with_created) {
            const std::size_t slot = initially_empty_ ? 0 : 1;
            put_u32(ack, 14 + slot * 35, 7002);
            put_u32(ack, 18 + slot * 35, 42);
            std::memcpy(ack.data() + 22 + slot * 35, create_wire_name_.data(),
                        create_wire_name_.size());
            const auto total = 189 + slot * 140;
            ack[total + 16] = 1;
            ack[total + 17] = 3;
            ack[total + 18] = 4;
            put_u16(ack, total + 19 + 2, 21000);
            put_u16(ack, total + 19 + 4, 23010);
            put_u16(ack, total + 19 + 6, 27010);
            put_u16(ack, total + 40, 1);
            put_u16(ack, total + 42, 10);
        }
        send(id, static_cast<std::uint8_t>(UserConnProtocol::CharacterListAck),
             std::move(ack));
    }

    Role role_;
    mxh::server::HselSessionManager hsel_;
    std::uint16_t agent_port_;
    std::string wire_name_;
    std::uint32_t game_user_id_;
    std::uint16_t game_map_;
    CreateReply create_reply_;
    std::string create_wire_name_;
    bool initially_empty_;
    mxh::net::TcpServer* server_ = nullptr;
};

class ProtocolPair {
public:
    ProtocolPair(bool hsel, std::string wire_name,
                 std::uint32_t game_user_id = 42,
                 std::uint16_t game_map = 10,
                 ProtocolServer::CreateReply create_reply =
                     ProtocolServer::CreateReply::Success,
                 std::string create_wire_name = {},
                 bool initially_empty = false)
        : login_port(find_free_port()), agent_port(find_free_port()),
          login(ProtocolServer::Role::Login, hsel, agent_port, wire_name),
          agent(ProtocolServer::Role::Agent, hsel, agent_port,
                std::move(wire_name), game_user_id, game_map, create_reply,
                std::move(create_wire_name), initially_empty),
          login_server(login), agent_server(agent) {
        login.attach(&login_server);
        agent.attach(&agent_server);
    }

    bool start(bool hsel) {
        if (login_port == 0 || agent_port == 0 || login_port == agent_port) return false;
        mxh::net::ServerConfig config;
        config.bind_address = "127.0.0.1";
        config.use_legacy_framing = true;
        config.use_encryption = hsel;
        config.port = agent_port;
        if (agent_server.start(config) != mxh::net::NetError::Ok) return false;
        config.port = login_port;
        if (login_server.start(config) != mxh::net::NetError::Ok) {
            agent_server.stop();
            return false;
        }
        return true;
    }

    ~ProtocolPair() {
        login_server.stop();
        agent_server.stop();
    }

    std::uint16_t login_port;
    std::uint16_t agent_port;
    ProtocolServer login;
    ProtocolServer agent;
    mxh::net::TcpServer login_server;
    mxh::net::TcpServer agent_server;
};

class MalformedHselServer final : public mxh::net::IConnectionHandler {
public:
    void attach(mxh::net::TcpServer* server) { server_ = server; }
    bool on_connect(mxh::net::ConnectionId id, const std::string&) override {
        mxh::net::Message key{};
        key.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        key.header.protocol = mxh::proto::kModernHselKey;
        key.payload.resize(sizeof(mxh::crypto::HselInit) - 1, 0x5a);
        if (!server_ || server_->send(id, key) != mxh::net::NetError::Ok)
            return false;
        mxh::net::Message connected{};
        connected.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        connected.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::DistConnectSuccess);
        connected.header.object_id = 77;
        return server_->send(id, connected) == mxh::net::NetError::Ok;
    }
    void on_message(mxh::net::ConnectionId, const mxh::net::Message&) override {}
    void on_disconnect(mxh::net::ConnectionId, mxh::net::NetError) override {
        disconnected.store(true, std::memory_order_release);
    }
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId) override {
        return &cipher_;
    }
    std::atomic<bool> disconnected{false};
private:
    mxh::net::TcpServer* server_ = nullptr;
    mxh::crypto::HselStreamCipher cipher_;
};

class SilentServer final : public mxh::net::IConnectionHandler {
public:
    bool on_connect(mxh::net::ConnectionId, const std::string&) override {
        connected.store(true, std::memory_order_release);
        return true;
    }
    void on_message(mxh::net::ConnectionId, const mxh::net::Message&) override {}
    std::atomic<bool> connected{false};
};

mxh_unity_connect_args make_connect(std::uint16_t port, std::uint32_t flags) {
    mxh_unity_connect_args args{};
    args.struct_size = sizeof(args);
    args.flags = flags;
    args.timeout_ms = 4000;
    args.login_port = port;
    constexpr char host[] = "127.0.0.1";
    constexpr char user[] = "test";
    constexpr char password[] = "secret";
    args.host_length = sizeof(host) - 1;
    args.user_id_length = sizeof(user) - 1;
    args.password_length = sizeof(password) - 1;
    std::memcpy(args.login_host, host, sizeof(host) - 1);
    std::memcpy(args.user_id, user, sizeof(user) - 1);
    std::memcpy(args.password, password, sizeof(password) - 1);
    return args;
}

bool wait_for_state(mxh_unity_handle handle, std::uint32_t expected,
                    mxh_unity_snapshot& snapshot) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)mxh_unity_tick(handle);
        std::uint32_t required = 0;
        if (mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                    &required) != MXH_UNITY_OK)
            return false;
        mxh_unity_event event{};
        while (mxh_unity_poll_event(handle, &event, sizeof(event),
                                    &required) == MXH_UNITY_OK) {}
        if (snapshot.state == expected) return true;
        if (snapshot.state == MXH_UNITY_STATE_FAILED) return false;
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

mxh_unity_command make_create_command(const mxh_unity_snapshot& snapshot,
                                      std::string name,
                                      std::uint64_t request_id = 501) {
    mxh_unity_command command{};
    command.struct_size = sizeof(command);
    command.type = MXH_UNITY_COMMAND_CREATE_CHARACTER;
    command.payload_size = MXH_UNITY_CREATE_COMMAND_PAYLOAD_SIZE;
    command.request_id = request_id;
    command.expected_session_generation = snapshot.session_generation;
    command.expected_map_generation = snapshot.map_generation;
    command.name_length = static_cast<std::uint32_t>(name.size());
    if (name.size() <= sizeof(command.name))
        std::memcpy(command.name, name.data(), name.size());
    command.sex_type = 1;
    command.hair_type = 4;
    command.face_type = 3;
    command.cloth_option = 1;
    command.boot_option = 1;
    command.weapon_option = 5;
    return command;
}

bool wait_for_create_event(mxh_unity_handle handle, mxh_unity_event& found,
                           mxh_unity_snapshot& snapshot) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)mxh_unity_tick(handle);
        std::uint32_t required = 0;
        if (mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                    &required) != MXH_UNITY_OK)
            return false;
        mxh_unity_event event{};
        while (mxh_unity_poll_event(handle, &event, sizeof(event),
                                    &required) == MXH_UNITY_OK) {
            if (event.type == MXH_UNITY_EVENT_CHARACTER_CREATE) {
                found = event;
                return true;
            }
        }
        if (snapshot.state == MXH_UNITY_STATE_FAILED) return false;
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

void run_protocol_round_trip(bool hsel, std::uint32_t text_flag,
                             const std::string& wire_name,
                             const std::string& expected_utf8) {
    ProtocolPair servers(hsel, wire_name);
    ASSERT_TRUE(servers.start(hsel));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port,
        (hsel ? MXH_UNITY_CONNECT_USE_HSEL : 0u) | text_flag);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot)) << snapshot.error;
    ASSERT_EQ(snapshot.character_count, 1);
    EXPECT_EQ(snapshot.characters[0].character_id, 7001u);
    EXPECT_EQ(std::string(snapshot.characters[0].name,
                          snapshot.characters[0].name_length), expected_utf8);
    const auto session_generation = snapshot.session_generation;
    mxh_unity_command command{};
    command.struct_size = sizeof(command);
    command.type = MXH_UNITY_COMMAND_SELECT_CHARACTER;
    command.argument0 = 7001;
    command.request_id = 99;
    command.expected_session_generation = session_generation - 1;
    command.expected_map_generation = snapshot.map_generation;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_WRONG_STATE);
    command.expected_session_generation = session_generation;
    command.expected_map_generation = snapshot.map_generation + 1;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_WRONG_STATE);
    command.expected_map_generation = snapshot.map_generation;
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_IN_GAME, snapshot))
        << snapshot.error;
    EXPECT_EQ(snapshot.session_generation, session_generation);
    EXPECT_EQ(snapshot.map_generation, 1u);
    EXPECT_EQ(snapshot.game.player_id, 7001u);
    EXPECT_EQ(snapshot.game.user_id, 42u);
    EXPECT_EQ(snapshot.game.map_number, 10);
    EXPECT_EQ(snapshot.game.level, 17);
    EXPECT_EQ(snapshot.game.life, 70001u);
    EXPECT_EQ(snapshot.game.max_life, 80002u);
    EXPECT_EQ(snapshot.game.mp, 222u);
    EXPECT_EQ(snapshot.game.experience, 0x000000020000115Cull);
    EXPECT_EQ(std::string(snapshot.game.name, snapshot.game.name_length),
              expected_utf8);
    EXPECT_EQ(mxh_unity_disconnect(handle), MXH_UNITY_OK);
    std::uint32_t required = 0;
    ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                      &required), MXH_UNITY_OK);
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_IDLE);
    EXPECT_EQ(snapshot.user_id, 0u);
    EXPECT_EQ(snapshot.character_count, 0u);
    EXPECT_EQ(snapshot.selected_character_id, 0u);
    EXPECT_EQ(snapshot.game.player_id, 0u);
    EXPECT_EQ(mxh_unity_disconnect(handle), MXH_UNITY_OK);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

void run_gamein_identity_mismatch(std::uint32_t game_user_id,
                                  std::uint16_t game_map) {
    ProtocolPair servers(false, "MismatchHero", game_user_id, game_map);
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port, 0);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot));
    mxh_unity_command command{};
    command.struct_size = sizeof(command);
    command.type = MXH_UNITY_COMMAND_SELECT_CHARACTER;
    command.argument0 = 7001;
    command.expected_session_generation = snapshot.session_generation;
    command.expected_map_generation = snapshot.map_generation;
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_FAILED, snapshot));
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_PROTOCOL_ERROR);
    EXPECT_EQ(snapshot.map_generation, 0u);
    EXPECT_EQ(snapshot.game.player_id, 0u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}
}  // namespace

TEST(UnityCoreWire, MonsterLifePayloadUsesLittleEndianLifeAndShield) {
    const std::array<std::uint8_t, 8> wire{0x78, 0x56, 0x34, 0x12,
                                            0xef, 0xcd, 0xab, 0x90};
    const auto parsed = mxh::client::parse_monster_life_payload(wire);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->first, 0x12345678u);
    EXPECT_EQ(parsed->second, 0x90abcdefu);
}

TEST(UnityCoreWire, MonsterLifePayloadRejectsTruncation) {
    const std::array<std::uint8_t, 7> wire{};
    EXPECT_FALSE(mxh::client::parse_monster_life_payload(wire).has_value());
}

TEST(UnityCoreWire, PickupMessagePinsLegacyItemWire) {
    const auto message = mxh::client::make_pickup_message(77u, 0x12345678u);
    EXPECT_EQ(message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Item));
    EXPECT_EQ(message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupSyn));
    EXPECT_EQ(message.header.object_id, 77u);
    ASSERT_EQ(message.payload.size(), 4u);
    EXPECT_EQ(message.payload[0], 0x78);
    EXPECT_EQ(message.payload[1], 0x56);
    EXPECT_EQ(message.payload[2], 0x34);
    EXPECT_EQ(message.payload[3], 0x12);
}
TEST(UnityCoreWire, PickupAckPayloadRejectsInvalidFields) {
    const std::array<std::uint8_t, 8> wire{41, 0, 0, 0, 9, 0, 3, 0};
    const auto ack = mxh::client::parse_pickup_ack_payload(wire);
    ASSERT_TRUE(ack.has_value());
    EXPECT_EQ(ack->drop_id, 41u); EXPECT_EQ(ack->item_id, 9u); EXPECT_EQ(ack->count, 3u);
    auto invalid = wire; invalid[6] = invalid[7] = 0;
    EXPECT_FALSE(mxh::client::parse_pickup_ack_payload(invalid).has_value());
}
TEST(UnityCoreWire, QuestMessagePinsLegacyQuestWire) {
    const auto message = mxh::client::make_quest_message(
        88u, mxh::proto::QuestProtocol::StartSyn, 0x3456u);
    EXPECT_EQ(message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Quest));
    EXPECT_EQ(message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn));
    EXPECT_EQ(message.header.object_id, 88u);
    ASSERT_EQ(message.payload.size(), 2u);
    EXPECT_EQ(message.payload[0], 0x56);
    EXPECT_EQ(message.payload[1], 0x34);
}

TEST(UnityCoreAbi, VersionAndHandleLifecycle) {
    EXPECT_EQ(mxh_unity_get_api_version(), MXH_UNITY_API_VERSION);
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    ASSERT_NE(handle, 0u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(mxh_unity_tick(handle), MXH_UNITY_INVALID_HANDLE);
}

TEST(UnityCoreAbi, ReportsRequiredBufferSizesWithoutConsuming) {
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    std::uint32_t required = 0;
    EXPECT_EQ(mxh_unity_poll_event(handle, nullptr, 0, &required),
              MXH_UNITY_BUFFER_TOO_SMALL);
    EXPECT_EQ(required, sizeof(mxh_unity_event));
    mxh_unity_event event{};
    EXPECT_EQ(mxh_unity_poll_event(handle, &event, sizeof(event), &required),
              MXH_UNITY_NOT_READY);
    EXPECT_EQ(required, sizeof(event));
    EXPECT_EQ(mxh_unity_copy_snapshot(handle, nullptr, 0, &required),
              MXH_UNITY_BUFFER_TOO_SMALL);
    EXPECT_EQ(required, sizeof(mxh_unity_snapshot));
    mxh_unity_snapshot snapshot{};
    EXPECT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot), &required),
              MXH_UNITY_OK);
    EXPECT_EQ(required, sizeof(snapshot));
    EXPECT_EQ(snapshot.api_version, MXH_UNITY_API_VERSION);
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_IDLE);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreAbi, UnsupportedCommandDoesNotSendPacket) {
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    mxh_unity_command command{};
    command.struct_size = sizeof(command);
    command.type = 999;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_UNSUPPORTED);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreAbi, SkillCommandRejectsIdleAndMalformedPayload) {
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    std::uint32_t required = 0;
    ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot), &required), MXH_UNITY_OK);
    mxh_unity_command head{};
    head.struct_size = sizeof(head);
    head.type = MXH_UNITY_COMMAND_SKILL;
    head.argument0 = 1;
    head.argument1 = 2;
    head.expected_session_generation = snapshot.session_generation;
    head.expected_map_generation = snapshot.map_generation;
    std::uint8_t payload[MXH_UNITY_SKILL_PAYLOAD_SIZE]{};
    mxh_unity_extended_command command{head, payload, MXH_UNITY_SKILL_PAYLOAD_SIZE};
    EXPECT_EQ(mxh_unity_submit_extended_command(handle, &command), MXH_UNITY_WRONG_STATE);
    command.payload_size = 4;
    EXPECT_EQ(mxh_unity_submit_extended_command(handle, &command), MXH_UNITY_WRONG_STATE);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, PlaintextRealSocketLoginThroughGameIn) {
    run_protocol_round_trip(false, 0, "墨香", "墨香");
}

TEST(UnityCoreNetwork, MovementPredictionCorrectionAndStaleGeneration) {
    ProtocolPair servers(true, "Mover");
    ASSERT_TRUE(servers.start(true));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    struct Owner { mxh_unity_handle h; ~Owner() { mxh_unity_destroy(h); } } owner{handle};
    auto args = make_connect(servers.login_port, MXH_UNITY_CONNECT_USE_HSEL);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY, snapshot));
    mxh_unity_command command{};
    command.struct_size = sizeof(command);
    command.expected_session_generation = snapshot.session_generation;
    command.expected_map_generation = snapshot.map_generation;
    command.type = MXH_UNITY_COMMAND_MOVE;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_WRONG_STATE);
    command.type = MXH_UNITY_COMMAND_SELECT_CHARACTER;
    command.argument0 = 7001;
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_IN_GAME, snapshot));
    command.type = MXH_UNITY_COMMAND_MOVE;
    command.argument0 = 1000; command.argument1 = 2000;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_WRONG_STATE);
    command.expected_map_generation = snapshot.map_generation;
    command.argument0 = 65536;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    command.argument0 = 1000; command.payload_size = 4;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    command.payload_size = 0;
    EXPECT_EQ(servers.agent.valid_move_packets.load(), 0u);
    for (const auto type : {MXH_UNITY_COMMAND_MOVE, MXH_UNITY_COMMAND_STOP}) {
        command.type = type; command.request_id = 77;
        ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
        std::uint32_t required = 0;
        ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot), &required), MXH_UNITY_OK);
        EXPECT_EQ(snapshot.game.position_x, 1000);
        EXPECT_EQ(snapshot.game.position_z, 2000);
        bool submitted = false, remote = false, corrected = false;
        std::uint64_t sequence = 0;
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (!corrected && std::chrono::steady_clock::now() < deadline) {
            mxh_unity_tick(handle);
            mxh_unity_event event{};
            while (mxh_unity_poll_event(handle, &event, sizeof(event), &required) == MXH_UNITY_OK) {
                EXPECT_GT(event.sequence, sequence); sequence = event.sequence;
                EXPECT_EQ(event.map_generation, snapshot.map_generation);
                if (event.type == MXH_UNITY_EVENT_MOVEMENT_SUBMITTED) {
                    submitted = true; EXPECT_EQ(event.request_id, 77u);
                    EXPECT_EQ(event.argument1, 1000u | (2000u << 16));
                    EXPECT_EQ(event.reserved0, type == MXH_UNITY_COMMAND_MOVE ? 13u : 8u);
                }
                if (event.type == MXH_UNITY_EVENT_OBJECT_MOVEMENT) {
                    remote = true; EXPECT_EQ(event.argument0, 8002u);
                    EXPECT_EQ(event.argument1, 1000u | (2000u << 16));
                }
                if (event.type == MXH_UNITY_EVENT_POSITION_CORRECTION) {
                    corrected = true; EXPECT_EQ(event.request_id, 0u);
                    EXPECT_EQ(event.argument1, 120u | (240u << 16));
                }
            }
            std::this_thread::sleep_for(5ms);
        }
        EXPECT_TRUE(submitted); EXPECT_TRUE(remote); ASSERT_TRUE(corrected);
        ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot), &required), MXH_UNITY_OK);
        EXPECT_EQ(snapshot.game.position_x, 120); EXPECT_EQ(snapshot.game.position_z, 240);
    }
    command.argument0 = 65535;
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_FAILED, snapshot));
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_PROTOCOL_ERROR);
    EXPECT_EQ(servers.agent.valid_move_packets.load(), 3u);
}

TEST(UnityCoreNetwork, HselRealSocketLoginThroughGameIn) {
    run_protocol_round_trip(true, 0, "HselHero", "HselHero");
}

TEST(UnityCoreNetwork, ConvertsCp936NamesToUtf8) {
    const std::string gbk_name{"\xC4\xAB\xCF\xE3", 4};
    run_protocol_round_trip(false, MXH_UNITY_CONNECT_LEGACY_TEXT_CP936,
                            gbk_name, "墨香");
}

TEST(UnityCoreNetwork, CreatesCp936CharacterFromSemanticOptionsAndRefreshesList) {
    const std::string first_wire_name{"\xC4\xAB\xCF\xE3", 4};
    const std::string created_wire_name{"\xD0\xC2\xCF\xC0", 4};
    const std::string created_utf8 = "新侠";
    ProtocolPair servers(false, first_wire_name, 42, 10,
        ProtocolServer::CreateReply::Success, created_wire_name);
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port,
                             MXH_UNITY_CONNECT_LEGACY_TEXT_CP936);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot));
    auto command = make_create_command(snapshot, created_utf8, 0x1234);
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    mxh_unity_event event{};
    ASSERT_TRUE(wait_for_create_event(handle, event, snapshot)) << snapshot.error;
    EXPECT_EQ(event.result, MXH_UNITY_OK);
    EXPECT_EQ(event.request_id, 0x1234u);
    EXPECT_EQ(event.argument0, 7002u);
    EXPECT_EQ(event.argument1, 2u);
    EXPECT_EQ(std::string(event.text, event.text_length), created_utf8);
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_CHARACTER_LIST_READY);
    ASSERT_EQ(snapshot.character_count, 2u);
    EXPECT_EQ(snapshot.characters[1].character_id, 7002u);
    EXPECT_EQ(std::string(snapshot.characters[1].name,
                          snapshot.characters[1].name_length), created_utf8);
    EXPECT_EQ(snapshot.characters[1].worn_item_index[1], 21000u);
    EXPECT_EQ(snapshot.characters[1].worn_item_index[2], 23010u);
    EXPECT_EQ(snapshot.characters[1].worn_item_index[3], 27010u);
    EXPECT_EQ(servers.agent.valid_create_packets.load(std::memory_order_acquire),
              1u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, EmptyAccountCreatesItsFirstCharacterOverRealSocket) {
    ProtocolPair servers(false, "UnusedHero", 42, 10,
        ProtocolServer::CreateReply::Success, "FirstHero", true);
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port, 0);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot));
    ASSERT_EQ(snapshot.character_count, 0u);
    auto command = make_create_command(snapshot, "FirstHero", 0x4321);
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    mxh_unity_event event{};
    ASSERT_TRUE(wait_for_create_event(handle, event, snapshot)) << snapshot.error;
    EXPECT_EQ(event.result, MXH_UNITY_OK);
    EXPECT_EQ(event.request_id, 0x4321u);
    EXPECT_EQ(event.argument0, 7002u);
    EXPECT_EQ(event.argument1, 1u);
    ASSERT_EQ(snapshot.character_count, 1u);
    EXPECT_EQ(snapshot.characters[0].character_id, 7002u);
    EXPECT_EQ(std::string(snapshot.characters[0].name,
                          snapshot.characters[0].name_length), "FirstHero");
    EXPECT_EQ(servers.agent.valid_create_packets.load(std::memory_order_acquire),
              1u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, CharacterMakeNackIsNonTerminalRejectedEvent) {
    ProtocolPair servers(false, "FirstHero", 42, 10,
        ProtocolServer::CreateReply::Reject, "TakenHero");
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port, 0);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot));
    auto command = make_create_command(snapshot, "TakenHero", 0x5678);
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    mxh_unity_event event{};
    ASSERT_TRUE(wait_for_create_event(handle, event, snapshot));
    EXPECT_EQ(event.result, MXH_UNITY_REJECTED);
    EXPECT_EQ(event.request_id, 0x5678u);
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_CHARACTER_LIST_READY);
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_REJECTED);
    EXPECT_EQ(snapshot.character_count, 1u);
    EXPECT_EQ(servers.agent.valid_create_packets.load(std::memory_order_acquire),
              1u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, CreateRefreshMustContainTheNewRequestedIdentity) {
    ProtocolPair servers(false, "FirstHero", 42, 10,
        ProtocolServer::CreateReply::MismatchedList, "SecondHero");
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port, 0);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot));
    auto command = make_create_command(snapshot, "SecondHero", 777);
    ASSERT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_OK);
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_FAILED, snapshot));
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_PROTOCOL_ERROR);
    EXPECT_EQ(snapshot.character_count, 1u);
    EXPECT_EQ(servers.agent.valid_create_packets.load(std::memory_order_acquire),
              1u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, RejectsStaleInvalidOrUnrepresentableCreateLocally) {
    const std::string first_wire_name{"\xC4\xAB\xCF\xE3", 4};
    const std::string expected_wire_name{"\xD0\xC2\xCF\xC0", 4};
    ProtocolPair servers(false, first_wire_name, 42, 10,
        ProtocolServer::CreateReply::Success, expected_wire_name);
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port,
                             MXH_UNITY_CONNECT_LEGACY_TEXT_CP936);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    ASSERT_TRUE(wait_for_state(handle, MXH_UNITY_STATE_CHARACTER_LIST_READY,
                               snapshot));

    auto command = make_create_command(snapshot, "新侠");
    command.expected_session_generation--;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_WRONG_STATE);
    command = make_create_command(snapshot, std::string{"\xFF" "abc", 4});
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    command = make_create_command(snapshot, std::string{"Ab\nc", 4});
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    command = make_create_command(snapshot, "😀");
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    command = make_create_command(snapshot, "新新新新新新新新新");
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    command = make_create_command(snapshot, "新侠");
    command.weapon_option = 6;
    EXPECT_EQ(mxh_unity_submit_command(handle, &command), MXH_UNITY_INVALID_ARGUMENT);
    EXPECT_EQ(servers.agent.valid_create_packets.load(std::memory_order_acquire),
              0u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, GameInRejectsMismatchedUserWithoutAdvancingGeneration) {
    run_gamein_identity_mismatch(99, 10);
}

TEST(UnityCoreNetwork, GameInRejectsMismatchedMapWithoutAdvancingGeneration) {
    run_gamein_identity_mismatch(42, 11);
}

TEST(UnityCoreNetwork, GameInRejectsZeroMapWithoutAdvancingGeneration) {
    run_gamein_identity_mismatch(42, 0);
}

TEST(UnityCoreNetwork, InvalidUtf8NameFailsInsteadOfLeakingLegacyBytes) {
    ProtocolPair servers(false, std::string{"\xFF", 1});
    ASSERT_TRUE(servers.start(false));
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(servers.login_port, 0);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    std::uint32_t required = 0;
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)mxh_unity_tick(handle);
        ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                          &required), MXH_UNITY_OK);
        if (snapshot.state == MXH_UNITY_STATE_FAILED) break;
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_FAILED);
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_PROTOCOL_ERROR);
    EXPECT_EQ(snapshot.character_count, 0u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreNetwork, MalformedHselKeyFailsAndClosesConnection) {
    const auto port = find_free_port();
    ASSERT_NE(port, 0);
    MalformedHselServer handler;
    mxh::net::TcpServer server(handler);
    handler.attach(&server);
    mxh::net::ServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = port;
    config.use_legacy_framing = true;
    config.use_encryption = true;
    ASSERT_EQ(server.start(config), mxh::net::NetError::Ok);
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(port, MXH_UNITY_CONNECT_USE_HSEL);
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)mxh_unity_tick(handle);
        std::uint32_t required = 0;
        ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                          &required), MXH_UNITY_OK);
        if (snapshot.state == MXH_UNITY_STATE_FAILED) break;
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_FAILED);
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_PROTOCOL_ERROR);
    EXPECT_NE(std::string(snapshot.error, snapshot.error_length).find("HSEL"),
              std::string::npos);
    const auto disconnect_deadline = std::chrono::steady_clock::now() + 1s;
    while (!handler.disconnected.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < disconnect_deadline)
        std::this_thread::sleep_for(5ms);
    EXPECT_TRUE(handler.disconnected.load(std::memory_order_acquire));
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
    server.stop();
}

TEST(UnityCoreNetwork, TcpConnectWithoutProtocolAckNeverReportsReady) {
    const auto port = find_free_port();
    ASSERT_NE(port, 0);
    SilentServer handler;
    mxh::net::TcpServer server(handler);
    mxh::net::ServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = port;
    config.use_legacy_framing = true;
    ASSERT_EQ(server.start(config), mxh::net::NetError::Ok);
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(port, 0);
    args.timeout_ms = 1000;
    ASSERT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    std::uint32_t required = 0;
    ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                      &required), MXH_UNITY_OK);
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_LOGIN_CONNECTING);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)mxh_unity_tick(handle);
        ASSERT_EQ(mxh_unity_copy_snapshot(handle, &snapshot, sizeof(snapshot),
                                          &required), MXH_UNITY_OK);
        if (snapshot.state == MXH_UNITY_STATE_FAILED) break;
        std::this_thread::sleep_for(10ms);
    }
    EXPECT_TRUE(handler.connected.load(std::memory_order_acquire));
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_FAILED);
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_NETWORK_ERROR);
    EXPECT_EQ(snapshot.character_count, 0u);
    EXPECT_EQ(snapshot.game.player_id, 0u);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
    server.stop();
}

TEST(NetworkEventQueue, OverflowIsCountedAndDoesNotReplaceOldEvent) {
    mxh::client::NetworkEventQueue queue(1);
    mxh::client::ClientRuntimeEvent first;
    first.kind = mxh::client::ClientRuntimeEventKind::Connected;
    mxh::client::ClientRuntimeEvent second;
    second.kind = mxh::client::ClientRuntimeEventKind::Disconnected;
    EXPECT_TRUE(queue.push(std::move(first)));
    EXPECT_FALSE(queue.push(std::move(second)));
    EXPECT_EQ(queue.dropped_count(), 1u);
    const auto events = queue.drain();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events.front().kind, mxh::client::ClientRuntimeEventKind::Connected);
}

TEST(ClientWire, CharacterListRequiresCompleteBoundedLegacyPayload) {
    std::array<std::uint8_t, 889> wire{};
    std::int32_t count = -1;
    std::memcpy(wire.data(), &count, sizeof(count));
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(wire).has_value());
    count = 6;
    std::memcpy(wire.data(), &count, sizeof(count));
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(wire).has_value());
    count = 1;
    std::memcpy(wire.data(), &count, sizeof(count));
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(wire.data(), 49)).has_value());
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(wire.data(), 888)).has_value());
    const std::uint32_t first_id = 7001;
    std::memcpy(wire.data() + 14, &first_id, sizeof(first_id));
    EXPECT_TRUE(mxh::client::parse_legacy_character_list_ack(wire).has_value());

    std::array<std::uint8_t, 890> trailing_byte{};
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(trailing_byte).has_value());
    std::array<std::uint8_t, 1017> cryptcheck_shaped{};
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(cryptcheck_shaped).has_value());

    const std::uint32_t tail_id = 88;
    std::memcpy(wire.data() + 14 + 35, &tail_id, sizeof(tail_id));
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(wire).has_value());
    count = 2;
    std::memcpy(wire.data(), &count, sizeof(count));
    std::memcpy(wire.data() + 14 + 35, &first_id, sizeof(first_id));
    EXPECT_FALSE(mxh::client::parse_legacy_character_list_ack(wire).has_value());
    const std::uint32_t second_id = 7002;
    std::memcpy(wire.data() + 14 + 35, &second_id, sizeof(second_id));
    EXPECT_TRUE(mxh::client::parse_legacy_character_list_ack(wire).has_value());
}

TEST(ClientWire, CharacterMakeUsesLockedChinaSemanticOptions) {
    auto params = mxh::client::legacy_china_character_make_params(
        "NewHero", 1, 4, 3, 1, 1, 5);
    ASSERT_TRUE(params.has_value());
    const auto wire = mxh::client::legacy_character_make_syn_payload(*params, 42);
    ASSERT_TRUE(wire.has_value());
    ASSERT_EQ(wire->size(), 59u);
    EXPECT_EQ(std::memcmp(wire->data(), "NewHero", 7), 0);
    EXPECT_EQ((*wire)[7], 0u);
    EXPECT_EQ(get_u32(*wire, 17), 42u);
    EXPECT_EQ((*wire)[21], 1u);
    EXPECT_EQ((*wire)[22], 0u);
    EXPECT_EQ((*wire)[23], 4u);
    EXPECT_EQ((*wire)[24], 3u);
    EXPECT_EQ((*wire)[25], 17u);
    EXPECT_EQ(get_u32(*wire, 26), 0u);
    EXPECT_EQ(get_u16(*wire, 32), 21000u);
    EXPECT_EQ(get_u16(*wire, 34), 23010u);
    EXPECT_EQ(get_u16(*wire, 36), 27010u);
    EXPECT_EQ((*wire)[50], 0xffu);
    EXPECT_EQ(get_u32(*wire, 51), 0x3f800000u);
    EXPECT_EQ(get_u32(*wire, 55), 0x3f800000u);
    EXPECT_FALSE(mxh::client::legacy_china_character_make_params(
        "abc", 1, 4, 3, 1, 1, 5).has_value());
    mxh::client::LegacyCharacterMakeParams too_long;
    too_long.wire_name = "12345678901234567";
    EXPECT_FALSE(mxh::client::legacy_character_make_syn_payload(
        too_long, 42).has_value());
    EXPECT_FALSE(mxh::client::legacy_china_character_make_params(
        "NewHero", 2, 4, 3, 1, 1, 5).has_value());
}

TEST(NativeClientCore, PublicEventOverflowIsFailClosed) {
    ProtocolPair servers(false, "OverflowHero");
    ASSERT_TRUE(servers.start(false));
    mxh::unity::NativeClientCore core(1);
    ASSERT_EQ(core.connect("127.0.0.1", servers.login_port,
                           "test", "secret", 0, 1000), MXH_UNITY_OK);
    mxh_unity_snapshot snapshot{};
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)core.tick();
        core.copy_snapshot(snapshot);
        if (snapshot.state == MXH_UNITY_STATE_FAILED) break;
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_FAILED);
    EXPECT_GE(snapshot.dropped_event_count, 1u);
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_INTERNAL_ERROR);
    mxh_unity_event event{};
    ASSERT_EQ(core.poll_event(event), MXH_UNITY_OK);
    EXPECT_EQ(event.type, MXH_UNITY_EVENT_ERROR);
    EXPECT_EQ(event.result, MXH_UNITY_INTERNAL_ERROR);
}

TEST(NativeClientCore, TerminalEventReplacesExactlyOneSlotAtCapacityThree) {
    ProtocolPair servers(false, "CapacityThreeHero");
    ASSERT_TRUE(servers.start(false));
    mxh::unity::NativeClientCore core(3);
    ASSERT_EQ(core.connect("127.0.0.1", servers.login_port,
                           "test", "secret", 0, 1000), MXH_UNITY_OK);
    mxh_unity_snapshot before{};
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        (void)core.tick();
        ASSERT_EQ(core.copy_snapshot(before), MXH_UNITY_OK);
        if (before.state == MXH_UNITY_STATE_FAILED) break;
        std::this_thread::sleep_for(5ms);
    }
    ASSERT_EQ(before.state, MXH_UNITY_STATE_FAILED);
    ASSERT_EQ(core.disconnect(), MXH_UNITY_OK);
    mxh_unity_snapshot after{};
    ASSERT_EQ(core.copy_snapshot(after), MXH_UNITY_OK);
    EXPECT_EQ(after.dropped_event_count, before.dropped_event_count + 2);
    std::vector<mxh_unity_event> events;
    mxh_unity_event event{};
    while (core.poll_event(event) == MXH_UNITY_OK) events.push_back(event);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events.back().type, MXH_UNITY_EVENT_DISCONNECTED);
    EXPECT_EQ(events.back().state, MXH_UNITY_STATE_IDLE);
}

TEST(NativeClientCore, NewConnectClearsFullOldGenerationEventQueue) {
    const auto port = find_free_port();
    ASSERT_NE(port, 0);
    SilentServer handler;
    mxh::net::TcpServer server(handler);
    mxh::net::ServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = port;
    config.use_legacy_framing = true;
    ASSERT_EQ(server.start(config), mxh::net::NetError::Ok);

    mxh::unity::NativeClientCore core(1);
    ASSERT_EQ(core.connect("127.0.0.1", port, "test", "secret", 0, 1000),
              MXH_UNITY_OK);
    ASSERT_EQ(core.disconnect(), MXH_UNITY_OK); // leaves one terminal old event
    mxh_unity_snapshot snapshot{};
    ASSERT_EQ(core.copy_snapshot(snapshot), MXH_UNITY_OK);
    ASSERT_GE(snapshot.dropped_event_count, 1u); // old generation queue was full
    ASSERT_EQ(core.connect("127.0.0.1", port, "test", "secret", 0, 1000),
              MXH_UNITY_OK);
    mxh_unity_event event{};
    ASSERT_EQ(core.poll_event(event), MXH_UNITY_OK);
    EXPECT_EQ(event.type, MXH_UNITY_EVENT_STATE_CHANGED);
    EXPECT_EQ(event.state, MXH_UNITY_STATE_LOGIN_CONNECTING);
    EXPECT_EQ(event.session_generation, 2u);
    EXPECT_EQ(core.tick(), MXH_UNITY_OK);
    ASSERT_EQ(core.copy_snapshot(snapshot), MXH_UNITY_OK);
    EXPECT_EQ(snapshot.state, MXH_UNITY_STATE_LOGIN_CONNECTING);
    EXPECT_EQ(snapshot.last_result, MXH_UNITY_OK);
    EXPECT_EQ(core.disconnect(), MXH_UNITY_OK);
    EXPECT_EQ(core.destroy(), MXH_UNITY_OK);
    server.stop();
}

TEST(NativeClientCore, AcquiredReferenceCannotExecuteAfterDestroy) {
    auto core = std::make_shared<mxh::unity::NativeClientCore>();
    auto acquired_before_destroy = core;
    std::latch acquired{1};
    std::latch execute{1};
    std::atomic<std::uint32_t> tick_result{MXH_UNITY_OK};
    std::thread caller([held = std::move(acquired_before_destroy), &acquired,
                        &execute, &tick_result] {
        acquired.count_down();
        execute.wait();
        tick_result.store(held->tick(), std::memory_order_release);
    });
    acquired.wait();
    const auto destroy_result = core->destroy();
    execute.count_down();
    caller.join();
    ASSERT_EQ(destroy_result, MXH_UNITY_OK);
    EXPECT_EQ(tick_result.load(std::memory_order_acquire), MXH_UNITY_INVALID_HANDLE);

    mxh_unity_snapshot snapshot{};
    mxh_unity_event event{};
    mxh_unity_command command{};
    command.struct_size = sizeof(command);
    EXPECT_EQ(core->copy_snapshot(snapshot), MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(core->poll_event(event), MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(core->submit(command), MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(core->disconnect(), MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(core->connect("127.0.0.1", 1, "test", "secret", 0, 1000),
              MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(core->destroy(), MXH_UNITY_INVALID_HANDLE);
}

TEST(NativeClientCore, OwnerThreadDestroyJoinsLiveReceiveThread) {
    const auto port = find_free_port();
    ASSERT_NE(port, 0);
    SilentServer handler;
    mxh::net::TcpServer server(handler);
    mxh::net::ServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = port;
    config.use_legacy_framing = true;
    ASSERT_EQ(server.start(config), mxh::net::NetError::Ok);
    mxh::unity::NativeClientCore core;
    ASSERT_EQ(core.connect("127.0.0.1", port, "test", "secret", 0, 1000),
              MXH_UNITY_OK);
    const auto deadline = std::chrono::steady_clock::now() + 1s;
    while (!handler.connected.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
    ASSERT_TRUE(handler.connected.load(std::memory_order_acquire));
    EXPECT_EQ(core.destroy(), MXH_UNITY_OK);
    EXPECT_EQ(core.tick(), MXH_UNITY_INVALID_HANDLE);
    server.stop();
}

TEST(UnityCoreAbi, RejectsInvalidStructuresAndTextFlags) {
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    auto args = make_connect(12345, MXH_UNITY_CONNECT_LEGACY_TEXT_CP936 |
                                    MXH_UNITY_CONNECT_LEGACY_TEXT_CP949);
    EXPECT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_INVALID_ARGUMENT);
    args.flags = 0;
    args.struct_size = sizeof(args) - 1;
    EXPECT_EQ(mxh_unity_connect(handle, &args), MXH_UNITY_INVALID_ARGUMENT);
    std::uint32_t required = 0;
    EXPECT_EQ(mxh_unity_copy_snapshot(0, nullptr, 0, &required),
              MXH_UNITY_INVALID_HANDLE);
    EXPECT_EQ(required, sizeof(mxh_unity_snapshot));
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
}

TEST(UnityCoreAbi, ConcurrentTickSnapshotAndDestroyAreSafe) {
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    std::atomic<bool> stop{false};
    std::thread worker([&] {
        while (!stop.load(std::memory_order_acquire)) {
            const auto result = mxh_unity_tick(handle);
            EXPECT_TRUE(result == MXH_UNITY_OK || result == MXH_UNITY_INVALID_HANDLE);
            mxh_unity_snapshot snapshot{};
            std::uint32_t required = 0;
            const auto copied = mxh_unity_copy_snapshot(
                handle, &snapshot, sizeof(snapshot), &required);
            EXPECT_TRUE(copied == MXH_UNITY_OK || copied == MXH_UNITY_INVALID_HANDLE);
        }
    });
    std::this_thread::sleep_for(20ms);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
    stop.store(true, std::memory_order_release);
    worker.join();
}
