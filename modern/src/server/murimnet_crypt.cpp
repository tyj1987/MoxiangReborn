#include "mxh/server/murimnet_crypt.hpp"
#include "mxh/crypto/hsel_stream.hpp"
namespace mxh::server {
namespace {
    // Mirror legacy CCrypt::Create() default HselInit (HSEL.cpp:21-32).
    mxh::crypto::HselInit make_default_init() {
        mxh::crypto::HselInit init;
        init.iEncryptType = mxh::crypto::HSEL_ENCRYPTTYPE_RAND;
        init.iDesCount = mxh::crypto::HSEL_DES_TRIPLE;
        init.iCustomize = mxh::crypto::HSEL_KEY_TYPE_DEFAULT;
        init.iSwapFlag = mxh::crypto::HSEL_SWAP_FLAG_ON;
        return init;
    }
}
bool MurimNetCrypt::Create() {
    auto init = make_default_init();
    if (m_hEnStream.initial(init) == 0) return false;
    if (m_hDeStream.initial(init) == 0) return false;
    m_bInited = true;
    return true;
}
bool MurimNetCrypt::Init(const mxh::crypto::HselInit& en_init, const mxh::crypto::HselInit& de_init) {
    // Legacy semantics: server dekey is client enkey and vice versa.
    // server de -> en: encrypt stream takes the de-init.
    if (m_hEnStream.initial(de_init) == 0) return false;
    // server en -> de: decrypt stream takes the en-init.
    if (m_hDeStream.initial(en_init) == 0) return false;
    m_bInited = true;
    return true;
}
bool MurimNetCrypt::Encrypt(char* buf, int32_t size) {
    if (!m_bInited) return true;  // match legacy: no-op when not initialized.
    return m_hEnStream.encrypt(buf, size);
}
bool MurimNetCrypt::Decrypt(char* buf, int32_t size) {
    if (!m_bInited) return true;
    return m_hDeStream.decrypt(buf, size);
}
std::int8_t MurimNetCrypt::GetEnCrcChar() const {
    return m_hEnStream.get_crc_char();
}
std::int8_t MurimNetCrypt::GetDeCrcChar() const {
    return m_hDeStream.get_crc_char();
}
}
