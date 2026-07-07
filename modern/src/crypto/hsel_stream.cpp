// hsel_stream.cpp — C++17 HSEL stream cipher implementation.
//
// Byte-identical to the legacy CHSEL_STREAM from HSEL_STREAM.cpp.
// Key generation uses an MSVC6-compatible LCG PRNG (MsvcRand) so
// the same seed produces the same key material.
//
// Reference: 墨香【源码】\[Lib]HSEL\HSEL_STREAM.cpp

#include "mxh/crypto/hsel_stream.hpp"
#include <algorithm>   // std::swap
#include <cstring>     // std::memcpy

namespace mxh::crypto {

// --- internal helpers ----------------------------------------------------

namespace {

// XOR-swap for int32_t (HSELSWAP macro in original).
inline void hsel_swap(int32_t& a, int32_t& b) {
    a ^= b;
    b ^= a;
    a ^= b;
}

// Key-generation constants (from HSEL_STREAM.cpp:11-14).
// GetKey()    = (rand()%34000+10256) * (rand()%30000+10256) + 5
// GetMultiGab() = ((rand()%15000+512) * (rand()%10000+256)) * 2 + 1  (always odd)
// GetPlusGab()  = ((rand()%5000+512)  * (rand()%1000+256))  * 2 + 1  (always odd)

inline int32_t gen_key(MsvcRand& r) {
    return (r.next() % 34000 + 10256) * (r.next() % 30000 + 10256) + 5;
}
inline int32_t gen_multi_gab(MsvcRand& r) {
    return ((r.next() % 15000 + 512) * (r.next() % 10000 + 256)) * 2 + 1;
}
inline int32_t gen_plus_gab(MsvcRand& r) {
    return ((r.next() % 5000 + 512) * (r.next() % 1000 + 256)) * 2 + 1;
}

// Key schedule: newKey = oldKey * multiGab + plusGab
// GetNextKey(A,B,C) = ((A)*(B)) + (C)
inline int32_t next_key(int32_t key, int32_t multi, int32_t plus) {
    return key * multi + plus;
}

}  // anonymous namespace

// --- HselStream -----------------------------------------------------------

HselStream::HselStream() {
    // m_rng starts with seed=1; the legacy code calls srand(timeGetTime())
    // in the constructor, which we don't replicate by default. Callers
    // who need the legacy seeding behavior should call rng().set_state(seed).
}

HselStream::~HselStream() = default;

int32_t HselStream::initial(const HselInit& init) {
    HselInit working = init;
    m_hselType = 0;

    // --- DES count (Single vs Triple) ---
    switch (working.iDesCount & 0x000F) {
    case HSEL_DES_SINGLE:
        m_desEncryptType = &HselStream::des_single_encode;
        m_desDecryptType = &HselStream::des_single_decode;
        break;
    case HSEL_DES_TRIPLE:
        m_desEncryptType = &HselStream::des_triple_encode;
        m_desDecryptType = &HselStream::des_triple_decode;
        break;
    default:
        return 0;
    }

    // --- Encrypt type (1-4, RAND picks one randomly) ---
    if (working.iEncryptType == HSEL_ENCRYPTTYPE_RAND) {
        switch (m_rng.next() % 4) {
        case 0:  working.iEncryptType = HSEL_ENCRYPTTYPE_1; break;
        case 1:  working.iEncryptType = HSEL_ENCRYPTTYPE_2; break;
        case 2:  working.iEncryptType = HSEL_ENCRYPTTYPE_3; break;
        default: working.iEncryptType = HSEL_ENCRYPTTYPE_4; break;
        }
    }

    switch (working.iEncryptType & 0x00F0) {
    case HSEL_ENCRYPTTYPE_1:
        m_desLeftEncrypt   = &HselStream::des_left_encode_type1;
        m_desRightEncrypt  = &HselStream::des_right_encode_type1;
        m_desMiddleEncrypt = &HselStream::des_middle_encode_type1;
        m_desLeftDecrypt   = &HselStream::des_left_decode_type1;
        m_desRightDecrypt  = &HselStream::des_right_decode_type1;
        m_desMiddleDecrypt = &HselStream::des_middle_decode_type1;
        break;
    case HSEL_ENCRYPTTYPE_2:
        m_desLeftEncrypt   = &HselStream::des_left_encode_type2;
        m_desRightEncrypt  = &HselStream::des_right_encode_type2;
        m_desMiddleEncrypt = &HselStream::des_middle_encode_type2;
        m_desLeftDecrypt   = &HselStream::des_left_decode_type2;
        m_desRightDecrypt  = &HselStream::des_right_decode_type2;
        m_desMiddleDecrypt = &HselStream::des_middle_decode_type2;
        break;
    case HSEL_ENCRYPTTYPE_3:
        m_desLeftEncrypt   = &HselStream::des_left_encode_type3;
        m_desRightEncrypt  = &HselStream::des_right_encode_type3;
        m_desMiddleEncrypt = &HselStream::des_middle_encode_type3;
        m_desLeftDecrypt   = &HselStream::des_left_decode_type3;
        m_desRightDecrypt  = &HselStream::des_right_decode_type3;
        m_desMiddleDecrypt = &HselStream::des_middle_decode_type3;
        break;
    case HSEL_ENCRYPTTYPE_4:
        m_desLeftEncrypt   = &HselStream::des_left_encode_type4;
        m_desRightEncrypt  = &HselStream::des_right_encode_type4;
        m_desMiddleEncrypt = &HselStream::des_middle_encode_type4;
        m_desLeftDecrypt   = &HselStream::des_left_decode_type4;
        m_desRightDecrypt  = &HselStream::des_right_decode_type4;
        m_desMiddleDecrypt = &HselStream::des_middle_decode_type4;
        break;
    default:
        return 0;
    }

    // --- Swap flag ---
    switch (working.iSwapFlag & 0x0F00) {
    case HSEL_SWAP_FLAG_ON:
        m_swapEncrypt = &HselStream::swap_encrypt;
        m_swapDecrypt = &HselStream::swap_decrypt;
        break;
    case HSEL_SWAP_FLAG_OFF:
        m_swapEncrypt = nullptr;     // no-op
        m_swapDecrypt = nullptr;     // no-op
        break;
    default:
        return 0;
    }

    m_init = working;

    // --- Key initialization ---
    switch (working.iCustomize & 0xF000) {
    case HSEL_KEY_TYPE_CUSTOMIZE:
        set_key_custom(working.Keys);
        break;
    case HSEL_KEY_TYPE_DEFAULT:
        generate_keys(m_init.Keys);
        break;
    default:
        return 0;
    }

    m_init.iCustomize = HSEL_KEY_TYPE_CUSTOMIZE;
    m_hselType = (working.iDesCount | working.iEncryptType |
                  working.iSwapFlag | working.iCustomize);
    return m_hselType;
}

void HselStream::set_key_custom(const HselKey& key) {
    m_init.Keys = key;
    m_crcValue = 0;
}

void HselStream::generate_keys(HselKey& key) {
    key.iLeftKey    = gen_key(m_rng);
    key.iRightKey   = gen_key(m_rng);
    key.iMiddleKey  = gen_key(m_rng);
    key.iTotalKey   = gen_key(m_rng);

    key.iLeftMultiGab   = gen_multi_gab(m_rng);
    key.iRightMultiGab  = gen_multi_gab(m_rng);
    key.iMiddleMultiGab = gen_multi_gab(m_rng);
    key.iTotalMultiGab  = gen_multi_gab(m_rng);

    key.iLeftPlusGab    = gen_plus_gab(m_rng);
    key.iRightPlusGab   = gen_plus_gab(m_rng);
    key.iMiddlePlusGab  = gen_plus_gab(m_rng);
    key.iTotalPlusGab   = gen_plus_gab(m_rng);
}

void HselStream::set_next_key() {
    m_init.Keys.iLeftKey   = next_key(m_init.Keys.iLeftKey,
                                       m_init.Keys.iLeftMultiGab,
                                       m_init.Keys.iLeftPlusGab);
    m_init.Keys.iRightKey  = next_key(m_init.Keys.iRightKey,
                                       m_init.Keys.iRightMultiGab,
                                       m_init.Keys.iRightPlusGab);
    m_init.Keys.iMiddleKey = next_key(m_init.Keys.iMiddleKey,
                                       m_init.Keys.iMiddleMultiGab,
                                       m_init.Keys.iMiddlePlusGab);
    m_init.Keys.iTotalKey  = next_key(m_init.Keys.iTotalKey,
                                       m_init.Keys.iTotalMultiGab,
                                       m_init.Keys.iTotalPlusGab);
}

bool HselStream::check_stream_size(int32_t size) const {
    return size > 0;
}

// --- Encrypt / Decrypt ----------------------------------------------------

bool HselStream::encrypt(char* stream, int32_t size) {
    if (!check_stream_size(size)) return false;

    if (m_swapEncrypt)
        (this->*m_swapEncrypt)(stream, size);

    (this->*m_desEncryptType)(stream, size);

    set_next_key();
    compute_crc(stream, size);
    return true;
}

bool HselStream::decrypt(char* stream, int32_t size) {
    if (!check_stream_size(size)) return false;

    compute_crc(stream, size);

    (this->*m_desDecryptType)(stream, size);

    if (m_swapDecrypt)
        (this->*m_swapDecrypt)(stream, size);

    set_next_key();
    return true;
}

// --- CRC ------------------------------------------------------------------

void HselStream::compute_crc(const char* stream, int32_t size) {
    m_crcValue    = 0;
    m_blockCount  = size >> 2;       // size / 4
    m_remainCount = size & 3;        // size % 4
    m_pos         = m_blockCount << 2;  // blockCount * 4

    const int32_t* block = reinterpret_cast<const int32_t*>(stream);
    int32_t n = m_blockCount;
    while (n) {
        m_crcValue ^= *(block++);
        n--;
    }
    int32_t p = m_pos;
    while (m_remainCount) {
        m_crcValue ^= static_cast<uint8_t>(stream[p++]);
        m_remainCount--;
    }
}

int8_t HselStream::get_crc_char() const {
    const auto* c = reinterpret_cast<const int8_t*>(&m_crcValue);
    return static_cast<int8_t>(c[0] ^ c[1] ^ c[2] ^ c[3]);
}

int16_t HselStream::get_crc_short() const {
    const auto* s = reinterpret_cast<const int16_t*>(&m_crcValue);
    return static_cast<int16_t>(s[0] ^ s[1]);
}

int32_t HselStream::get_crc_int() const {
    return m_crcValue;
}

// --- DES dispatchers ------------------------------------------------------

void HselStream::des_single_encode(char* stream, int32_t size) {
    (this->*m_desLeftEncrypt)(stream, size);
}
void HselStream::des_single_decode(char* stream, int32_t size) {
    (this->*m_desLeftDecrypt)(stream, size);
}
void HselStream::des_triple_encode(char* stream, int32_t size) {
    (this->*m_desLeftEncrypt)(stream, size);
    (this->*m_desRightEncrypt)(stream, size);
    (this->*m_desMiddleEncrypt)(stream, size);
}
void HselStream::des_triple_decode(char* stream, int32_t size) {
    (this->*m_desMiddleDecrypt)(stream, size);
    (this->*m_desRightDecrypt)(stream, size);
    (this->*m_desLeftDecrypt)(stream, size);
}

// --- Swap -----------------------------------------------------------------

void HselStream::swap_encrypt(char* stream, int32_t size) {
    m_blockCount = size >> 2;
    if (kLimitSwapBlockCount > m_blockCount) return;
    m_blockCount--;

    const HselKey& keys = m_init.Keys;
    int32_t bc = m_blockCount;

    m_lPos[0] = (keys.iLeftKey   & 0x000F)       % bc;
    m_lPos[1] = ((keys.iLeftKey  >> 8)  & 0x000F) % bc;
    m_lPos[2] = ((keys.iLeftKey  >> 16) & 0x000F) % bc;
    m_lPos[3] = ((keys.iLeftKey  >> 24) & 0x000F) % bc;

    m_rPos[0] = (keys.iRightKey   & 0x000F)       % bc;
    m_rPos[1] = ((keys.iRightKey  >> 8)  & 0x000F) % bc;
    m_rPos[2] = ((keys.iRightKey  >> 16) & 0x000F) % bc;
    m_rPos[3] = ((keys.iRightKey  >> 24) & 0x000F) % bc;

    m_mPos[0] = (keys.iMiddleKey   & 0x000F)       % bc;
    m_mPos[1] = ((keys.iMiddleKey  >> 8)  & 0x000F) % bc;
    m_mPos[2] = ((keys.iMiddleKey  >> 16) & 0x000F) % bc;
    m_mPos[3] = ((keys.iMiddleKey  >> 24) & 0x000F) % bc;

    int32_t* block = reinterpret_cast<int32_t*>(stream);

    // Encrypt swap order: L↔R then R↔M
    if (m_lPos[0] != m_rPos[0]) hsel_swap(block[m_lPos[0]], block[m_rPos[0]]);
    if (m_rPos[0] != m_mPos[0]) hsel_swap(block[m_rPos[0]], block[m_mPos[0]]);

    if (m_lPos[1] != m_rPos[1]) hsel_swap(block[m_lPos[1]], block[m_rPos[1]]);
    if (m_rPos[1] != m_mPos[1]) hsel_swap(block[m_rPos[1]], block[m_mPos[1]]);

    if (m_lPos[2] != m_rPos[2]) hsel_swap(block[m_lPos[2]], block[m_rPos[2]]);
    if (m_rPos[2] != m_mPos[2]) hsel_swap(block[m_rPos[2]], block[m_mPos[2]]);

    if (m_lPos[3] != m_rPos[3]) hsel_swap(block[m_lPos[3]], block[m_rPos[3]]);
    if (m_rPos[3] != m_mPos[3]) hsel_swap(block[m_rPos[3]], block[m_mPos[3]]);
}

void HselStream::swap_decrypt(char* stream, int32_t size) {
    m_blockCount = size >> 2;
    if (kLimitSwapBlockCount > m_blockCount) return;
    m_blockCount--;

    const HselKey& keys = m_init.Keys;
    int32_t bc = m_blockCount;

    m_lPos[0] = (keys.iLeftKey   & 0x000F)       % bc;
    m_lPos[1] = ((keys.iLeftKey  >> 8)  & 0x000F) % bc;
    m_lPos[2] = ((keys.iLeftKey  >> 16) & 0x000F) % bc;
    m_lPos[3] = ((keys.iLeftKey  >> 24) & 0x000F) % bc;

    m_rPos[0] = (keys.iRightKey   & 0x000F)       % bc;
    m_rPos[1] = ((keys.iRightKey  >> 8)  & 0x000F) % bc;
    m_rPos[2] = ((keys.iRightKey  >> 16) & 0x000F) % bc;
    m_rPos[3] = ((keys.iRightKey  >> 24) & 0x000F) % bc;

    m_mPos[0] = (keys.iMiddleKey   & 0x000F)       % bc;
    m_mPos[1] = ((keys.iMiddleKey  >> 8)  & 0x000F) % bc;
    m_mPos[2] = ((keys.iMiddleKey  >> 16) & 0x000F) % bc;
    m_mPos[3] = ((keys.iMiddleKey  >> 24) & 0x000F) % bc;

    int32_t* block = reinterpret_cast<int32_t*>(stream);

    // Decrypt swap order: R↔M then L↔R (REVERSED from encrypt)
    if (m_rPos[3] != m_mPos[3]) hsel_swap(block[m_rPos[3]], block[m_mPos[3]]);
    if (m_lPos[3] != m_rPos[3]) hsel_swap(block[m_lPos[3]], block[m_rPos[3]]);

    if (m_rPos[2] != m_mPos[2]) hsel_swap(block[m_rPos[2]], block[m_mPos[2]]);
    if (m_lPos[2] != m_rPos[2]) hsel_swap(block[m_lPos[2]], block[m_rPos[2]]);

    if (m_rPos[1] != m_mPos[1]) hsel_swap(block[m_rPos[1]], block[m_mPos[1]]);
    if (m_lPos[1] != m_rPos[1]) hsel_swap(block[m_lPos[1]], block[m_rPos[1]]);

    if (m_rPos[0] != m_mPos[0]) hsel_swap(block[m_rPos[0]], block[m_mPos[0]]);
    if (m_lPos[0] != m_rPos[0]) hsel_swap(block[m_lPos[0]], block[m_rPos[0]]);
}

// ======================================================================
// Type 1: XOR operations
// ======================================================================

void HselStream::des_left_encode_type1(char* stream, int32_t size) {
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempLeftKey;

    while (n) {
        *block ^= tk;
        tk = *(block++);
        n--;
    }
    m_tempLeftKey = tk;

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] ^= keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_left_decode_type1(char* stream, int32_t size) {
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);

    block += (m_blockCount - 1);
    int32_t tk = (m_blockCount >= 2) ? *(block - 1) : 0;
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block--) ^= tk;
        tk = *(block - 1);
        n--;
    }
    if (n) {
        *block ^= m_init.Keys.iLeftKey;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] ^= keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_right_encode_type1(char* stream, int32_t size) {
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + size - 4);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempRightKey;

    while (n) {
        *block ^= tk;
        tk = *(block--);
        n--;
    }
    m_tempRightKey = tk;

    while (m_remainCount) {
        stream[--m_remainCount] ^= keyChars[m_remainCount];
    }
}

void HselStream::des_right_decode_type1(char* stream, int32_t size) {
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + m_remainCount);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);

    int32_t tk = *(block + 1);
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block++) ^= tk;
        tk = *(block + 1);
        n--;
    }
    if (n) {
        *block ^= m_init.Keys.iRightKey;
    }

    while (m_remainCount) {
        stream[--m_remainCount] ^= keyChars[m_remainCount];
    }
}

void HselStream::des_middle_encode_type1(char* stream, int32_t size) {
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) ^= tk;
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] ^= keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_middle_decode_type1(char* stream, int32_t size) {
    // Middle decode type 1 is IDENTICAL to encode (XOR is self-inverse).
    des_middle_encode_type1(stream, size);
}

// ======================================================================
// Type 2: ADD operations
// ======================================================================

void HselStream::des_left_encode_type2(char* stream, int32_t size) {
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempLeftKey;

    while (n) {
        *block += tk;
        tk = *(block++);
        n--;
    }
    m_tempLeftKey = tk;

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] += keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_left_decode_type2(char* stream, int32_t size) {
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);

    block += (m_blockCount - 1);
    int32_t tk = (m_blockCount >= 2) ? *(block - 1) : 0;
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block--) -= tk;
        tk = *(block - 1);
        n--;
    }
    if (n) {
        *block -= m_init.Keys.iLeftKey;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] -= keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_right_encode_type2(char* stream, int32_t size) {
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + size - 4);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempRightKey;

    while (n) {
        *block += tk;
        tk = *(block--);
        n--;
    }
    m_tempRightKey = tk;

    while (m_remainCount) {
        stream[--m_remainCount] += keyChars[m_remainCount];
    }
}

void HselStream::des_right_decode_type2(char* stream, int32_t size) {
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + m_remainCount);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);

    int32_t tk = *(block + 1);
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block++) -= tk;
        tk = *(block + 1);
        n--;
    }
    if (n) {
        *block -= m_init.Keys.iRightKey;
    }

    while (m_remainCount) {
        stream[--m_remainCount] -= keyChars[m_remainCount];
    }
}

void HselStream::des_middle_encode_type2(char* stream, int32_t size) {
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) += tk;
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] += keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_middle_decode_type2(char* stream, int32_t size) {
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) -= tk;
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] -= keyChars[m_remainCount];
        m_remainCount--;
    }
}

// ======================================================================
// Type 3: SUB operations
// ======================================================================

void HselStream::des_left_encode_type3(char* stream, int32_t size) {
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempLeftKey;

    while (n) {
        *block -= tk;
        tk = *(block++);
        n--;
    }
    m_tempLeftKey = tk;

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] -= keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_left_decode_type3(char* stream, int32_t size) {
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);

    block += (m_blockCount - 1);
    int32_t tk = (m_blockCount >= 2) ? *(block - 1) : 0;
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block--) += tk;
        tk = *(block - 1);
        n--;
    }
    if (n) {
        *block += m_init.Keys.iLeftKey;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] += keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_right_encode_type3(char* stream, int32_t size) {
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + size - 4);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempRightKey;

    while (n) {
        *block -= tk;
        tk = *(block--);
        n--;
    }
    m_tempRightKey = tk;

    while (m_remainCount) {
        stream[--m_remainCount] -= keyChars[m_remainCount];
    }
}

void HselStream::des_right_decode_type3(char* stream, int32_t size) {
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + m_remainCount);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);

    int32_t tk = *(block + 1);
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block++) += tk;
        tk = *(block + 1);
        n--;
    }
    if (n) {
        *block += m_init.Keys.iRightKey;
    }

    while (m_remainCount) {
        stream[--m_remainCount] += keyChars[m_remainCount];
    }
}

void HselStream::des_middle_encode_type3(char* stream, int32_t size) {
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) -= tk;
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] -= keyChars[m_remainCount];
        m_remainCount--;
    }
}

void HselStream::des_middle_decode_type3(char* stream, int32_t size) {
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) += tk;
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] += keyChars[m_remainCount];
        m_remainCount--;
    }
}

// ======================================================================
// Type 4: MIXED operations
//   Encode: Left=SUB, Right=ADD, Middle=XOR
//   Decode: Left=ADD, Right=SUB, Middle=XOR
// ======================================================================

void HselStream::des_left_encode_type4(char* stream, int32_t size) {
    // Type 4 Left Encode: SUB (same as Type 3 encode)
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempLeftKey;

    while (n) {
        *block -= tk;
        tk = *(block++);
        n--;
    }
    m_tempLeftKey = tk;

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] ^= keyChars[m_remainCount];  // XOR remainder (Type 4 uses ^ for remainder)
        m_remainCount--;
    }
}

void HselStream::des_left_decode_type4(char* stream, int32_t size) {
    // Type 4 Left Decode: ADD (reverse of SUB encode)
    m_tempLeftKey = m_init.Keys.iLeftKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iLeftKey);

    block += (m_blockCount - 1);
    int32_t tk = (m_blockCount >= 2) ? *(block - 1) : 0;
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block--) += tk;
        tk = *(block - 1);
        n--;
    }
    if (n) {
        *block += m_init.Keys.iLeftKey;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] ^= keyChars[m_remainCount];  // XOR remainder
        m_remainCount--;
    }
}

void HselStream::des_right_encode_type4(char* stream, int32_t size) {
    // Type 4 Right Encode: ADD (same as Type 2 encode with XOR remainder)
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + size - 4);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);
    int32_t n = m_blockCount;
    int32_t tk = m_tempRightKey;

    while (n) {
        *block += tk;
        tk = *(block--);
        n--;
    }
    m_tempRightKey = tk;

    while (m_remainCount) {
        stream[--m_remainCount] ^= keyChars[m_remainCount];  // XOR remainder
    }
}

void HselStream::des_right_decode_type4(char* stream, int32_t size) {
    // Type 4 Right Decode: SUB (reverse of ADD encode)
    m_tempRightKey = m_init.Keys.iRightKey;
    m_blockCount   = size >> 2;
    m_remainCount  = size & 3;
    m_pos          = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream + m_remainCount);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iRightKey);

    int32_t tk = *(block + 1);
    int32_t n = m_blockCount;

    while (1 < n) {
        *(block++) -= tk;
        tk = *(block + 1);
        n--;
    }
    if (n) {
        *block -= m_init.Keys.iRightKey;
    }

    while (m_remainCount) {
        stream[--m_remainCount] ^= keyChars[m_remainCount];  // XOR remainder
    }
}

void HselStream::des_middle_encode_type4(char* stream, int32_t size) {
    // Type 4 Middle Encode: XOR blocks + ADD remainder
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) ^= tk;     // XOR on blocks
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] += keyChars[m_remainCount];  // ADD on remainder
        m_remainCount--;
    }
}

void HselStream::des_middle_decode_type4(char* stream, int32_t size) {
    // Type 4 Middle Decode: XOR blocks + SUB remainder
    int32_t tk = m_init.Keys.iMiddleKey;
    m_blockCount  = size >> 2;
    m_remainCount = size & 3;
    m_pos         = m_blockCount << 2;

    int32_t* block = reinterpret_cast<int32_t*>(stream);
    const auto* keyChars = reinterpret_cast<const int8_t*>(&m_init.Keys.iMiddleKey);
    int32_t n = m_blockCount;

    while (n) {
        *(block++) ^= tk;     // XOR on blocks (self-inverse)
        n--;
    }

    int32_t p = m_pos;
    while (m_remainCount) {
        stream[p++] -= keyChars[m_remainCount];  // SUB on remainder
        m_remainCount--;
    }
}

}  // namespace mxh::crypto
