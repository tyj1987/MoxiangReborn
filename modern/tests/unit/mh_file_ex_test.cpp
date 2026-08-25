// Tests for mxh::compat::MhFileEx - .bin format.

#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <vector>

using namespace mxh::compat;

namespace {
std::filesystem::path find_current_playdh() {
    auto cursor = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !cursor.empty(); ++depth) {
        const auto candidate = cursor / "modern" / "data" / "PlayDH";
        if (std::filesystem::is_directory(candidate)) return candidate;
        const auto parent = cursor.parent_path();
        if (parent == cursor) break;
        cursor = parent;
    }
    return {};
}
}  // namespace

TEST(MhFileEx, DetectsSizePrefixedServerContainerWithoutCallingItClassic) {
    const std::array<std::uint8_t, 9> blob = {
        9, 0, 0, 0, 0xDD, 0x3A, 0xF2, 0xF2, 0xC1
    };
    EXPECT_TRUE(is_size_prefixed_opaque_server_profile(blob));
    EXPECT_FALSE(is_mh_bin(blob));
}

TEST(MhFileEx, ServerProfileReaderFailsClosedForCurrentOpaqueContainer) {
    auto tmp = std::filesystem::temp_directory_path() /
        "mxh_test_current_server_opaque.bin";
    const std::array<std::uint8_t, 9> blob = {
        9, 0, 0, 0, 0xDD, 0x3A, 0xF2, 0xF2, 0xC1
    };
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(blob.data()),
                  static_cast<std::streamsize>(blob.size()));
    }
    const auto result = read_server_mh_bin(tmp, "playdh-current");
    EXPECT_EQ(result.error, MhError::UnsupportedOpaqueServerProfile);
    std::filesystem::remove(tmp);
}

TEST(MhFileEx, ServerProfileReaderKeepsReferenceProfileExplicit) {
    auto tmp = std::filesystem::temp_directory_path() /
        "mxh_test_reference_server.bin";
    const std::vector<std::uint8_t> payload = {'$', 'G', 'R', 'O', 'U', 'P'};
    ASSERT_EQ(write_mh_bin(tmp, payload, 0), MhError::Ok);
    const auto result = read_server_mh_bin(tmp, "sworking-2008-reference");
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value.data, payload);
    std::filesystem::remove(tmp);
}

TEST(MhFileEx, CurrentOpaqueServerEntriesNeverEnterClassicDecoder) {
    const auto root = find_current_playdh();
    if (root.empty()) GTEST_SKIP() << "canonical PlayDH root not found";
    const auto server = root / "Resource" / "Server";
    ASSERT_TRUE(std::filesystem::is_directory(server));
    std::size_t opaque_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(server)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".bin") continue;
        std::ifstream input(entry.path(), std::ios::binary);
        const std::vector<std::uint8_t> bytes(
            std::istreambuf_iterator<char>(input), {});
        if (!is_size_prefixed_opaque_server_profile(bytes)) continue;
        ++opaque_count;
        const auto result = read_server_mh_bin(entry.path(), "playdh-current");
        EXPECT_EQ(result.error, MhError::UnsupportedOpaqueServerProfile)
            << entry.path().filename().string();
    }
    EXPECT_GT(opaque_count, 1u);
}

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
