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
    stream_.rng().set_state(next_seed());
    init_ = default_init();
    const std::int32_t rt = stream_.initial(init_);
    if (rt == 0) {
        ready_ = false;
        return;
    }
    // initial() resolved the RAND type and generated concrete keys into
    // the stream's own init; re-read it so export_init() carries the
    // fully-resolved session (identical on the peer after import).
    init_ = stream_.hsel_init();
    ready_ = true;
}

mxh::net::NetError HselStreamCipher::encrypt(
    std::span<std::uint8_t> data) {
    if (!ready_) {
        return mxh::net::NetError::EncryptionFailed;
    }
    if (data.empty()) {
        return mxh::net::NetError::Ok;
    }
    const std::int32_t size = static_cast<std::int32_t>(data.size());
    if (size <= 0) {
        return mxh::net::NetError::EncryptionFailed;
    }
    const bool ok =
        stream_.encrypt(reinterpret_cast<char*>(data.data()), size);
    return ok ? mxh::net::NetError::Ok
              : mxh::net::NetError::EncryptionFailed;
}

mxh::net::NetError HselStreamCipher::decrypt(
    std::span<std::uint8_t> data) {
    if (!ready_) {
        return mxh::net::NetError::DecryptionFailed;
    }
    if (data.empty()) {
        return mxh::net::NetError::Ok;
    }
    const std::int32_t size = static_cast<std::int32_t>(data.size());
    if (size <= 0) {
        return mxh::net::NetError::DecryptionFailed;
    }
    const bool ok =
        stream_.decrypt(reinterpret_cast<char*>(data.data()), size);
    return ok ? mxh::net::NetError::Ok
              : mxh::net::NetError::DecryptionFailed;
}

bool HselStreamCipher::export_init(HselInit& out) const {
    if (!ready_) {
        return false;
    }
    out = init_;
    return true;
}

bool HselStreamCipher::import_init(const HselInit& init) {
    init_ = init;
    const std::int32_t rt = stream_.initial(init_);
    ready_ = (rt != 0);
    return ready_;
}

}  // namespace mxh::crypto
