// hsel_class.cpp - Implementation of CHSEL/CHSEL_STREAM wrappers.

#include "mxh/crypto/hsel_class.hpp"

namespace mxh::crypto {

// ---- CHSEL_STREAM -------------------------------------------------------

CHSEL_STREAM::CHSEL_STREAM(void) {}

CHSEL_STREAM::~CHSEL_STREAM(void) {}

std::int32_t CHSEL_STREAM::Initial(HselInit hselinit) {
    return m_stream.initial(hselinit);
}

HselInit CHSEL_STREAM::GetHSELCustomizeOption(void) const {
    return m_stream.hsel_init();
}

HselKey CHSEL_STREAM::GetNowHSELKey(void) const {
    return m_stream.now_key();
}

// ---- CHSEL overrides ---------------------------------------------------

std::int32_t CHSEL_STREAM::GetVersion(void) {
    return HselStream::kVersion;
}

std::int32_t CHSEL_STREAM::GetHSELType(void) {
    return m_stream.hsel_type();
}

bool CHSEL_STREAM::Encrypt(char* lpStream, const std::int32_t iStreamSize) {
    // Legacy semantics: iStreamSize == 0 means "use internal size",
    // but HselStream requires an explicit positive size. The legacy
    // CHSEL_STREAM also rejected size <= 0 (CheckFaultStreamSize).
    // Returning false for size == 0 preserves legacy behavior.
    if (iStreamSize <= 0) return false;
    return m_stream.encrypt(lpStream, iStreamSize);
}

bool CHSEL_STREAM::Decrypt(char* lpStream, const std::int32_t iStreamSize) {
    if (iStreamSize <= 0) return false;
    return m_stream.decrypt(lpStream, iStreamSize);
}

char CHSEL_STREAM::GetCRCConvertChar(void) {
    return m_stream.get_crc_char();
}

short CHSEL_STREAM::GetCRCConvertShort(void) {
    return m_stream.get_crc_short();
}

std::int32_t CHSEL_STREAM::GetCRCConvertInt(void) {
    return m_stream.get_crc_int();
}

void CHSEL_STREAM::SetKeyCustom(HselKey IntoKey) {
    m_stream.set_key_custom(IntoKey);
}

void CHSEL_STREAM::SetNextKey(void) {
    m_stream.set_next_key();
}

void CHSEL_STREAM::GenerateKeys(HselKey& IntoKey) {
    m_stream.generate_keys(IntoKey);
}

}  // namespace mxh::crypto