#include "NativeClientCore.hpp"
#include "../client/CInGameState.hpp"

#include "mxh/proto/protocol.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <span>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace mxh::unity {
namespace {
constexpr std::uint8_t kUserConn =
    static_cast<std::uint8_t>(mxh::proto::Category::UserConn);

void copy_text(char* target, std::size_t capacity, std::uint32_t& length,
               const std::string& source) {
    const auto count = std::min(source.size(), capacity == 0 ? 0 : capacity - 1);
    if (count != 0) std::memcpy(target, source.data(), count);
    if (capacity != 0) target[count] = '\0';
    length = static_cast<std::uint32_t>(count);
}

bool valid_utf8(const std::string& text) noexcept {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t count = 0;
        std::uint32_t value = 0;
        if (lead <= 0x7f) { ++i; continue; }
        if ((lead & 0xe0) == 0xc0) { count = 2; value = lead & 0x1f; }
        else if ((lead & 0xf0) == 0xe0) { count = 3; value = lead & 0x0f; }
        else if ((lead & 0xf8) == 0xf0) { count = 4; value = lead & 0x07; }
        else return false;
        if (i + count > text.size()) return false;
        for (std::size_t j = 1; j < count; ++j) {
            const auto byte = static_cast<unsigned char>(text[i + j]);
            if ((byte & 0xc0) != 0x80) return false;
            value = (value << 6) | (byte & 0x3f);
        }
        if ((count == 2 && value < 0x80) || (count == 3 && value < 0x800) ||
            (count == 4 && value < 0x10000) || value > 0x10ffff ||
            (value >= 0xd800 && value <= 0xdfff)) return false;
        i += count;
    }
    return true;
}

bool valid_utf8_name_text(const std::string& text) noexcept {
    if (!valid_utf8(text) || text.empty()) return false;
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t count = 1;
        std::uint32_t value = lead;
        if ((lead & 0xe0) == 0xc0) { count = 2; value = lead & 0x1f; }
        else if ((lead & 0xf0) == 0xe0) { count = 3; value = lead & 0x0f; }
        else if ((lead & 0xf8) == 0xf0) { count = 4; value = lead & 0x07; }
        for (std::size_t j = 1; j < count; ++j)
            value = (value << 6) |
                    (static_cast<unsigned char>(text[i + j]) & 0x3f);
        if (value < 0x20 || (value >= 0x7f && value <= 0x9f)) return false;
        i += count;
    }
    return true;
}

std::optional<std::string> name_from_utf8(const std::string& source,
                                          std::uint32_t code_page) {
    if (!valid_utf8_name_text(source)) return std::nullopt;
    if (code_page == 0) return source;
#if defined(_WIN32)
    const int wide_count = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, source.data(),
        static_cast<int>(source.size()), nullptr, 0);
    if (wide_count <= 0) return std::nullopt;
    std::wstring wide(static_cast<std::size_t>(wide_count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source.data(),
                            static_cast<int>(source.size()), wide.data(),
                            wide_count) != wide_count) return std::nullopt;
    BOOL used_default = FALSE;
    const int byte_count = WideCharToMultiByte(
        code_page, WC_NO_BEST_FIT_CHARS, wide.data(), wide_count,
        nullptr, 0, nullptr, &used_default);
    if (byte_count <= 0 || used_default) return std::nullopt;
    std::string encoded(static_cast<std::size_t>(byte_count), '\0');
    used_default = FALSE;
    if (WideCharToMultiByte(code_page, WC_NO_BEST_FIT_CHARS, wide.data(),
                            wide_count, encoded.data(), byte_count,
                            nullptr, &used_default) != byte_count || used_default)
        return std::nullopt;
    return encoded;
#else
    return std::nullopt;
#endif
}

std::optional<std::string> name_to_utf8(const std::string& source,
                                        std::uint32_t code_page) {
    if (code_page == 0) {
        if (!valid_utf8(source)) return std::nullopt;
        return source;
    }
#if defined(_WIN32)
    if (source.empty()) return std::string{};
    const int wide_count = MultiByteToWideChar(
        code_page, MB_ERR_INVALID_CHARS, source.data(),
        static_cast<int>(source.size()), nullptr, 0);
    if (wide_count <= 0) return std::nullopt;
    std::wstring wide(static_cast<std::size_t>(wide_count), L'\0');
    if (MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, source.data(),
                            static_cast<int>(source.size()), wide.data(),
                            wide_count) != wide_count) return std::nullopt;
    const int utf8_count = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_count,
        nullptr, 0, nullptr, nullptr);
    if (utf8_count <= 0) return std::nullopt;
    std::string utf8(static_cast<std::size_t>(utf8_count), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
                            wide_count, utf8.data(), utf8_count,
                            nullptr, nullptr) != utf8_count) return std::nullopt;
    return utf8;
#else
    return std::nullopt;
#endif
}

void clear_bytes(std::vector<std::uint8_t>& value) noexcept {
    volatile std::uint8_t* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; bytes && i < value.size(); ++i) bytes[i] = 0;
    value.clear();
}
}

NativeClientCore::NativeClientCore(std::size_t event_capacity)
    : event_capacity_(event_capacity == 0 ? 1 : event_capacity) {}

NativeClientCore::~NativeClientCore() noexcept {
    try {
        std::lock_guard lock(mutex_);
        destroyed_ = true;
        (void)shutdown_noexcept();
    } catch (...) {}
}

void NativeClientCore::clear_secret(std::string& value) noexcept {
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; bytes && i < value.size(); ++i) bytes[i] = '\0';
    value.clear();
}

std::uint32_t NativeClientCore::connect(
    std::string host, std::uint16_t port, std::string user_id,
    std::string password, std::uint32_t flags, std::uint32_t timeout_ms) {
    std::lock_guard lock(mutex_);
    if (destroyed_) {
        clear_secret(password);
        return MXH_UNITY_INVALID_HANDLE;
    }
    if (state_ != MXH_UNITY_STATE_IDLE) {
        clear_secret(password);
        return MXH_UNITY_WRONG_STATE;
    }
    if (host.empty() || port == 0 || user_id.empty() || password.empty()) {
        clear_secret(password);
        return MXH_UNITY_INVALID_ARGUMENT;
    }

    // A successful new-session attempt starts with an empty public queue so
    // stale events cannot consume capacity or starve the new generation.
    events_.clear();
    public_dropped_seen_ = dropped_events_;
    ++session_generation_;
    map_generation_ = 0;
    revision_++;
    last_result_ = MXH_UNITY_OK;
    last_error_.clear();
    auth_key_ = 0;
    user_index_ = 0;
    user_level_ = 0;
    selected_character_id_ = 0;
    selected_map_ = 0;
    pending_request_id_ = 0;
    pending_create_name_.clear();
    pending_character_ids_.clear();
    characters_.clear();
    game_ = {};
    login_events_.clear();
    agent_.events().clear();
    login_dropped_seen_ = login_events_.dropped_count();
    agent_dropped_seen_ = agent_.events().dropped_count();
    use_hsel_ = (flags & MXH_UNITY_CONNECT_USE_HSEL) != 0;
    legacy_text_code_page_ = (flags & MXH_UNITY_CONNECT_LEGACY_TEXT_CP949) != 0
        ? 949u : ((flags & MXH_UNITY_CONNECT_LEGACY_TEXT_CP936) != 0 ? 936u : 0u);
    login_hsel_received_ = false;
    timeout_ms_ = std::clamp(timeout_ms, 1000u, 60000u);
    user_id_ = std::move(user_id);
    password_ = std::move(password);
    if (use_hsel_) login_hsel_ = std::make_unique<mxh::crypto::HselStreamCipher>();

    login_client_ = std::make_unique<mxh::net::TcpClient>(*this);
    mxh::net::ClientConfig config;
    config.remote_address = std::move(host);
    config.port = port;
    config.connect_timeout = std::chrono::milliseconds(timeout_ms_);
    config.use_encryption = use_hsel_;
    config.use_legacy_framing = true;
    transition(MXH_UNITY_STATE_LOGIN_CONNECTING);
    const auto result = login_client_->connect(config);
    if (result != mxh::net::NetError::Ok) {
        fail(MXH_UNITY_NETWORK_ERROR,
             std::string("login connect failed: ") + mxh::net::to_string(result));
        return MXH_UNITY_NETWORK_ERROR;
    }
    deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
    return MXH_UNITY_OK;
}

std::uint32_t NativeClientCore::destroy() noexcept {
    try {
        std::lock_guard lock(mutex_);
        if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
        // This flag is permanent and is set while holding the same lock used
        // by every public operation.  A caller that acquired a shared_ptr
        // before registry removal but executes afterward is therefore rejected.
        destroyed_ = true;
        return shutdown_noexcept() ? MXH_UNITY_OK : MXH_UNITY_INTERNAL_ERROR;
    } catch (...) {
        return MXH_UNITY_INTERNAL_ERROR;
    }
}

std::uint32_t NativeClientCore::disconnect() {
    std::lock_guard lock(mutex_);
    if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
    if (state_ == MXH_UNITY_STATE_IDLE) return MXH_UNITY_OK;
    const bool was_in_game = state_ == MXH_UNITY_STATE_IN_GAME;
    transition(MXH_UNITY_STATE_SHUTTING_DOWN);
    if (was_in_game && agent_.is_connected() && selected_character_id_ != 0)
        (void)agent_.send(mxh::client::make_legacy_gameout_syn_message(
            selected_character_id_));
    close_connections();
    clear_secret(password_);
    user_id_.clear();
    auth_key_ = 0;
    user_index_ = 0;
    user_level_ = 0;
    selected_character_id_ = 0;
    selected_map_ = 0;
    pending_request_id_ = 0;
    pending_create_name_.clear();
    pending_character_ids_.clear();
    characters_.clear();
    game_ = {};
    deadline_ = {};
    state_ = MXH_UNITY_STATE_IDLE;
    ++revision_;
    (void)emit(MXH_UNITY_EVENT_DISCONNECTED, MXH_UNITY_OK);
    return MXH_UNITY_OK;
}

void NativeClientCore::close_connections() {
    if (login_client_) {
        if (login_client_->is_connected()) login_client_->disconnect();
        login_client_.reset();
    }
    login_hsel_.reset();
    agent_.disconnect();
}

bool NativeClientCore::shutdown_noexcept() noexcept {
    bool clean = true;
    const bool was_in_game = state_ == MXH_UNITY_STATE_IN_GAME;
    state_ = MXH_UNITY_STATE_SHUTTING_DOWN;
    if (was_in_game && selected_character_id_ != 0) {
        try {
            if (agent_.is_connected())
                (void)agent_.send(mxh::client::make_legacy_gameout_syn_message(
                    selected_character_id_));
        } catch (...) { clean = false; }
    }
    try {
        if (login_client_ && login_client_->is_connected())
            login_client_->disconnect();
    } catch (...) { clean = false; }
    login_client_.reset();
    login_hsel_.reset();
    try { agent_.disconnect(); }
    catch (...) { clean = false; }
    clear_secret(password_);
    user_id_.clear();
    auth_key_ = 0;
    user_index_ = 0;
    user_level_ = 0;
    selected_character_id_ = 0;
    selected_map_ = 0;
    pending_request_id_ = 0;
    pending_create_name_.clear();
    pending_character_ids_.clear();
    characters_.clear();
    game_ = {};
    deadline_ = {};
    state_ = MXH_UNITY_STATE_IDLE;
    return clean;
}

std::uint32_t NativeClientCore::tick() {
    std::lock_guard lock(mutex_);
    if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
    for (const auto& event : login_events_.drain()) handle_login_event(event);
    for (const auto& event : agent_.events().drain()) handle_agent_event(event);
    if (login_events_.dropped_count() != login_dropped_seen_ ||
        agent_.events().dropped_count() != agent_dropped_seen_) {
        fail(MXH_UNITY_INTERNAL_ERROR, "network event queue overflow");
    }
    if (dropped_events_ != public_dropped_seen_ && state_ != MXH_UNITY_STATE_FAILED &&
        state_ != MXH_UNITY_STATE_IDLE) {
        public_dropped_seen_ = dropped_events_;
        fail(MXH_UNITY_INTERNAL_ERROR, "public event queue overflow");
    }
    if (deadline_ != Clock::time_point{} && Clock::now() >= deadline_ &&
        state_ != MXH_UNITY_STATE_IDLE && state_ != MXH_UNITY_STATE_FAILED &&
        state_ != MXH_UNITY_STATE_CHARACTER_LIST_READY &&
        state_ != MXH_UNITY_STATE_IN_GAME) {
        fail(MXH_UNITY_NETWORK_ERROR, "protocol acknowledgement timeout");
    }
    return last_result_;
}

void NativeClientCore::handle_login_event(
    const mxh::client::ClientRuntimeEvent& event) {
    if (event.kind == mxh::client::ClientRuntimeEventKind::Message) {
        handle_login_message(event.message);
    } else if (event.kind == mxh::client::ClientRuntimeEventKind::Error) {
        fail(MXH_UNITY_PROTOCOL_ERROR, event.detail);
    } else if (event.kind == mxh::client::ClientRuntimeEventKind::Disconnected &&
               state_ != MXH_UNITY_STATE_AGENT_CONNECTING &&
               state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_LIST &&
               state_ != MXH_UNITY_STATE_CHARACTER_LIST_READY &&
               state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_CREATE &&
               state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_SELECT &&
               state_ != MXH_UNITY_STATE_AWAIT_GAME_IN &&
               state_ != MXH_UNITY_STATE_IN_GAME &&
               state_ != MXH_UNITY_STATE_FAILED) {
        fail(MXH_UNITY_NETWORK_ERROR, "login disconnected before acknowledgement");
    }
}

void NativeClientCore::handle_login_message(const mxh::net::Message& message) {
    using mxh::proto::UserConnProtocol;
    if (message.header.category != kUserConn) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected login message category");
        return;
    }
    const auto protocol = static_cast<UserConnProtocol>(message.header.protocol);
    if (protocol == UserConnProtocol::DistConnectSuccess) {
        if (state_ != MXH_UNITY_STATE_LOGIN_CONNECTING ||
            message.header.object_id == 0 || !message.payload.empty()) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected or invalid DistConnectSuccess");
            return;
        }
        if (use_hsel_ && !login_hsel_received_) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "DistConnectSuccess arrived before HSEL key");
            return;
        }
        auth_key_ = message.header.object_id;
        mxh::net::Message request{};
        request.header.category = kUserConn;
        request.header.protocol = static_cast<std::uint8_t>(UserConnProtocol::RequestLogin);
        request.payload = mxh::client::legacy_request_login_payload(
            auth_key_, user_id_, password_);
        const auto sent = login_client_ ? login_client_->send(request)
                                        : mxh::net::NetError::Disconnected;
        clear_secret(password_);
        clear_bytes(request.payload);
        if (sent != mxh::net::NetError::Ok) {
            fail(MXH_UNITY_NETWORK_ERROR,
                 std::string("RequestLogin send failed: ") + mxh::net::to_string(sent));
            return;
        }
        transition(MXH_UNITY_STATE_AWAIT_LOGIN_ACK);
        deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
        return;
    }
    if (protocol == UserConnProtocol::NotifyUserLoginNack) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "login rejected");
        return;
    }
    if (protocol != UserConnProtocol::NotifyUserLoginAck ||
        state_ != MXH_UNITY_STATE_AWAIT_LOGIN_ACK) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected login protocol or state");
        return;
    }
    if (message.payload.size() != 23) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "LoginAck payload length is not 23 bytes");
        return;
    }
    const auto ack = mxh::client::parse_legacy_login_ack(message.payload);
    if (!ack || ack->agent_addr.empty() || ack->agent_port == 0 ||
        ack->user_idx == 0) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "invalid LoginAck payload");
        return;
    }
    user_index_ = ack->user_idx;
    user_level_ = ack->user_level;
    deadline_ = {};
    if (login_client_) {
        if (login_client_->is_connected()) login_client_->disconnect();
        login_client_.reset();
    }
    login_hsel_.reset();
    transition(MXH_UNITY_STATE_AGENT_CONNECTING);
    const auto connected = agent_.connect(ack->agent_addr, ack->agent_port,
        use_hsel_, std::chrono::milliseconds(timeout_ms_));
    if (connected != mxh::net::NetError::Ok) {
        fail(MXH_UNITY_NETWORK_ERROR,
             std::string("agent connect failed: ") + mxh::net::to_string(connected));
        return;
    }
    deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
}

void NativeClientCore::handle_agent_event(
    const mxh::client::ClientRuntimeEvent& event) {
    if (event.kind == mxh::client::ClientRuntimeEventKind::Message) {
        handle_agent_message(event.message);
    } else if (event.kind == mxh::client::ClientRuntimeEventKind::Error) {
        fail(MXH_UNITY_PROTOCOL_ERROR, event.detail);
    } else if (event.kind == mxh::client::ClientRuntimeEventKind::Disconnected &&
               state_ != MXH_UNITY_STATE_IDLE &&
               state_ != MXH_UNITY_STATE_FAILED &&
               state_ != MXH_UNITY_STATE_SHUTTING_DOWN) {
        fail(MXH_UNITY_NETWORK_ERROR, "agent disconnected");
    }
}

void NativeClientCore::handle_agent_message(const mxh::net::Message& message) {
    using mxh::proto::UserConnProtocol;
    if (message.header.category != kUserConn) {
        if (state_ != MXH_UNITY_STATE_IN_GAME) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected agent message category");
            return;
        }
        if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Move)) {
            using mxh::proto::MoveProtocol;
            namespace wire = mxh::proto::movement;
            const auto protocol = message.header.protocol;
            // Bounded versioned timed-movement state: MXMS with the new
            // owner_state_protocol / observer_state_protocol subprotocols.
            // Decode the wire, validate monotonic sequence, capture epoch for
            // outgoing commands, and surface to managed code via dedicated
            // event types so it can carry the full state in event.text.
            if (protocol == wire::owner_state_protocol ||
                protocol == wire::observer_state_protocol) {
                if (message.header.object_id == 0) return;
                const auto state = wire::decode_state(message.payload);
                if (!state) return;
                if (movement_epoch_ != 0 && state->epoch != movement_epoch_) return;
                if (movement_epoch_ == 0) movement_epoch_ = state->epoch;
                if (state->state_sequence <= last_movement_state_sequence_) return;
                last_movement_state_sequence_ = state->state_sequence;
                const auto packed = static_cast<std::uint32_t>(state->x) |
                    (static_cast<std::uint32_t>(state->z) << 16);
                const auto text_len = static_cast<std::uint32_t>(message.payload.size());
                std::string text;
                text.resize(text_len);
                if (text_len != 0) std::memcpy(text.data(), message.payload.data(), text_len);
                const auto event_type = protocol == wire::owner_state_protocol
                    ? MXH_UNITY_EVENT_TIMED_MOVEMENT_OWNER_STATE
                    : MXH_UNITY_EVENT_TIMED_MOVEMENT_OBSERVER_STATE;
                (void)emit(event_type, MXH_UNITY_OK, state->command_sequence,
                    message.header.object_id, packed, std::move(text), protocol);
                return;
            }
            const auto move = static_cast<MoveProtocol>(protocol);
            if (move != MoveProtocol::OneTarget && move != MoveProtocol::Stop &&
                move != MoveProtocol::Correction) return;
            const auto position = mxh::client::parse_move_payload(message.payload);
            if (!position || message.payload.size() != 4 || message.header.object_id == 0) {
                fail(MXH_UNITY_PROTOCOL_ERROR, "invalid modern movement payload");
                return;
            }
            const auto packed = static_cast<std::uint32_t>(position->first) |
                (static_cast<std::uint32_t>(position->second) << 16);
            if (message.header.object_id == game_.player_id) {
                // Normal broadcasts exclude the sender. Only Correction is
                // authoritative; a send or an ordinary echo is not an ACK.
                if (move != MoveProtocol::Correction) return;
                game_.position_x = position->first;
                game_.position_z = position->second;
                ++revision_;
                (void)emit(MXH_UNITY_EVENT_POSITION_CORRECTION, MXH_UNITY_OK, 0,
                    game_.player_id, packed, {}, message.header.protocol);
            } else {
                (void)emit(MXH_UNITY_EVENT_OBJECT_MOVEMENT, MXH_UNITY_OK, 0,
                    message.header.object_id, packed, {}, message.header.protocol);
            }
        }
        if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Monster) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::MonsterProtocol::LifeNotify)) {
            const auto life = mxh::client::parse_monster_life_payload(message.payload);
            if (!life || message.header.object_id == 0) {
                fail(MXH_UNITY_PROTOCOL_ERROR, "invalid Monster LifeNotify payload");
                return;
            }
            (void)emit(MXH_UNITY_EVENT_ENTITY_LIFE, MXH_UNITY_OK, 0,
                       message.header.object_id, life->first, {}, message.header.protocol);
            (void)emit(MXH_UNITY_EVENT_ENTITY_SHIELD, MXH_UNITY_OK, 0,
                       message.header.object_id, life->second, {}, message.header.protocol);
            return;
        }
        if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify)) {
            const auto drop = mxh::client::parse_legacy_ground_drop(message.payload);
            if (!drop) { fail(MXH_UNITY_PROTOCOL_ERROR, "invalid ground drop payload"); return; }
            const auto detail = std::to_string(drop->count) + "," + std::to_string(drop->position_x) + "," + std::to_string(drop->position_z);
            (void)emit(MXH_UNITY_EVENT_GROUND_DROP, MXH_UNITY_OK, 0, drop->object_id, drop->item_id, detail, message.header.protocol);
            return;
        }
        return;
    }
    const auto protocol = static_cast<UserConnProtocol>(message.header.protocol);
    if (state_ == MXH_UNITY_STATE_IN_GAME &&
        protocol == UserConnProtocol::MonsterAdd) {
        const auto parsed = mxh::client::parse_legacy_monster_add(message.payload);
        if (!parsed || parsed->object_id == 0) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid MonsterAdd payload");
            return;
        }
        const auto packed = static_cast<std::uint32_t>(parsed->position_x) |
            (static_cast<std::uint32_t>(parsed->position_z) << 16);
        (void)emit(MXH_UNITY_EVENT_MONSTER_ADDED, MXH_UNITY_OK, 0,
                   parsed->object_id, packed, parsed->name,
                   message.header.protocol);
        (void)emit(MXH_UNITY_EVENT_ENTITY_LIFE, MXH_UNITY_OK, 0,
                   parsed->object_id, parsed->current_life, {}, message.header.protocol);
        (void)emit(MXH_UNITY_EVENT_ENTITY_SHIELD, MXH_UNITY_OK, 0,
                   parsed->object_id, parsed->current_shield, {}, message.header.protocol);
        return;
    }
    if (state_ == MXH_UNITY_STATE_IN_GAME &&
        protocol == UserConnProtocol::NpcAdd) {
        const auto parsed = mxh::client::parse_legacy_npc_add(message.payload);
        if (!parsed || parsed->npc_id == 0) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid NpcAdd payload");
            return;
        }
        const auto packed = static_cast<std::uint32_t>(parsed->position_x) |
            (static_cast<std::uint32_t>(parsed->position_z) << 16);
        (void)emit(MXH_UNITY_EVENT_NPC_ADDED, MXH_UNITY_OK, 0,
                   parsed->npc_id, packed, parsed->name,
                   message.header.protocol);
        return;
    }
    if (state_ == MXH_UNITY_STATE_IN_GAME &&
        protocol == UserConnProtocol::ObjectRemove) {
        if (message.payload.size() < 4) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid ObjectRemove payload");
            return;
        }
        std::uint32_t object_id = 0;
        std::memcpy(&object_id, message.payload.data(), sizeof(object_id));
        if (object_id == 0) return;
        (void)emit(MXH_UNITY_EVENT_ENTITY_REMOVED, MXH_UNITY_OK, 0, object_id, 0, {}, message.header.protocol);
        return;
    }
    if (protocol == UserConnProtocol::AgentConnectSuccess) {
        if (state_ != MXH_UNITY_STATE_AGENT_CONNECTING || !agent_.is_ready() ||
            !message.payload.empty()) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected or unready AgentConnectSuccess");
            return;
        }
        mxh::net::Message request{};
        request.header.category = kUserConn;
        request.header.protocol = static_cast<std::uint8_t>(UserConnProtocol::CharacterListSyn);
        request.header.object_id = user_index_;
        request.payload = mxh::client::legacy_character_list_syn_payload(
            user_index_, auth_key_);
        const auto sent = agent_.send(request);
        if (sent != mxh::net::NetError::Ok) {
            fail(MXH_UNITY_NETWORK_ERROR, "CharacterListSyn send failed");
            return;
        }
        transition(MXH_UNITY_STATE_AWAIT_CHARACTER_LIST);
        deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
        return;
    }
    if (protocol == UserConnProtocol::CharacterListNack) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "character list rejected");
        return;
    }
    if (protocol == UserConnProtocol::CharacterListAck) {
        const bool creation_refresh =
            state_ == MXH_UNITY_STATE_AWAIT_CHARACTER_CREATE;
        if (state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_LIST &&
            !creation_refresh) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected CharacterListAck");
            return;
        }
        if (message.payload.size() != 889) {
            fail(MXH_UNITY_PROTOCOL_ERROR,
                 "CharacterListAck payload length is not 889 bytes");
            return;
        }
        std::int32_t advertised_count = 0;
        std::memcpy(&advertised_count, message.payload.data(), sizeof(advertised_count));
        if (advertised_count < 0 || advertised_count > 5) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "CharacterListAck count is out of range");
            return;
        }
        auto parsed = mxh::client::parse_legacy_character_list_ack(message.payload);
        if (!parsed) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid CharacterListAck payload");
            return;
        }
        for (auto& slot : *parsed) {
            if (!slot.valid) continue;
            auto utf8 = name_to_utf8(slot.name, legacy_text_code_page_);
            if (!utf8 || utf8->size() > MXH_UNITY_MAX_NAME_BYTES) {
                fail(MXH_UNITY_PROTOCOL_ERROR, "character name encoding failed");
                return;
            }
            slot.name = std::move(*utf8);
        }
        std::uint32_t count = 0;
        for (const auto& slot : *parsed) if (slot.valid) ++count;
        std::uint32_t created_character_id = 0;
        if (creation_refresh) {
            if (count != pending_character_ids_.size() + 1) {
                fail(MXH_UNITY_PROTOCOL_ERROR,
                     "character creation refresh count mismatch");
                return;
            }
            for (const auto old_id : pending_character_ids_) {
                if (!mxh::client::is_listed_character(*parsed, old_id)) {
                    fail(MXH_UNITY_PROTOCOL_ERROR,
                         "character creation refresh removed an existing character");
                    return;
                }
            }
            for (const auto& slot : *parsed) {
                if (!slot.valid || slot.name != pending_create_name_) continue;
                if (std::find(pending_character_ids_.begin(),
                              pending_character_ids_.end(), slot.chrid) !=
                    pending_character_ids_.end() || created_character_id != 0) {
                    fail(MXH_UNITY_PROTOCOL_ERROR,
                         "character creation refresh identity mismatch");
                    return;
                }
                created_character_id = slot.chrid;
            }
            if (created_character_id == 0) {
                fail(MXH_UNITY_PROTOCOL_ERROR,
                     "character creation refresh omitted requested character");
                return;
            }
        }
        const auto completed_request_id = pending_request_id_;
        const auto completed_name = pending_create_name_;
        characters_ = std::move(*parsed);
        deadline_ = {};
        pending_request_id_ = 0;
        pending_create_name_.clear();
        pending_character_ids_.clear();
        last_result_ = MXH_UNITY_OK;
        transition(MXH_UNITY_STATE_CHARACTER_LIST_READY);
        (void)emit(MXH_UNITY_EVENT_CHARACTER_LIST, MXH_UNITY_OK, 0, count);
        if (creation_refresh) {
            (void)emit(MXH_UNITY_EVENT_CHARACTER_CREATE, MXH_UNITY_OK,
                       completed_request_id, created_character_id, count,
                       completed_name);
        }
        return;
    }
    if (protocol == UserConnProtocol::CharacterMakeNack) {
        if (state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_CREATE ||
            !message.payload.empty() ||
            (message.header.object_id != 0 &&
             message.header.object_id != user_index_)) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid CharacterMakeNack framing");
            return;
        }
        const auto request_id = pending_request_id_;
        pending_request_id_ = 0;
        pending_create_name_.clear();
        pending_character_ids_.clear();
        deadline_ = {};
        last_result_ = MXH_UNITY_REJECTED;
        transition(MXH_UNITY_STATE_CHARACTER_LIST_READY);
        (void)emit(MXH_UNITY_EVENT_CHARACTER_CREATE, MXH_UNITY_REJECTED,
                   request_id, 0, 0, "character creation rejected");
        return;
    }
    if (protocol == UserConnProtocol::CharacterMakeAck) {
        if (state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_CREATE ||
            !message.payload.empty() ||
            (message.header.object_id != 0 &&
             message.header.object_id != user_index_)) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid CharacterMakeAck framing");
            return;
        }
        // Some legacy variants send this before the authoritative refreshed
        // character list. Success is published only after that list arrives.
        deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
        return;
    }
    if (protocol == UserConnProtocol::CharacterSelectNack) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "character selection rejected");
        return;
    }
    if (protocol == UserConnProtocol::CharacterSelectAck) {
        if (state_ != MXH_UNITY_STATE_AWAIT_CHARACTER_SELECT) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected CharacterSelectAck");
            return;
        }
        if (message.payload.size() != 1 ||
            (message.header.object_id != 0 &&
             message.header.object_id != selected_character_id_)) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid CharacterSelectAck framing");
            return;
        }
        const auto map = mxh::client::parse_legacy_character_select_ack(message.payload);
        if (!map || *map == 0) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid CharacterSelectAck payload");
            return;
        }
        selected_map_ = *map;
        const auto sent = agent_.send(
            mxh::client::make_legacy_gamein_syn_message(selected_character_id_));
        if (sent != mxh::net::NetError::Ok) {
            fail(MXH_UNITY_NETWORK_ERROR, "GameInSyn send failed");
            return;
        }
        transition(MXH_UNITY_STATE_AWAIT_GAME_IN);
        deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
        return;
    }
    if (protocol == UserConnProtocol::GameInNack) {
        fail(MXH_UNITY_PROTOCOL_ERROR, "game entry rejected");
        return;
    }
    if (protocol == UserConnProtocol::GameInAck) {
        if (state_ != MXH_UNITY_STATE_AWAIT_GAME_IN) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected GameInAck");
            return;
        }
        const auto parsed = mxh::client::parse_legacy_gamein_ack(message.payload);
        if (!parsed || parsed->player_id != selected_character_id_ ||
            parsed->user_id != user_index_ || parsed->map_num == 0 ||
            parsed->map_num != selected_map_ ||
            (message.header.object_id != 0 &&
             message.header.object_id != selected_character_id_)) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "invalid or mismatched GameInAck payload");
            return;
        }
        auto utf8_name = name_to_utf8(parsed->name, legacy_text_code_page_);
        if (!utf8_name || utf8_name->size() > MXH_UNITY_MAX_NAME_BYTES) {
            fail(MXH_UNITY_PROTOCOL_ERROR, "GameIn name encoding failed");
            return;
        }
        game_ = *parsed;
        game_.name = std::move(*utf8_name);
        selected_map_ = game_.map_num;
        ++map_generation_;
        deadline_ = {};
        transition(MXH_UNITY_STATE_IN_GAME);
        (void)emit(MXH_UNITY_EVENT_GAME_IN, MXH_UNITY_OK,
                   pending_request_id_, game_.player_id, game_.map_num);
        pending_request_id_ = 0;
        return;
    }
    if (state_ != MXH_UNITY_STATE_IN_GAME)
        fail(MXH_UNITY_PROTOCOL_ERROR, "unexpected agent protocol during handshake");
}

std::uint32_t NativeClientCore::submit(const mxh_unity_command& command) {
    std::lock_guard lock(mutex_);
    if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
    if (command.type != MXH_UNITY_COMMAND_SELECT_CHARACTER &&
        command.type != MXH_UNITY_COMMAND_CREATE_CHARACTER &&
        command.type != MXH_UNITY_COMMAND_MOVE && command.type != MXH_UNITY_COMMAND_STOP &&
        command.type != MXH_UNITY_COMMAND_PICKUP)
        return MXH_UNITY_UNSUPPORTED;
    if (command.expected_session_generation != session_generation_ ||
        command.expected_map_generation != map_generation_)
        return MXH_UNITY_WRONG_STATE;
    if (command.type == MXH_UNITY_COMMAND_MOVE || command.type == MXH_UNITY_COMMAND_STOP) {
        if (state_ != MXH_UNITY_STATE_IN_GAME) return MXH_UNITY_WRONG_STATE;
        if (command.payload_size != 0 || command.argument0 > 0xffffu ||
            command.argument1 > 0xffffu) return MXH_UNITY_INVALID_ARGUMENT;
        const auto protocol = command.type == MXH_UNITY_COMMAND_MOVE
            ? mxh::proto::MoveProtocol::OneTarget : mxh::proto::MoveProtocol::Stop;
        const auto x = static_cast<std::uint16_t>(command.argument0);
        const auto z = static_cast<std::uint16_t>(command.argument1);
        if (agent_.send(mxh::client::make_move_message(game_.player_id, protocol, x, z))
            != mxh::net::NetError::Ok) {
            fail(MXH_UNITY_NETWORK_ERROR, "movement send failed");
            return MXH_UNITY_NETWORK_ERROR;
        }
        game_.position_x = x;
        game_.position_z = z;
        ++revision_;
        (void)emit(MXH_UNITY_EVENT_MOVEMENT_SUBMITTED, MXH_UNITY_OK, command.request_id,
            game_.player_id, static_cast<std::uint32_t>(x) | (static_cast<std::uint32_t>(z) << 16),
            {}, static_cast<std::uint8_t>(protocol));
        return MXH_UNITY_OK;
    }
    if (command.type == MXH_UNITY_COMMAND_PICKUP) {
        if (state_ != MXH_UNITY_STATE_IN_GAME) return MXH_UNITY_WRONG_STATE;
        if (command.payload_size != 0 || command.argument0 == 0 || command.argument1 != 0)
            return MXH_UNITY_INVALID_ARGUMENT;
        const auto message = mxh::client::make_pickup_message(game_.player_id, command.argument0);
        if (agent_.send(message) != mxh::net::NetError::Ok) {
            fail(MXH_UNITY_NETWORK_ERROR, "pickup send failed");
            return MXH_UNITY_NETWORK_ERROR;
        }
        ++revision_;
        return MXH_UNITY_OK;
    }
    if (state_ != MXH_UNITY_STATE_CHARACTER_LIST_READY) return MXH_UNITY_WRONG_STATE;
    if (command.type == MXH_UNITY_COMMAND_CREATE_CHARACTER) {
        if (command.payload_size != MXH_UNITY_CREATE_COMMAND_PAYLOAD_SIZE ||
            command.argument0 != 0 || command.argument1 != 0 ||
            command.name_length == 0 ||
            command.name_length > MXH_UNITY_MAX_NAME_BYTES ||
            std::memchr(command.name, 0, command.name_length) != nullptr)
            return MXH_UNITY_INVALID_ARGUMENT;
        std::size_t current_count = 0;
        for (const auto& slot : characters_) if (slot.valid) ++current_count;
        if (current_count >= MXH_UNITY_MAX_CHARACTER_SLOTS)
            return MXH_UNITY_WRONG_STATE;
        std::string utf8_name(command.name, command.name_length);
        auto wire_name = name_from_utf8(utf8_name, legacy_text_code_page_);
        if (!wire_name) return MXH_UNITY_INVALID_ARGUMENT;
        auto params = mxh::client::legacy_china_character_make_params(
            std::move(*wire_name), command.sex_type, command.hair_type,
            command.face_type, command.cloth_option, command.boot_option,
            command.weapon_option);
        if (!params) return MXH_UNITY_INVALID_ARGUMENT;
        mxh::net::Message request{};
        request.header.category = kUserConn;
        request.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterMakeSyn);
        request.header.object_id = user_index_;
        auto payload = mxh::client::legacy_character_make_syn_payload(
            *params, user_index_);
        if (!payload) return MXH_UNITY_INVALID_ARGUMENT;
        request.payload = std::move(*payload);
        const auto sent = agent_.send(request);
        if (sent != mxh::net::NetError::Ok) {
            fail(MXH_UNITY_NETWORK_ERROR, "CharacterMakeSyn send failed");
            return MXH_UNITY_NETWORK_ERROR;
        }
        pending_request_id_ = command.request_id;
        pending_create_name_ = std::move(utf8_name);
        pending_character_ids_.clear();
        for (const auto& slot : characters_)
            if (slot.valid) pending_character_ids_.push_back(slot.chrid);
        last_result_ = MXH_UNITY_OK;
        transition(MXH_UNITY_STATE_AWAIT_CHARACTER_CREATE);
        deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
        return MXH_UNITY_OK;
    }
    if (command.payload_size != 0) return MXH_UNITY_INVALID_ARGUMENT;
    if (!mxh::client::is_listed_character(characters_, command.argument0))
        return MXH_UNITY_INVALID_ARGUMENT;
    if (command.argument1 > 0xffffu) return MXH_UNITY_INVALID_ARGUMENT;
    mxh::net::Message request{};
    request.header.category = kUserConn;
    request.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterSelectSyn);
    request.header.object_id = command.argument0;
    request.payload = mxh::client::legacy_character_select_syn_payload(
        static_cast<std::uint16_t>(command.argument1));
    const auto sent = agent_.send(request);
    if (sent != mxh::net::NetError::Ok) {
        fail(MXH_UNITY_NETWORK_ERROR, "CharacterSelectSyn send failed");
        return MXH_UNITY_NETWORK_ERROR;
    }
    selected_character_id_ = command.argument0;
    pending_request_id_ = command.request_id;
    pending_create_name_.clear();
    pending_character_ids_.clear();
    last_result_ = MXH_UNITY_OK;
    transition(MXH_UNITY_STATE_AWAIT_CHARACTER_SELECT);
    deadline_ = Clock::now() + std::chrono::milliseconds(timeout_ms_);
    return MXH_UNITY_OK;
}

void NativeClientCore::transition(std::uint32_t state) {
    state_ = state;
    ++revision_;
    (void)emit(MXH_UNITY_EVENT_STATE_CHANGED, MXH_UNITY_OK, 0, state);
}

void NativeClientCore::fail(std::uint32_t result, std::string detail) {
    if (state_ == MXH_UNITY_STATE_FAILED) return;
    last_result_ = result;
    last_error_ = std::move(detail);
    clear_secret(password_);
    user_id_.clear();
    auth_key_ = 0;
    pending_request_id_ = 0;
    pending_create_name_.clear();
    pending_character_ids_.clear();
    deadline_ = {};
    state_ = MXH_UNITY_STATE_FAILED;
    ++revision_;
    close_connections();
    (void)emit(MXH_UNITY_EVENT_ERROR, result, 0, 0, 0, last_error_);
}

bool NativeClientCore::emit(std::uint32_t type, std::uint32_t result,
                            std::uint64_t request_id, std::uint32_t argument0,
                            std::uint32_t argument1, const std::string& text,
                            std::uint32_t wire_protocol) {
    const bool terminal = type == MXH_UNITY_EVENT_ERROR ||
                          type == MXH_UNITY_EVENT_DISCONNECTED;
    const auto make_event = [&] {
        mxh_unity_event event{};
        event.struct_size = sizeof(event);
        event.type = type;
        event.result = result;
        event.state = state_;
        event.sequence = next_event_sequence_++;
        event.request_id = request_id;
        event.session_generation = session_generation_;
        event.map_generation = map_generation_;
        event.argument0 = argument0;
        event.argument1 = argument1;
        event.reserved0 = wire_protocol;
        copy_text(event.text, sizeof(event.text), event.text_length, text);
        return event;
    };
    if (events_.size() >= event_capacity_) {
        ++dropped_events_;
        if (!terminal) return false;
        // Replace one already allocated slot.  A terminal event must remain
        // observable even if allocating a new deque block would fail.
        events_.back() = make_event();
        return true;
    }
    events_.push_back(make_event());
    return true;
}

std::uint32_t NativeClientCore::submit_extended(const mxh_unity_extended_command& command) {
    std::lock_guard lock(mutex_);
    if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
    if (command.head.type == MXH_UNITY_COMMAND_SKILL) {
        if (command.head.expected_session_generation != session_generation_ ||
            command.head.expected_map_generation != map_generation_)
            return MXH_UNITY_WRONG_STATE;
        if (state_ != MXH_UNITY_STATE_IN_GAME || !command.payload ||
            command.payload_size != MXH_UNITY_SKILL_PAYLOAD_SIZE ||
            command.head.argument0 == 0 || command.head.argument1 == 0)
            return state_ == MXH_UNITY_STATE_IN_GAME ? MXH_UNITY_INVALID_ARGUMENT : MXH_UNITY_WRONG_STATE;
        float target_x = 0.0f, target_z = 0.0f;
        std::memcpy(&target_x, command.payload, sizeof(target_x));
        std::memcpy(&target_z, command.payload + sizeof(target_x), sizeof(target_z));
        if (!std::isfinite(target_x) || !std::isfinite(target_z)) return MXH_UNITY_INVALID_ARGUMENT;
        mxh::net::Message message{};
        message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
        message.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
        message.header.object_id = game_.player_id;
        message.payload.resize(16);
        std::memcpy(message.payload.data(), &command.head.argument0, 4);
        std::memcpy(message.payload.data() + 4, &command.head.argument1, 4);
        std::memcpy(message.payload.data() + 8, &target_x, 4);
        std::memcpy(message.payload.data() + 12, &target_z, 4);
        if (agent_.send(message) != mxh::net::NetError::Ok) { fail(MXH_UNITY_NETWORK_ERROR, "skill send failed"); return MXH_UNITY_NETWORK_ERROR; }
        ++revision_;
        return MXH_UNITY_OK;
    }
    namespace wire = mxh::proto::movement;
    switch (command.head.type) {
        case MXH_UNITY_COMMAND_HELLO_TIMED:
        case MXH_UNITY_COMMAND_TIMED_ROUTE:
        case MXH_UNITY_COMMAND_TIMED_STOP:
            break;
        default:
            return MXH_UNITY_UNSUPPORTED;
    }
    if (command.head.expected_session_generation != session_generation_ ||
        command.head.expected_map_generation != map_generation_)
        return MXH_UNITY_WRONG_STATE;
    if (state_ != MXH_UNITY_STATE_IN_GAME) return MXH_UNITY_WRONG_STATE;
    if (!command.payload || command.payload_size == 0)
        return MXH_UNITY_INVALID_ARGUMENT;
    if (command.head.type == MXH_UNITY_COMMAND_HELLO_TIMED) {
        if (command.payload_size != MXH_UNITY_TIMED_MOVEMENT_HELLO_PAYLOAD)
            return MXH_UNITY_INVALID_ARGUMENT;
        if (std::memcmp(command.payload, wire::hello_payload.data(),
                        wire::hello_payload.size()) != 0)
            return MXH_UNITY_INVALID_ARGUMENT;
    } else {
        if (command.payload_size < wire::command_header_size ||
            command.payload_size > MXH_UNITY_TIMED_MOVEMENT_MAX_PAYLOAD ||
            (command.payload_size - wire::command_header_size) % 4 != 0)
            return MXH_UNITY_INVALID_ARGUMENT;
        const auto decoded = wire::decode_command(
            std::span<const std::uint8_t>(command.payload, command.payload_size));
        if (!decoded) return MXH_UNITY_INVALID_ARGUMENT;
        // Bind the command to the session epoch the server handed us in the
        // first state broadcast. The C# helper builds the wire payload, so
        // this is the only spot that enforces the cross-field contract.
        if (movement_epoch_ == 0) return MXH_UNITY_WRONG_STATE;
        if (decoded->epoch != movement_epoch_) return MXH_UNITY_PROTOCOL_ERROR;
        if (decoded->sequence == 0 ||
            decoded->sequence <= next_movement_command_sequence_)
            return MXH_UNITY_PROTOCOL_ERROR;
        next_movement_command_sequence_ = decoded->sequence;
    }
    mxh::net::Message message{};
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    message.header.protocol = command.head.type == MXH_UNITY_COMMAND_HELLO_TIMED
        ? wire::hello_protocol : wire::command_protocol;
    message.header.object_id = game_.player_id;
    message.payload.assign(command.payload,
                          command.payload + command.payload_size);
    const auto sent = agent_.send(message);
    if (sent != mxh::net::NetError::Ok) {
        fail(MXH_UNITY_NETWORK_ERROR, "timed movement send failed");
        return MXH_UNITY_NETWORK_ERROR;
    }
    ++revision_;
    return MXH_UNITY_OK;
}

std::uint32_t NativeClientCore::poll_event(mxh_unity_event& event) {
    std::lock_guard lock(mutex_);
    if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
    if (events_.empty()) return MXH_UNITY_NOT_READY;
    event = events_.front();
    events_.pop_front();
    return MXH_UNITY_OK;
}

std::uint32_t NativeClientCore::copy_snapshot(mxh_unity_snapshot& snapshot) const {
    std::lock_guard lock(mutex_);
    if (destroyed_) return MXH_UNITY_INVALID_HANDLE;
    snapshot = {};
    snapshot.struct_size = sizeof(snapshot);
    snapshot.api_version = MXH_UNITY_API_VERSION;
    snapshot.state = state_;
    snapshot.last_result = last_result_;
    snapshot.revision = revision_;
    snapshot.session_generation = session_generation_;
    snapshot.map_generation = map_generation_;
    snapshot.user_id = user_index_;
    snapshot.selected_character_id = selected_character_id_;
    snapshot.selected_map_number = selected_map_;
    snapshot.dropped_event_count = dropped_events_;
    std::size_t valid_count = 0;
    for (std::size_t i = 0; i < characters_.size() && i < 5; ++i) {
        const auto& source = characters_[i];
        auto& target = snapshot.characters[i];
        target.character_id = source.chrid;
        target.valid = source.valid ? 1u : 0u;
        target.level = source.level;
        target.map_number = source.map_num;
        target.gender = source.gender;
        target.face_type = source.face_type;
        target.hair_type = source.hair_type;
        std::copy(source.weared_item_idx.begin(), source.weared_item_idx.end(),
                  target.worn_item_index);
        copy_text(target.name, sizeof(target.name), target.name_length, source.name);
        if (source.valid) ++valid_count;
    }
    snapshot.character_count = static_cast<std::uint16_t>(valid_count);
    snapshot.game.player_id = game_.player_id;
    snapshot.game.user_id = game_.user_id;
    copy_text(snapshot.game.name, sizeof(snapshot.game.name),
              snapshot.game.name_length, game_.name);
    snapshot.game.level = game_.level;
    snapshot.game.map_number = game_.map_num;
    snapshot.game.life = game_.life;
    snapshot.game.max_life = game_.max_life;
    snapshot.game.mp = game_.mp;
    snapshot.game.max_mp = game_.max_mp;
    snapshot.game.experience = game_.exp;
    snapshot.game.money = game_.money;
    snapshot.game.gen_gol = game_.gen_gol;
    snapshot.game.min_chub = game_.min_chub;
    snapshot.game.che_ryuk = game_.che_ryuk;
    snapshot.game.sim_mek = game_.sim_mek;
    snapshot.game.position_x = game_.position_x;
    snapshot.game.position_z = game_.position_z;
    snapshot.game.server_year = game_.server_year;
    snapshot.game.server_month = game_.server_month;
    snapshot.game.server_day = game_.server_day;
    snapshot.game.server_hour = game_.server_hour;
    copy_text(snapshot.error, sizeof(snapshot.error), snapshot.error_length,
              last_error_);
    return MXH_UNITY_OK;
}

bool NativeClientCore::on_connect(mxh::net::ConnectionId id,
                                  const std::string& remote_addr) {
    mxh::client::ClientRuntimeEvent event;
    event.kind = mxh::client::ClientRuntimeEventKind::Connected;
    event.connection = id;
    event.detail = remote_addr;
    return login_events_.push(std::move(event));
}

void NativeClientCore::on_message(mxh::net::ConnectionId id,
                                  const mxh::net::Message& message) {
    if (message.header.category == kUserConn &&
        message.header.protocol == mxh::proto::kModernHselKey) {
        mxh::client::ClientRuntimeEvent error;
        error.kind = mxh::client::ClientRuntimeEventKind::Error;
        error.connection = id;
        if (!use_hsel_ || login_hsel_received_) {
            error.detail = "unexpected or duplicate login HSEL key";
            (void)login_events_.push(std::move(error));
            return;
        }
        if (message.payload.size() != sizeof(mxh::crypto::HselInit)) {
            error.detail = "login HSEL key has invalid length";
            (void)login_events_.push(std::move(error));
            return;
        }
        mxh::crypto::HselInit init{};
        std::memcpy(&init, message.payload.data(), sizeof(init));
        if (!login_hsel_ || !login_hsel_->import_init(init)) {
            error.detail = "login HSEL key import failed";
            (void)login_events_.push(std::move(error));
            return;
        }
        login_hsel_received_ = true;
        return;
    }
    mxh::client::ClientRuntimeEvent event;
    event.kind = mxh::client::ClientRuntimeEventKind::Message;
    event.connection = id;
    event.message = message;
    if (!login_events_.push(std::move(event))) {
        mxh::client::ClientRuntimeEvent overflow;
        overflow.kind = mxh::client::ClientRuntimeEventKind::Error;
        overflow.detail = "login event queue overflow";
        (void)login_events_.push(std::move(overflow));
    }
}

void NativeClientCore::on_disconnect(mxh::net::ConnectionId id,
                                     mxh::net::NetError reason) {
    mxh::client::ClientRuntimeEvent event;
    event.kind = mxh::client::ClientRuntimeEventKind::Disconnected;
    event.connection = id;
    event.network_error = reason;
    (void)login_events_.push(std::move(event));
}

mxh::net::IEncryptor* NativeClientCore::encryptor_for(
    mxh::net::ConnectionId) {
    return login_hsel_.get();
}

}  // namespace mxh::unity
