// Tests for mxh::compat::MhFileEx - .bin format.

#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <vector>

using namespace mxh::compat;

TEST(MhFileEx, RoundtripBasicType0) {
    // Build a payload: "Hello Moxian!" (13 bytes).
    const std::array<std::uint8_t, 13> payload = {
        'H','e','l','l','o',' ','M','o','x','i','a','n','!'
    };

    // Encrypt with type=0.
    auto encrypted = encrypt_bin_payload(payload, /*type=*/0);

    // Build full .bin blob: header + payload.
    MhFileHeader header{};
    header.version = 0x00000001;
    header.type = 0;
    header.file_size = static_cast<std::uint32_t>(payload.size());

    std::vector<std::uint8_t> blob(sizeof(header) + payload.size());
    std::memcpy(blob.data(), &header, sizeof(header));
    std::memcpy(blob.data() + sizeof(header), encrypted.data(), encrypted.size());

    // Sniff detects it.
    EXPECT_TRUE(is_mh_bin(blob));

    // Decrypt should give us back the original payload.
    auto decrypted = decrypt_bin_payload(encrypted, 0);
    ASSERT_EQ(decrypted.size(), payload.size());
    for (std::size_t i = 0; i < payload.size(); ++i) {
        EXPECT_EQ(decrypted[i], payload[i]) << "byte mismatch at " << i;
    }
}

TEST(MhFileEx, RoundtripType1ExtraSubtraction) {
    // type=1 means: at every (i % type == 0) position, also subtract type.
    const std::array<std::uint8_t, 8> payload = {0,0,0,0,0,0,0,0};
    auto encrypted = encrypt_bin_payload(payload, 1);
    auto decrypted = decrypt_bin_payload(encrypted, 1);
    EXPECT_EQ(decrypted.size(), payload.size());
    for (std::size_t i = 0; i < payload.size(); ++i) {
        EXPECT_EQ(decrypted[i], payload[i]);
    }
}

TEST(MhFileEx, RejectTooShort) {
    std::vector<std::uint8_t> tiny = {0x01, 0x00, 0x00, 0x00};
    EXPECT_FALSE(is_mh_bin(tiny));
}

TEST(MhFileEx, Crc8Zero) {
    std::vector<std::uint8_t> empty;
    EXPECT_EQ(compute_crc8(empty), 0);
}

TEST(MhFileEx, Crc8Basic) {
    std::vector<std::uint8_t> data = {1, 2, 3, 4, 5};
    // Sum mod 256.
    EXPECT_EQ(compute_crc8(data), 15);
}

TEST(MhFileEx, WriteAndReadEmptyPayload) {
    const std::vector<std::uint8_t> payload;
    const auto encrypted = encrypt_bin_payload(payload, 1);
    EXPECT_TRUE(encrypted.empty());
}

TEST(MhFileEx, WriteAndReadBack) {
    // Write a small .bin to a temp file and read it back.
    auto tmp = std::filesystem::temp_directory_path() / "mxh_test_roundtrip.bin";

    std::vector<std::uint8_t> payload = {'M', 'o', 'x', 'i', 'a', 'n'};
    ASSERT_EQ(write_mh_bin(tmp, payload, /*type=*/0), MhError::Ok);

    auto result = read_mh_bin(tmp);
    ASSERT_TRUE(result.ok()) << "read_mh_bin failed with error "
                              << static_cast<int>(result.error);
    EXPECT_EQ(result.value.header.version, 0x00000001u);
    EXPECT_EQ(result.value.header.type, 0u);
    EXPECT_EQ(result.value.data, payload);

    std::filesystem::remove(tmp);
}