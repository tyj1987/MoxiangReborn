#pragma once

#include "../client/AgentSession.hpp"
#include "../client/ClientWire.hpp"
#include "../client/NetworkEventQueue.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/net/net.hpp"
#include "mxh/unity/unity_client.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace mxh::unity {

class NativeClientCore final : public mxh::net::IConnectionHandler {
public:
    explicit NativeClientCore(std::size_t event_capacity = 256);
    ~NativeClientCore() noexcept override;

    NativeClientCore(const NativeClientCore&) = delete;
    NativeClientCore& operator=(const NativeClientCore&) = delete;

    std::uint32_t connect(std::string host, std::uint16_t port,
                          std::string user_id, std::string password,
                          std::uint32_t flags, std::uint32_t timeout_ms);
    std::uint32_t destroy() noexcept;
    std::uint32_t disconnect();
    std::uint32_t tick();
    std::uint32_t submit(const mxh_unity_command& command);
    std::uint32_t poll_event(mxh_unity_event& event);
    std::uint32_t copy_snapshot(mxh_unity_snapshot& snapshot) const;

    // TcpClient invokes these on its receive thread.  They enqueue protocol
    // data (plus transport-local HSEL setup); lifecycle and state transitions
    // are performed by the Unity owner thread through tick/connect/disconnect/
    // destroy. TcpClient::disconnect therefore never self-joins in this core.
    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& message) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id) override;

private:
    using Clock = std::chrono::steady_clock;

    void handle_login_event(const mxh::client::ClientRuntimeEvent& event);
    void handle_agent_event(const mxh::client::ClientRuntimeEvent& event);
    void handle_login_message(const mxh::net::Message& message);
    void handle_agent_message(const mxh::net::Message& message);
    void transition(std::uint32_t state);
    void fail(std::uint32_t result, std::string detail);
    void close_connections();
    bool shutdown_noexcept() noexcept;
    bool emit(std::uint32_t type, std::uint32_t result,
              std::uint64_t request_id = 0, std::uint32_t argument0 = 0,
              std::uint32_t argument1 = 0, const std::string& text = {},
              std::uint32_t wire_protocol = 0);
    static void clear_secret(std::string& value) noexcept;

    mutable std::recursive_mutex mutex_;
    bool destroyed_ = false;
    std::unique_ptr<mxh::net::TcpClient> login_client_;
    std::unique_ptr<mxh::crypto::HselStreamCipher> login_hsel_;
    mxh::client::NetworkEventQueue login_events_;
    mxh::client::AgentSession agent_;

    std::deque<mxh_unity_event> events_;
    const std::size_t event_capacity_;
    std::uint64_t dropped_events_ = 0;
    std::uint64_t public_dropped_seen_ = 0;
    std::uint64_t login_dropped_seen_ = 0;
    std::uint64_t agent_dropped_seen_ = 0;
    std::uint64_t next_event_sequence_ = 1;
    std::uint64_t revision_ = 0;
    std::uint64_t session_generation_ = 0;
    std::uint64_t map_generation_ = 0;
    std::uint32_t state_ = MXH_UNITY_STATE_IDLE;
    std::uint32_t last_result_ = MXH_UNITY_OK;
    std::string last_error_;

    std::string user_id_;
    std::string password_;
    bool use_hsel_ = false;
    std::uint32_t legacy_text_code_page_ = 0;
    bool login_hsel_received_ = false;
    std::uint32_t timeout_ms_ = 10000;
    std::uint32_t auth_key_ = 0;
    std::uint32_t user_index_ = 0;
    std::uint8_t user_level_ = 0;
    std::uint32_t selected_character_id_ = 0;
    std::uint16_t selected_map_ = 0;
    std::uint64_t pending_request_id_ = 0;
    std::string pending_create_name_;
    std::vector<std::uint32_t> pending_character_ids_;
    Clock::time_point deadline_{};
    std::vector<mxh::client::CharacterSlot> characters_;
    mxh::client::GameInInfo game_{};
};

}  // namespace mxh::unity
