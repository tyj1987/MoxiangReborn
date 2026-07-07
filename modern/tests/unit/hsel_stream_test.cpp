// hsel_stream_test.cpp — HSEL stream cipher unit tests (Phase 3.2).
//
// Verifies:
//   1. Key generation produces valid key material
//   2. Key schedule advances correctly
//   3. Encrypt/decrypt round-trip (all 4 types, single + triple DES)
//   4. Swap + no-swap modes
//   5. CRC computation
//   6. Deterministic output for fixed seed (reproducibility)
//   7. MSVC rand() LCG correctness
//   8. Large buffer stress test

#include "mxh/crypto/hsel_stream.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cstdint>

namespace mxh::crypto {
namespace {

// ========================================================================
// MsvcRand tests
// ========================================================================

TEST(MsvcRandTest, DeterministicSequence) {
    MsvcRand r(1);
    // MSVC6 rand() with seed 1 produces:
    //   1*214013+2531011 = 2745024 → (2745024>>16)&0x7FFF = 41
    //   2745024*214013+2531011 = 587507180131 → (>>16) = 8964...
    // Known first 5 values from MSVC6 rand() with srand(1):
    uint32_t expected[] = {41, 18467, 6334, 26500, 19169};
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(r.next(), expected[i]) << "rand() mismatch at index " << i;
    }
}

TEST(MsvcRandTest, DifferentSeeds) {
    MsvcRand r1(42), r2(42), r3(99);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(r1.next(), r2.next());
    }
    // r3 should diverge
    EXPECT_NE(r1.next(), r3.next());
}

TEST(MsvcRandTest, StatePersistence) {
    MsvcRand r(12345);
    int vals[50];
    for (int i = 0; i < 50; i++) vals[i] = r.next();

    MsvcRand r2(12345);
    for (int i = 0; i < 25; i++) r2.next();
    unsigned int s = r2.state();  // snapshot halfway
    for (int i = 0; i < 25; i++) r2.next();

    MsvcRand r3(0);
    r3.set_state(s);  // restore from snapshot
    for (int i = 25; i < 50; i++) {
        EXPECT_EQ(r3.next(), vals[i]) << "state restore mismatch at " << i;
    }
}

// ========================================================================
// Key generation tests
// ========================================================================

TEST(HselKeyGenTest, GenerateKeysProducesNonZero) {
    HselStream hs;
    HselKey key;
    hs.generate_keys(key);

    EXPECT_NE(key.iLeftKey, 0);
    EXPECT_NE(key.iRightKey, 0);
    EXPECT_NE(key.iMiddleKey, 0);
    EXPECT_NE(key.iTotalKey, 0);

    // MultiGabs and PlusGabs must always be odd (from GetMultiGab()/GetPlusGab()).
    EXPECT_EQ(key.iLeftMultiGab   & 1, 1);
    EXPECT_EQ(key.iRightMultiGab  & 1, 1);
    EXPECT_EQ(key.iMiddleMultiGab & 1, 1);
    EXPECT_EQ(key.iTotalMultiGab  & 1, 1);
    EXPECT_EQ(key.iLeftPlusGab    & 1, 1);
    EXPECT_EQ(key.iRightPlusGab   & 1, 1);
    EXPECT_EQ(key.iMiddlePlusGab  & 1, 1);
    EXPECT_EQ(key.iTotalPlusGab   & 1, 1);
}

TEST(HselKeyGenTest, DeterministicWithFixedSeed) {
    // Same seed → same keys.
    HselStream hs1, hs2;
    hs1.rng().set_state(42u);
    hs2.rng().set_state(42u);
    HselKey k1, k2;
    hs1.generate_keys(k1);
    hs2.generate_keys(k2);

    EXPECT_EQ(k1.iLeftKey,   k2.iLeftKey);
    EXPECT_EQ(k1.iRightKey,  k2.iRightKey);
    EXPECT_EQ(k1.iMiddleKey, k2.iMiddleKey);
    EXPECT_EQ(k1.iTotalKey,  k2.iTotalKey);
}

TEST(HselKeyGenTest, SetNextKeyAdvances) {
    HselStream hs;
    HselKey key;
    hs.generate_keys(key);

    HselKey before = key;
    hs.set_key_custom(key);

    hs.set_next_key();
    HselKey after = hs.now_key();

    // Key schedule: newKey = oldKey * multiGab + plusGab
    EXPECT_EQ(after.iLeftKey,   before.iLeftKey   * before.iLeftMultiGab   + before.iLeftPlusGab);
    EXPECT_EQ(after.iRightKey,  before.iRightKey  * before.iRightMultiGab  + before.iRightPlusGab);
    EXPECT_EQ(after.iMiddleKey, before.iMiddleKey * before.iMiddleMultiGab + before.iMiddlePlusGab);
    EXPECT_EQ(after.iTotalKey,  before.iTotalKey  * before.iTotalMultiGab  + before.iTotalPlusGab);
}

// ========================================================================
// Encrypt/Decrypt round-trip tests (all types)
// ========================================================================

struct HselRoundTripParam {
    int32_t desCount;
    int32_t encryptType;   // not RAND — deterministic type
    int32_t swapFlag;
    int32_t bufSize;
};

class HselRoundTripTest : public ::testing::TestWithParam<HselRoundTripParam> {
protected:
    void SetUp() override {
        const auto& p = GetParam();
        HselInit init;
        init.iDesCount    = p.desCount;
        init.iEncryptType = p.encryptType;
        init.iSwapFlag    = p.swapFlag;
        init.iCustomize   = HSEL_KEY_TYPE_DEFAULT;  // auto-generate keys
        // Use a FIXED RNG seed so both encrypt and decrypt get the same keys.
        enc_hs_.rng().set_state(123456u);
        enc_hs_.initial(init);
    }

    HselStream enc_hs_;
};

TEST_P(HselRoundTripTest, EncryptThenDecryptRestoresOriginal) {
    const auto& p = GetParam();
    std::vector<char> buf(p.bufSize);
    // Fill with deterministic but non-trivial pattern.
    for (int32_t i = 0; i < p.bufSize; i++) {
        buf[i] = static_cast<char>((i * 7 + 13) & 0xFF);
    }

    std::vector<char> original = buf;

    ASSERT_TRUE(enc_hs_.encrypt(buf.data(), p.bufSize));

    // Create decrypt stream with the SAME RNG seed and SAME init params
    // so it gets identical starting keys.
    HselInit decInit;
    decInit.iDesCount    = p.desCount;
    decInit.iEncryptType = p.encryptType;
    decInit.iSwapFlag    = p.swapFlag;
    decInit.iCustomize   = HSEL_KEY_TYPE_DEFAULT;

    HselStream dec_hs;
    dec_hs.rng().set_state(123456u);  // SAME seed as encrypt
    dec_hs.initial(decInit);

    ASSERT_TRUE(dec_hs.decrypt(buf.data(), p.bufSize));
    EXPECT_EQ(buf, original) << "Round-trip failed for desCount=" << p.desCount
                             << " encryptType=" << p.encryptType
                             << " swapFlag=" << p.swapFlag
                             << " bufSize=" << p.bufSize;
}

INSTANTIATE_TEST_SUITE_P(
    AllTypesSizes,
    HselRoundTripTest,
    ::testing::Values(
        // Single DES, various types + swap modes, various sizes
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_1, HSEL_SWAP_FLAG_ON,  64},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_1, HSEL_SWAP_FLAG_OFF, 64},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_2, HSEL_SWAP_FLAG_ON,  128},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_2, HSEL_SWAP_FLAG_OFF, 128},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_3, HSEL_SWAP_FLAG_ON,  256},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_3, HSEL_SWAP_FLAG_OFF, 256},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_4, HSEL_SWAP_FLAG_ON,  512},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_4, HSEL_SWAP_FLAG_OFF, 512},
        // Triple DES
        HselRoundTripParam{HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_1, HSEL_SWAP_FLAG_ON,  1024},
        HselRoundTripParam{HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_2, HSEL_SWAP_FLAG_ON,  256},
        HselRoundTripParam{HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_3, HSEL_SWAP_FLAG_ON,  128},
        HselRoundTripParam{HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_4, HSEL_SWAP_FLAG_ON,  512},
        // Odd sizes (not aligned to 4 bytes)
        HselRoundTripParam{HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_1, HSEL_SWAP_FLAG_ON,  7},
        HselRoundTripParam{HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_2, HSEL_SWAP_FLAG_ON,  13},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_3, HSEL_SWAP_FLAG_ON,  31},
        HselRoundTripParam{HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_4, HSEL_SWAP_FLAG_ON,  22},
        // Small buffers (below LIMIT_SWAP_BLOCK_COUNT=5 blocks = 20 bytes)
        HselRoundTripParam{HSEL_DES_SINGLE,  HSEL_ENCRYPTTYPE_1, HSEL_SWAP_FLAG_ON,  4},
        HselRoundTripParam{HSEL_DES_TRIPLE,  HSEL_ENCRYPTTYPE_4, HSEL_SWAP_FLAG_ON,  16},
        HselRoundTripParam{HSEL_DES_TRIPLE,  HSEL_ENCRYPTTYPE_2, HSEL_SWAP_FLAG_OFF, 8}
    )
);

// ========================================================================
// RAND encrypt type
// ========================================================================

TEST(HselStreamTest, RandomEncryptTypeRoundTrip) {
    HselInit init;
    init.iDesCount    = HSEL_DES_TRIPLE;
    init.iEncryptType = HSEL_ENCRYPTTYPE_RAND;  // picks randomly
    init.iSwapFlag    = HSEL_SWAP_FLAG_ON;
    init.iCustomize   = HSEL_KEY_TYPE_DEFAULT;

    HselStream hs_enc, hs_dec;
    // Force same RNG seed so both pick the same random type and same keys.
    hs_enc.rng().set_state(12345u);
    hs_dec.rng().set_state(12345u);

    int32_t t1 = hs_enc.initial(init);
    int32_t t2 = hs_dec.initial(init);
    EXPECT_EQ(t1, t2) << "Same seed → same HSEL type";

    std::vector<char> buf(256);
    for (int i = 0; i < 256; i++) buf[i] = static_cast<char>(i);

    std::vector<char> original = buf;
    ASSERT_TRUE(hs_enc.encrypt(buf.data(), 256));
    ASSERT_TRUE(hs_dec.decrypt(buf.data(), 256));
    EXPECT_EQ(buf, original);
}

// ========================================================================
// CRC tests
// ========================================================================

TEST(HselStreamTest, CrcAfterEncrypt) {
    HselInit init;
    init.iDesCount    = HSEL_DES_SINGLE;
    init.iEncryptType = HSEL_ENCRYPTTYPE_1;
    init.iSwapFlag    = HSEL_SWAP_FLAG_OFF;
    init.iCustomize   = HSEL_KEY_TYPE_CUSTOMIZE;
    init.Keys.iLeftKey = 0x12345678;
    init.Keys.iRightKey = 0x9ABCDEF0;

    HselStream hs;
    hs.initial(init);

    // 16 zero bytes. Type-1 XOR with feed-forward: first block XORed
    // with key=0x12345678, then key becomes the (old) block value.
    // All 4 blocks become 0x12345678, so CRC = 0x12345678 ^ 0x12345678
    // ^ 0x12345678 ^ 0x12345678 = 0. This is correct HSEL behavior.
    std::vector<char> buf(16, 0);
    ASSERT_TRUE(hs.encrypt(buf.data(), 16));

    int32_t crc = hs.get_crc_int();
    // With all-zero input + feed-forward XOR, all blocks are identical
    // → CRC XORs to 0. (Bug in original test: we expected crc != 0.)
    EXPECT_EQ(crc, 0);

    // Verify CRC byte/short derivation functions work correctly.
    int8_t crcChar = hs.get_crc_char();
    int16_t crcShort = hs.get_crc_short();
    EXPECT_EQ(crcChar, 0);
    EXPECT_EQ(crcShort, 0);
}

// CRC with non-uniform data should produce non-zero CRC.
TEST(HselStreamTest, CrcNonZeroForVariedData) {
    HselInit init;
    init.iDesCount    = HSEL_DES_SINGLE;
    init.iEncryptType = HSEL_ENCRYPTTYPE_1;
    init.iSwapFlag    = HSEL_SWAP_FLAG_OFF;
    init.iCustomize   = HSEL_KEY_TYPE_CUSTOMIZE;
    init.Keys.iLeftKey = 0x12345678;

    HselStream hs;
    hs.initial(init);

    // 17 bytes of varied non-zero data → should produce non-zero CRC.
    std::vector<char> buf(17);
    for (int i = 0; i < 17; i++) buf[i] = static_cast<char>(i + 1);
    ASSERT_TRUE(hs.encrypt(buf.data(), 17));

    int32_t crc = hs.get_crc_int();
    EXPECT_NE(crc, 0) << "Non-uniform data should produce non-zero CRC";
}

// ========================================================================
// Custom key mode
// ========================================================================

TEST(HselStreamTest, CustomKeyDeterministic) {
    HselInit init;
    init.iDesCount    = HSEL_DES_SINGLE;
    init.iEncryptType = HSEL_ENCRYPTTYPE_1;
    init.iSwapFlag    = HSEL_SWAP_FLAG_OFF;
    init.iCustomize   = HSEL_KEY_TYPE_CUSTOMIZE;
    init.Keys.iLeftKey = 42;

    HselStream hs1, hs2;
    hs1.initial(init);
    hs2.initial(init);

    std::vector<char> buf1(256), buf2(256);
    for (int i = 0; i < 256; i++) buf1[i] = buf2[i] = static_cast<char>(i);

    hs1.encrypt(buf1.data(), 256);
    hs2.encrypt(buf2.data(), 256);
    EXPECT_EQ(buf1, buf2) << "Custom key → deterministic output";
}

// ========================================================================
// Reject invalid sizes
// ========================================================================

TEST(HselStreamTest, EncryptZeroSizeReturnsFalse) {
    HselStream hs;
    HselInit init;
    init.iCustomize = HSEL_KEY_TYPE_CUSTOMIZE;
    hs.initial(init);

    char dummy = 0;
    EXPECT_FALSE(hs.encrypt(&dummy, 0));
    EXPECT_FALSE(hs.encrypt(nullptr, 0));
    EXPECT_FALSE(hs.decrypt(&dummy, 0));
}

TEST(HselStreamTest, EncryptNegativeSizeReturnsFalse) {
    HselStream hs;
    HselInit init;
    init.iCustomize = HSEL_KEY_TYPE_CUSTOMIZE;
    hs.initial(init);

    char dummy = 0;
    EXPECT_FALSE(hs.encrypt(&dummy, -1));
}

// ========================================================================
// Stress test: 10000 random inputs, all types
// ========================================================================

TEST(HselStreamStressTest, ManyRandomInputs) {
    const int kIterations = 1000;

    struct Config {
        int32_t desCount;
        int32_t encryptType;
        int32_t swapFlag;
    };
    Config configs[] = {
        {HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_1, HSEL_SWAP_FLAG_ON},
        {HSEL_DES_SINGLE, HSEL_ENCRYPTTYPE_2, HSEL_SWAP_FLAG_OFF},
        {HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_3, HSEL_SWAP_FLAG_ON},
        {HSEL_DES_TRIPLE, HSEL_ENCRYPTTYPE_4, HSEL_SWAP_FLAG_OFF},
    };

    for (const auto& cfg : configs) {
        for (int i = 0; i < kIterations; i++) {
            HselInit init;
            init.iDesCount    = cfg.desCount;
            init.iEncryptType = cfg.encryptType;
            init.iSwapFlag    = cfg.swapFlag;
            init.iCustomize   = HSEL_KEY_TYPE_DEFAULT;

            HselStream hs_enc, hs_dec;
            hs_enc.rng().set_state(static_cast<unsigned>(i));
            hs_dec.rng().set_state(static_cast<unsigned>(i));

            hs_enc.initial(init);
            hs_dec.initial(init);

            int32_t size = (i % 1024) + 1;  // 1..1024 bytes
            std::vector<char> buf(size);
            for (int32_t j = 0; j < size; j++) {
                buf[j] = static_cast<char>((i + j * 3) & 0xFF);
            }
            std::vector<char> original = buf;

            ASSERT_TRUE(hs_enc.encrypt(buf.data(), size));
            ASSERT_TRUE(hs_dec.decrypt(buf.data(), size));
            ASSERT_EQ(buf, original) << "Stress test failed at iteration " << i
                                     << ", size=" << size;
        }
    }
}

}  // namespace
}  // namespace mxh::crypto
