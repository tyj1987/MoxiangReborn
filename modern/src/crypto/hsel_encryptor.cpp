// hsel_encryptor.cpp - HselStreamCipher implementation.

#include "mxh/crypto/hsel_encryptor.hpp"

#include <atomic>
#include <chrono>

namespace mxh::crypto {
namespace {

// Legacy CCrypt::Create() seeded the HSEL dongle RNG from the tick count.
// We combine a steady-clock seed with a monotonic counter so consecutive
// seed() calls on the same process always produce distinct sessions.
std::uint32_t next_seed() {
    static std::atomic<std::uint32_t> counter{0u};
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const std::uint32_t tick =
        static_cast<std::uint32_t>(now.count() & 0xFFFFFFFFu);
    const std::uint32_t n =
        counter.fetch_add(1u, std::memory_order_relaxed);
    return tick ^ (n * 2654435761u);
}

HselInit default_init() {
    HselInit init;
    init.iEncryptType = HSEL_ENCRYPTTYPE_RAND;
    init.iDesCount    = HSEL_DES_TRIPLE;
    init.iSwapFlag    = HSEL_SWAP_FLAG_ON;
    init.iCustomize   = HSEL_KEY_TYPE_DEFAULT;
    return init;
}

}  // namespace

void HselStreamCipher::seed() {
    std::lock_guard lock(mutex_);
    encrypt_stream_.rng().set_state(next_seed());
    init_ = default_init();
    const std::int32_t rt = encrypt_stream_.initial(init_);
    if (rt == 0) {
        ready_ = false;
        return;
    }
    // initial() resolved the RAND type and generated concrete keys into
    // the stream's own init; re-read it so export_init() carries the
    // fully-resolved session (identical on the peer after import).
    init_ = encrypt_stream_.hsel_init();
    ready_ = decrypt_stream_.initial(init_) != 0;
}

mxh::net::NetError HselStreamCipher::encrypt(
    std::span<std::uint8_t> data) {
    std::lock_guard lock(mutex_);
    if (!ready_) {
        // 1:1 with legacy CCrypt::Encrypt: not inited -> pass through.
        return mxh::net::NetError::Ok;
    }
    if (data.empty()) {
        return mxh::net::NetError::Ok;
    }
    const std::int32_t size = static_cast<std::int32_t>(data.size());
    if (size <= 0) {
        return mxh::net::NetError::EncryptionFailed;
    }
    const bool ok =
        encrypt_stream_.encrypt(reinterpret_cast<char*>(data.data()), size);
    return ok ? mxh::net::NetError::Ok
              : mxh::net::NetError::EncryptionFailed;
}

mxh::net::NetError HselStreamCipher::decrypt(
    std::span<std::uint8_t> data) {
    std::lock_guard lock(mutex_);
    if (!ready_) {
        // 1:1 with legacy CCrypt::Decrypt: not inited -> pass through.
        return mxh::net::NetError::Ok;
    }
    if (data.empty()) {
        return mxh::net::NetError::Ok;
    }
    const std::int32_t size = static_cast<std::int32_t>(data.size());
    if (size <= 0) {
        return mxh::net::NetError::DecryptionFailed;
    }
    const bool ok =
        decrypt_stream_.decrypt(reinterpret_cast<char*>(data.data()), size);
    return ok ? mxh::net::NetError::Ok
              : mxh::net::NetError::DecryptionFailed;
}

bool HselStreamCipher::export_init(HselInit& out) const {
    std::lock_guard lock(mutex_);
    if (!ready_) {
        return false;
    }
    out = init_;
    return true;
}

bool HselStreamCipher::import_init(const HselInit& init) {
    std::lock_guard lock(mutex_);
    init_ = init;
    const std::int32_t en = encrypt_stream_.initial(init_);
    const std::int32_t de = decrypt_stream_.initial(init_);
    ready_ = en != 0 && de != 0;
    return ready_;
}

void HselStreamCipher::reset() noexcept {
    std::lock_guard lock(mutex_);
    encrypt_stream_ = HselStream{};
    decrypt_stream_ = HselStream{};
    init_ = HselInit{};
    ready_ = false;
}

}  // namespace mxh::crypto
