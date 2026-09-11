#include "NativeClientCore.hpp"

#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {
std::mutex g_registry_mutex;
std::unordered_map<mxh_unity_handle,
                   std::shared_ptr<mxh::unity::NativeClientCore>> g_registry;
std::atomic<std::uint64_t> g_next_handle{1};

std::shared_ptr<mxh::unity::NativeClientCore> acquire(mxh_unity_handle handle) {
    if (handle == 0) return {};
    std::lock_guard lock(g_registry_mutex);
    const auto found = g_registry.find(handle);
    return found == g_registry.end() ? nullptr : found->second;
}

bool contains_nul(const char* data, std::uint32_t length) {
    return length != 0 && std::memchr(data, 0, length) != nullptr;
}
}

extern "C" {

std::uint32_t MXH_UNITY_CALL mxh_unity_get_api_version(void) {
    return MXH_UNITY_API_VERSION;
}

std::uint32_t MXH_UNITY_CALL mxh_unity_create(mxh_unity_handle* out_handle) {
    if (!out_handle) return MXH_UNITY_INVALID_ARGUMENT;
    *out_handle = 0;
    try {
        auto core = std::make_shared<mxh::unity::NativeClientCore>();
        auto handle = g_next_handle.fetch_add(1, std::memory_order_relaxed);
        if (handle == 0) handle = g_next_handle.fetch_add(1, std::memory_order_relaxed);
        {
            std::lock_guard lock(g_registry_mutex);
            g_registry.emplace(handle, std::move(core));
        }
        *out_handle = handle;
        return MXH_UNITY_OK;
    } catch (...) {
        return MXH_UNITY_INTERNAL_ERROR;
    }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_destroy(mxh_unity_handle handle) {
    std::shared_ptr<mxh::unity::NativeClientCore> core;
    {
        std::lock_guard lock(g_registry_mutex);
        const auto found = g_registry.find(handle);
        if (found == g_registry.end()) return MXH_UNITY_INVALID_HANDLE;
        core = std::move(found->second);
        g_registry.erase(found);
    }
    try {
        return core->destroy();
    } catch (...) {
        return MXH_UNITY_INTERNAL_ERROR;
    }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_connect(
    mxh_unity_handle handle, const mxh_unity_connect_args* args) {
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    if (!args || args->struct_size < sizeof(mxh_unity_connect_args) ||
        args->login_port == 0 || args->host_length == 0 ||
        args->host_length > MXH_UNITY_MAX_HOST_BYTES ||
        args->user_id_length == 0 ||
        args->user_id_length > MXH_UNITY_MAX_CREDENTIAL_BYTES ||
        args->password_length == 0 ||
        args->password_length > MXH_UNITY_MAX_CREDENTIAL_BYTES ||
        contains_nul(args->login_host, args->host_length) ||
        contains_nul(args->user_id, args->user_id_length) ||
        contains_nul(args->password, args->password_length))
        return MXH_UNITY_INVALID_ARGUMENT;
    constexpr std::uint32_t kKnownFlags = MXH_UNITY_CONNECT_USE_HSEL |
        MXH_UNITY_CONNECT_LEGACY_TEXT_CP949 | MXH_UNITY_CONNECT_LEGACY_TEXT_CP936;
    if ((args->flags & ~kKnownFlags) != 0 ||
        ((args->flags & MXH_UNITY_CONNECT_LEGACY_TEXT_CP949) != 0 &&
         (args->flags & MXH_UNITY_CONNECT_LEGACY_TEXT_CP936) != 0))
        return MXH_UNITY_INVALID_ARGUMENT;
    try {
        return core->connect(
            std::string(args->login_host, args->host_length), args->login_port,
            std::string(args->user_id, args->user_id_length),
            std::string(args->password, args->password_length), args->flags,
            args->timeout_ms == 0 ? 10000u : args->timeout_ms);
    } catch (...) {
        return MXH_UNITY_INTERNAL_ERROR;
    }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_disconnect(mxh_unity_handle handle) {
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    try { return core->disconnect(); }
    catch (...) { return MXH_UNITY_INTERNAL_ERROR; }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_tick(mxh_unity_handle handle) {
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    try { return core->tick(); }
    catch (...) { return MXH_UNITY_INTERNAL_ERROR; }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_submit_command(
    mxh_unity_handle handle, const mxh_unity_command* command) {
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    if (!command || command->struct_size < sizeof(mxh_unity_command))
        return MXH_UNITY_INVALID_ARGUMENT;
    if ((command->type == MXH_UNITY_COMMAND_SELECT_CHARACTER &&
         command->payload_size != 0) ||
        (command->type == MXH_UNITY_COMMAND_CREATE_CHARACTER &&
         command->payload_size != MXH_UNITY_CREATE_COMMAND_PAYLOAD_SIZE))
        return MXH_UNITY_INVALID_ARGUMENT;
    // Reject timed-movement types from the legacy entry: callers must use
    // mxh_unity_submit_extended_command so the variable-length payload is
    // delivered through the dedicated channel.
    if (command->type == MXH_UNITY_COMMAND_HELLO_TIMED ||
        command->type == MXH_UNITY_COMMAND_TIMED_ROUTE ||
        command->type == MXH_UNITY_COMMAND_TIMED_STOP)
        return MXH_UNITY_UNSUPPORTED;
    try { return core->submit(*command); }
    catch (...) { return MXH_UNITY_INTERNAL_ERROR; }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_submit_extended_command(
    mxh_unity_handle handle, const mxh_unity_extended_command* command) {
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    if (!command) return MXH_UNITY_INVALID_ARGUMENT;
    if (command->head.struct_size < sizeof(mxh_unity_command) ||
        command->payload_size > MXH_UNITY_TIMED_MOVEMENT_MAX_PAYLOAD)
        return MXH_UNITY_INVALID_ARGUMENT;
    try { return core->submit_extended(*command); }
    catch (...) { return MXH_UNITY_INTERNAL_ERROR; }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_poll_event(
    mxh_unity_handle handle, void* event_buffer, std::uint32_t event_buffer_size,
    std::uint32_t* out_required_size) {
    if (!out_required_size) return MXH_UNITY_INVALID_ARGUMENT;
    *out_required_size = sizeof(mxh_unity_event);
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    if (!event_buffer || event_buffer_size < sizeof(mxh_unity_event))
        return MXH_UNITY_BUFFER_TOO_SMALL;
    try {
        mxh_unity_event event{};
        const auto result = core->poll_event(event);
        if (result == MXH_UNITY_OK)
            std::memcpy(event_buffer, &event, sizeof(event));
        return result;
    } catch (...) {
        return MXH_UNITY_INTERNAL_ERROR;
    }
}

std::uint32_t MXH_UNITY_CALL mxh_unity_copy_snapshot(
    mxh_unity_handle handle, void* snapshot_buffer,
    std::uint32_t snapshot_buffer_size, std::uint32_t* out_required_size) {
    if (!out_required_size) return MXH_UNITY_INVALID_ARGUMENT;
    *out_required_size = sizeof(mxh_unity_snapshot);
    auto core = acquire(handle);
    if (!core) return MXH_UNITY_INVALID_HANDLE;
    if (!snapshot_buffer || snapshot_buffer_size < sizeof(mxh_unity_snapshot))
        return MXH_UNITY_BUFFER_TOO_SMALL;
    try {
        mxh_unity_snapshot snapshot{};
        const auto result = core->copy_snapshot(snapshot);
        if (result != MXH_UNITY_OK) return result;
        std::memcpy(snapshot_buffer, &snapshot, sizeof(snapshot));
        return MXH_UNITY_OK;
    } catch (...) {
        return MXH_UNITY_INTERNAL_ERROR;
    }
}

}  // extern "C"
