#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace mxh::server {

// One agent->map socket as seen by the leave selector.
// present: the pointer is non-null. connected: is_connected() is true.
struct MapLeaveEndpoint {
    bool present = false;
    bool connected = false;
};

enum class MapLeaveForward : std::uint8_t {
    None = 0,
    Slot = 1,
    DefaultMapClient = 2,
};

// route_for_map returns the map_routes_ entry when the key exists, even
// when its client is null, and returns map_client_ only when the key is
// absent. The leave uses that result, then: a present slot with a null
// client falls back to map_client_ when that default socket is connected.
// A non-null but disconnected slot client is left alone; that socket is
// the one the map server sees close.
inline MapLeaveForward select_map_leave_forward(
    bool slot_key_present,
    MapLeaveEndpoint slot_client,
    MapLeaveEndpoint default_client) noexcept {
    if (slot_key_present && slot_client.present && slot_client.connected)
        return MapLeaveForward::Slot;
    if (!slot_client.present && default_client.present && default_client.connected)
        return MapLeaveForward::DefaultMapClient;
    return MapLeaveForward::None;
}

// In-flight leave sends. wait_until_idle blocks until every Claim is
// destroyed. wait_for releases the mutex, so another thread can end a
// claim. Calling wait on the thread that still holds a Claim deadlocks;
// production release runs on the agent main loop, and clear_session_routes
// runs on the client recv thread (see release_map_sender).
class MapSenderClaimGate {
public:
    class Claim {
    public:
        Claim() = default;
        Claim(const Claim&) = delete;
        Claim& operator=(const Claim&) = delete;
        Claim(Claim&& other) noexcept : gate_(other.gate_) { other.gate_ = nullptr; }
        Claim& operator=(Claim&& other) noexcept {
            if (this != &other) {
                reset();
                gate_ = other.gate_;
                other.gate_ = nullptr;
            }
            return *this;
        }
        ~Claim() { reset(); }
        void reset() noexcept {
            if (gate_ != nullptr) {
                gate_->release();
                gate_ = nullptr;
            }
        }

    private:
        friend class MapSenderClaimGate;
        explicit Claim(MapSenderClaimGate* gate) : gate_(gate) {}
        MapSenderClaimGate* gate_ = nullptr;
    };

    Claim acquire() {
        std::lock_guard<std::mutex> lock(mu_);
        ++claims_;
        return Claim(this);
    }

    void wait_until_idle() {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [this] { return claims_ == 0; });
    }

    template <typename Rep, typename Period>
    bool wait_for_idle(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mu_);
        return cv_.wait_for(lock, timeout, [this] { return claims_ == 0; });
    }

private:
    void release() noexcept {
        std::lock_guard<std::mutex> lock(mu_);
        if (claims_ > 0) --claims_;
        cv_.notify_all();
    }

    std::mutex mu_;
    std::condition_variable cv_;
    int claims_ = 0;
};

}  // namespace mxh::server
