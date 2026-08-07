// hsel_session.hpp - Per-connection HSEL session bookkeeping for the
// server handlers (Login/Agent/Map).
//
// Phase R-1: each handler owns one HselSessionManager. The manager:
//   * lazily creates an unseeded HselStreamCipher per connection at
//     accept time (legacy pass-through, so the key-delivery message can
//     ride the plaintext phase);
//   * delivers the key-first handshake (kModernHselKey with the fully
//     resolved 64-byte HselInit) synchronously via direct_send, then
//     re-arms the cipher so every subsequent frame is HSEL-encrypted;
//   * cleans up the cipher on disconnect.
//
// The handshake must run from on_connect AFTER the net layer has
// registered the connection (the net layer registers connections before
// invoking on_connect), because direct_send resolves via the connection
// table.

#pragma once

#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/net/net.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace mxh::server {

class HselSessionManager {
public:
    using DirectSendFn = std::function<void(
        mxh::net::ConnectionId, const mxh::net::Message&)>;

    HselSessionManager(bool enabled, DirectSendFn direct_send);

    bool enabled() const noexcept { return enabled_; }

    // Returns the per-connection cipher (creating it unseeded on first
    // call) or nullptr when HSEL is disabled.
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id);

    // Key-first handshake for a fresh connection. Returns true when the
    // session is armed (or HSEL is disabled / direct_send missing and
    // the session degrades to plaintext pass-through).
    bool handshake(mxh::net::ConnectionId id, std::uint8_t category);

    void on_disconnect(mxh::net::ConnectionId id);

private:
    bool enabled_;
    DirectSendFn direct_send_;
    std::mutex mu_;
    std::unordered_map<std::uint64_t,
                       std::unique_ptr<mxh::crypto::HselStreamCipher>>
        ciphers_;
};

}  // namespace mxh::server
