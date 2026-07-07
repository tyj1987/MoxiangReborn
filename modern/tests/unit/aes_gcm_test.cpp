// aes_gcm_test.cpp — AES-256-GCM cipher unit tests (Phase 3.3).
//
// Tests the mxh::crypto::Aes256GcmCipher class from crypto.cpp:
//   1. Construction succeeds (bcrypt.dll loads, algorithm opens)
//   2. Key generation (seed) produces valid key material
//   3. Encrypt/decrypt round-trip for various data sizes
//   4. Multiple round-trips with counter increment
//   5. Auth tag verification (tampered data rejected)
//   6. Key export/import cycle
//   7. IV export/import cycle
//   8. Empty/zero-size data rejection
//   9. Uninitialized cipher operations fail gracefully

#include "mxh/crypto/crypto.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <cstring>

namespace mxh::crypto {
namespace {

// ========================================================================
// Construction and basic sanity
// ========================================================================

TEST(AesGcmTest, CipherConstructsSuccessfully) {
    Aes256GcmCipher cipher;
    EXPECT_TRUE(cipher.ok()) << "BCrypt should initialize on Windows Vista+";
}

TEST(AesGcmTest, NotOkBeforeSeed) {
    Aes256GcmCipher cipher;
    ASSERT_TRUE(cipher.ok());

    // encrypt/decrypt should fail before seed() is called (no key)
    std::vector<uint8_t> buf(64, 0x42);
    auto r1 = cipher.encrypt(buf);
    EXPECT_NE(r1, mxh::net::NetError::Ok) << "Encrypt before seed should fail";

    auto r2 = cipher.decrypt(buf);
    EXPECT_NE(r2, mxh::net::NetError::Ok) << "Decrypt before seed should fail";
}

// ========================================================================
// Encrypt/decrypt round-trip
// ========================================================================

TEST(AesGcmTest, EncryptDecryptRoundTrip16Bytes) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));

    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    // 16 bytes plaintext + 16 bytes for auth tag
    std::vector<uint8_t> buf(16 + 16);
    for (size_t i = 0; i < 16; i++) buf[i] = static_cast<uint8_t>((i * 17 + 3) & 0xFF);
    std::vector<uint8_t> original(16);
    std::memcpy(original.data(), buf.data(), 16);

    ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);
    // After encrypt, first 16 bytes are ciphertext, but buf is still 32 bytes
    // (BCrypt doesn't resize — we tag the end). For decrypt test, we work
    // with the full buf.

    // Decrypt needs the tag at the end.
    // Create a fresh buffer for decrypt with ciphertext + tag.
    std::vector<uint8_t> ct_buf(16 + 16);
    std::memcpy(ct_buf.data(), buf.data(), 16 + 16);
    ASSERT_EQ(dec.decrypt(ct_buf), mxh::net::NetError::Ok);

    // Compare plaintext (first 16 bytes)
    EXPECT_EQ(std::memcmp(ct_buf.data(), original.data(), 16), 0);
}

TEST(AesGcmTest, EncryptDecryptRoundTrip256Bytes) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    const size_t pt_size = 256;
    std::vector<uint8_t> buf(pt_size + 16);
    for (size_t i = 0; i < pt_size; i++)
        buf[i] = static_cast<uint8_t>((i * 7 + 13) & 0xFF);
    std::vector<uint8_t> original(pt_size);
    std::memcpy(original.data(), buf.data(), pt_size);

    ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);

    std::vector<uint8_t> ct_buf(pt_size + 16);
    std::memcpy(ct_buf.data(), buf.data(), pt_size + 16);
    ASSERT_EQ(dec.decrypt(ct_buf), mxh::net::NetError::Ok);
    EXPECT_EQ(std::memcmp(ct_buf.data(), original.data(), pt_size), 0);
}

TEST(AesGcmTest, EncryptDecryptRoundTrip1024Bytes) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    const size_t pt_size = 1024;
    std::vector<uint8_t> buf(pt_size + 16);
    for (size_t i = 0; i < pt_size; i++)
        buf[i] = static_cast<uint8_t>(i & 0xFF);
    std::vector<uint8_t> original(pt_size);
    std::memcpy(original.data(), buf.data(), pt_size);

    ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);

    std::vector<uint8_t> ct_buf(pt_size + 16);
    std::memcpy(ct_buf.data(), buf.data(), pt_size + 16);
    ASSERT_EQ(dec.decrypt(ct_buf), mxh::net::NetError::Ok);
    EXPECT_EQ(std::memcmp(ct_buf.data(), original.data(), pt_size), 0);
}

TEST(AesGcmTest, EncryptDecryptSingleByte) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    std::vector<uint8_t> buf(1 + 16);
    buf[0] = 0xAB;
    std::vector<uint8_t> original = { 0xAB };

    ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);
    ASSERT_EQ(dec.decrypt(buf), mxh::net::NetError::Ok);
    EXPECT_EQ(buf[0], original[0]);
}

// ========================================================================
// Auth tag verification (tamper detection)
// ========================================================================

TEST(AesGcmTest, CorruptedCiphertextFailsDecrypt) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    std::vector<uint8_t> buf(64 + 16);
    for (size_t i = 0; i < 64; i++) buf[i] = static_cast<uint8_t>(i);
    ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);

    // Corrupt a byte in the ciphertext
    buf[10] ^= 0xFF;

    auto result = dec.decrypt(buf);
    EXPECT_NE(result, mxh::net::NetError::Ok)
        << "Corrupted ciphertext should fail GCM auth tag verification";
}

TEST(AesGcmTest, CorruptedAuthTagFailsDecrypt) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    std::vector<uint8_t> buf(32 + 16);
    for (size_t i = 0; i < 32; i++) buf[i] = static_cast<uint8_t>(i);
    ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);

    // Corrupt the auth tag (last 16 bytes)
    buf[32 + 15] ^= 0x01;

    auto result = dec.decrypt(buf);
    EXPECT_NE(result, mxh::net::NetError::Ok)
        << "Corrupted auth tag should fail GCM verification";
}

// ========================================================================
// Key export/import
// ========================================================================

TEST(AesGcmTest, ExportKeyAfterSeed) {
    Aes256GcmCipher cipher;
    ASSERT_TRUE(cipher.ok());

    cipher.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(cipher.export_key(key));

    // Key should not be all zeros
    bool all_zero = true;
    for (auto b : key) { if (b != 0) { all_zero = false; break; } }
    EXPECT_FALSE(all_zero) << "Generated key should be non-zero (random 256 bits)";
}

TEST(AesGcmTest, ImportExportKeyRoundTrip) {
    Aes256GcmCipher cipher;
    ASSERT_TRUE(cipher.ok());

    cipher.seed();
    std::array<uint8_t, 32> key1 = {};
    ASSERT_TRUE(cipher.export_key(key1));

    // Import into a new cipher
    Aes256GcmCipher cipher2;
    ASSERT_TRUE(cipher2.ok());
    ASSERT_TRUE(cipher2.import_key(key1));

    std::array<uint8_t, 32> key2 = {};
    ASSERT_TRUE(cipher2.export_key(key2));

    EXPECT_EQ(key1, key2) << "Key export→import→export should preserve key material";
}

// ========================================================================
// IV management
// ========================================================================

TEST(AesGcmTest, ExportImportIvRoundTrip) {
    Aes256GcmCipher cipher;
    ASSERT_TRUE(cipher.ok());

    cipher.seed();
    std::array<uint8_t, 12> iv1 = {};
    ASSERT_TRUE(cipher.export_iv(iv1));

    // Import into a new cipher
    Aes256GcmCipher cipher2;
    ASSERT_TRUE(cipher2.ok());
    ASSERT_TRUE(cipher2.import_iv(iv1));

    std::array<uint8_t, 12> iv2 = {};
    ASSERT_TRUE(cipher2.export_iv(iv2));

    EXPECT_EQ(iv1, iv2) << "IV export→import→export should preserve IV";
}

TEST(AesGcmTest, ExportIvBeforeSeedFails) {
    Aes256GcmCipher cipher;
    ASSERT_TRUE(cipher.ok());

    std::array<uint8_t, 12> iv = {};
    EXPECT_FALSE(cipher.export_iv(iv)) << "IV export before seed should fail";
}

// ========================================================================
// Counter increment across multiple messages
// ========================================================================

TEST(AesGcmTest, MultiMessageSequence) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    // Send 5 messages — counter increments each time
    for (int msg = 0; msg < 5; msg++) {
        size_t sz = static_cast<size_t>(32 + msg * 16);
        std::vector<uint8_t> buf(sz + 16);
        for (size_t i = 0; i < sz; i++)
            buf[i] = static_cast<uint8_t>((msg * 256 + i) & 0xFF);
        std::vector<uint8_t> original(sz);
        std::memcpy(original.data(), buf.data(), sz);

        ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);
        ASSERT_EQ(dec.decrypt(buf), mxh::net::NetError::Ok);
        EXPECT_EQ(std::memcmp(buf.data(), original.data(), sz), 0)
            << "Message " << msg << " round-trip failed";
    }
}

// ========================================================================
// Different keys produce different ciphertexts
// ========================================================================

TEST(AesGcmTest, DifferentKeysProduceDifferentOutput) {
    Aes256GcmCipher c1, c2;
    ASSERT_TRUE(c1.ok() && c2.ok());

    c1.seed();
    c2.seed();

    std::vector<uint8_t> buf1(32 + 16), buf2(32 + 16);
    for (size_t i = 0; i < 32; i++) buf1[i] = buf2[i] = static_cast<uint8_t>(i);

    c1.encrypt(buf1);
    c2.encrypt(buf2);

    // Ciphertexts should differ (random keys → different output)
    bool same = (std::memcmp(buf1.data(), buf2.data(), 32) == 0);
    EXPECT_FALSE(same) << "Different keys should produce different ciphertext";
}

// ========================================================================
// Same key+IV produces same ciphertext (deterministic)
// ========================================================================

TEST(AesGcmTest, SameKeyIvProducesSameOutput) {
    Aes256GcmCipher c1, c2;
    ASSERT_TRUE(c1.ok() && c2.ok());

    // c1 generates, c2 imports
    c1.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(c1.export_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(c1.export_iv(iv));

    ASSERT_TRUE(c2.import_key(key));
    ASSERT_TRUE(c2.import_iv(iv));

    std::vector<uint8_t> buf1(32 + 16), buf2(32 + 16);
    for (size_t i = 0; i < 32; i++) buf1[i] = buf2[i] = static_cast<uint8_t>(i);

    c1.encrypt(buf1);
    c2.encrypt(buf2);

    EXPECT_EQ(std::memcmp(buf1.data(), buf2.data(), 32 + 16), 0)
        << "Same key+IV should produce identical ciphertext";
}

// ========================================================================
// Error cases
// ========================================================================

TEST(AesGcmTest, DecryptTooSmallFails) {
    Aes256GcmCipher cipher;
    ASSERT_TRUE(cipher.ok());
    cipher.seed();

    std::vector<uint8_t> buf(8);  // < 16 bytes (minimum tag size)
    auto result = cipher.decrypt(buf);
    EXPECT_NE(result, mxh::net::NetError::Ok) << "Decrypt with < 16 bytes should fail";
}

// ========================================================================
// Stress test
// ========================================================================

TEST(AesGcmStressTest, ManyRandomMessages) {
    Aes256GcmCipher enc, dec;
    ASSERT_TRUE(enc.ok() && dec.ok());

    enc.seed();
    std::array<uint8_t, 32> key = {};
    ASSERT_TRUE(enc.export_key(key));
    ASSERT_TRUE(dec.import_key(key));
    std::array<uint8_t, 12> iv = {};
    ASSERT_TRUE(enc.export_iv(iv));
    ASSERT_TRUE(dec.import_iv(iv));

    for (int i = 0; i < 200; i++) {
        size_t sz = static_cast<size_t>((i % 256) + 1);
        std::vector<uint8_t> buf(sz + 16);
        for (size_t j = 0; j < sz; j++)
            buf[j] = static_cast<uint8_t>((i * 3 + j * 7) & 0xFF);
        std::vector<uint8_t> original(sz);
        std::memcpy(original.data(), buf.data(), sz);

        ASSERT_EQ(enc.encrypt(buf), mxh::net::NetError::Ok);
        ASSERT_EQ(dec.decrypt(buf), mxh::net::NetError::Ok);
        ASSERT_EQ(std::memcmp(buf.data(), original.data(), sz), 0)
            << "Stress test failed at iteration " << i << " size=" << sz;
    }
}

}  // namespace
}  // namespace mxh::crypto
