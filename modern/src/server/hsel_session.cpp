// hsel_session.cpp - HselSessionManager implementation.

#include "mxh/server/hsel_session.hpp"

#include "mxh/proto/protocol.hpp"

#include <cstring>
#include <iostream>
#include <vector>

namespace mxh::server {

namespace {

std::vector<std::uint8_t> serialize_hsel_init(
    const mxh::crypto::HselInit& init) {
    static_assert(sizeof(mxh::crypto::HselInit) == 64u,
                  "HselInit must be 64 bytes on the key-delivery wire");
    std::vector<std::uint8_t> out(sizeof(init));
    std::memcpy(out.data(), &init, sizeof(init));
    return out;
}

}  // namespace

HselSessionManager::HselSessionManager(bool enabled,
                                       DirectSendFn direct_send)
    : enabled_(enabled), direct_send_(std::move(direct_send)) {}

mxh::net::IEncryptor* HselSessionManager::encryptor_for(
    mxh::net::ConnectionId id) {
    if (!enabled_) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lk(mu_);
    auto it = ciphers_.find(id.value);
    if (it == ciphers_.end()) {
        auto cipher = std::make_unique<mxh::crypto::HselStreamCipher>();
        mxh::crypto::HselStreamCipher* raw = cipher.get();
        ciphers_.emplace(id.value, std::move(cipher));
        return raw;
    }
    return it->second.get();
}

bool HselSessionManager::handshake(mxh::net::ConnectionId id,
                                   std::uint8_t category) {
    if (!enabled_) {
        return true;  // plaintext session
    }
    if (!direct_send_) {
        std::cout << "[HSEL] use_hsel requires a direct_send hook; "
                     "falling back to plaintext session\n";
        return true;
    }
    mxh::crypto::HselStreamCipher* cipher =
        static_cast<mxh::crypto::HselStreamCipher*>(encryptor_for(id));
    cipher->seed();
    mxh::crypto::HselInit init{};
    if (!cipher->export_init(init)) {
        std::cout << "[HSEL] seed failed; falling back to plaintext "
                     "session\n";
        return true;
    }
    // Key message must be plaintext: reset to pass-through, deliver,
    // then re-arm with the exported keys.
    cipher->reset();
    mxh::net::Message key_msg;
    key_msg.header.category = category;
    key_msg.header.protocol = mxh::proto::kModernHselKey;
    key_msg.header.object_id = 0;
    key_msg.payload = serialize_hsel_init(init);
    direct_send_(id, key_msg);
    return cipher->import_init(init);
}

void HselSessionManager::on_disconnect(mxh::net::ConnectionId id) {
    std::lock_guard<std::mutex> lk(mu_);
    ciphers_.erase(id.value);
}

}  // namespace mxh::server
