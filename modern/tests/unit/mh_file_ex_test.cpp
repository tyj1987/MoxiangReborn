// Tests for mxh::compat::MhFileEx - .bin format.

#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>
#include "../support/playdh_env.hpp"

using namespace mxh::compat;

namespace {
std::filesystem::path find_current_playdh() {
    if (const auto playdh_override = mxh::test::playdh_dir_override()) return *playdh_override;
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

TEST(MhFileEx, ReadsCurrentOpaqueContainerWithoutDecodingIt) {
    const auto root = find_current_playdh();
    if (root.empty()) GTEST_SKIP() << "canonical PlayDH root not found";
    const auto path = root / "Resource" / "Server" / "Monster_10.bin";
    const auto result = read_opaque_server_container(path);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value.total_size, 22766u);
    EXPECT_EQ(result.value.payload.size(), 22762u);
    ASSERT_GE(result.value.payload.size(), 4u);
    EXPECT_EQ(result.value.payload[0], 0xDDu);
    EXPECT_EQ(result.value.payload[1], 0x3Au);
    EXPECT_EQ(result.value.payload[2], 0xF2u);
    EXPECT_EQ(result.value.payload[3], 0xF2u);
}

TEST(MhFileEx, DecodesCurrentOpaqueMonster10Payload) {
    const auto root = find_current_playdh();
    if (root.empty()) GTEST_SKIP() << "canonical PlayDH root not found";
    const auto path = root / "Resource" / "Server" / "Monster_10.bin";
    const auto result = read_server_mh_bin(path, "playdh-current");
    ASSERT_TRUE(result.ok());
    ASSERT_GE(result.value.data.size(), 8u);
    const std::string_view text(
        reinterpret_cast<const char*>(result.value.data.data()),
        result.value.data.size());
    EXPECT_EQ(text.compare(0, 8, "$Group 1"), 0);
    EXPECT_NE(text.find("#ADD"), std::string_view::npos);
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
        if (entry.path().filename() == "Monster_10.bin") {
            EXPECT_TRUE(result.ok()) << entry.path().filename().string();
        } else if (!result.ok()) {
            EXPECT_EQ(result.error, MhError::UnsupportedOpaqueServerProfile)
                << entry.path().filename().string();
        }
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

namespace {

std::vector<std::uint8_t> synthetic_r17(const std::vector<std::uint8_t>& payload) {
    const auto total = static_cast<std::uint32_t>(4 + payload.size());
    std::vector<std::uint8_t> bytes(total);
    std::memcpy(bytes.data(), &total, sizeof(total));
    std::memcpy(bytes.data() + 4, payload.data(), payload.size());
    return bytes;
}

void write_bytes(const std::filesystem::path& path,
                 const std::vector<std::uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
}

class TempBin {
public:
    explicit TempBin(const std::vector<std::uint8_t>& bytes, const char* name)
        : path_(std::filesystem::temp_directory_path() / name) {
        write_bytes(path_, bytes);
    }
    ~TempBin() { std::filesystem::remove(path_); }
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

}  // namespace

TEST(MhFileEx, DefaultAndSworkingParseSyntheticR17) {
    const std::vector<std::uint8_t> payload = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
    };
    const auto bytes = synthetic_r17(payload);
    ASSERT_GE(bytes.size(), 20u);
    TempBin file(bytes, "mxh_test_r17.bin");
    const auto expected = decrypt_bin_payload(payload, 0);

    const auto def = read_mh_bin(file.path());
    ASSERT_TRUE(def.ok()) << static_cast<int>(def.error);
    EXPECT_NE(def.error, MhError::ObfuscatedContainer);
    EXPECT_EQ(def.value.header.type, 0u);
    EXPECT_EQ(def.value.header.file_size, payload.size());
    EXPECT_EQ(def.value.data, expected);
    EXPECT_EQ(def.message.find("obfuscated container"), std::string::npos);

    const auto reference = read_mh_bin(file.path(), "sworking-2008-reference");
    ASSERT_TRUE(reference.ok());
    EXPECT_EQ(reference.value.data, def.value.data);
    EXPECT_EQ(reference.value.header.file_size, def.value.header.file_size);

    const auto server = read_server_mh_bin(file.path(), "sworking-2008-reference");
    ASSERT_TRUE(server.ok());
    EXPECT_EQ(server.value.data, expected);
}

TEST(MhFileEx, PlaydhCurrentRefusesSyntheticR17) {
    const std::vector<std::uint8_t> payload(32, 0x5A);
    const auto bytes = synthetic_r17(payload);
    TempBin file(bytes, "mxh_test_r17_playdh.bin");
    const auto result = read_mh_bin(file.path(), "playdh-current");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error, MhError::ObfuscatedContainer);
    EXPECT_NE(result.message.find("obfuscated container"), std::string::npos)
        << result.message;
    EXPECT_NE(result.message.find("bytes=" + std::to_string(bytes.size())),
              std::string::npos)
        << result.message;
    EXPECT_TRUE(result.value.data.empty());
}

TEST(MhFileEx, PlaydhCurrentRefusesContainerOfExactly20Bytes) {
    const auto bytes = synthetic_r17(std::vector<std::uint8_t>(16, 0x11));
    ASSERT_EQ(bytes.size(), 20u);
    TempBin file(bytes, "mxh_test_r17_edge.bin");
    const auto result = read_mh_bin(file.path(), "playdh-current");
    EXPECT_EQ(result.error, MhError::ObfuscatedContainer);
    EXPECT_NE(result.message.find("obfuscated container"), std::string::npos)
        << result.message;
    EXPECT_NE(result.message.find("bytes=20"), std::string::npos) << result.message;
    EXPECT_TRUE(result.value.data.empty());
}

TEST(MhFileEx, PlainClassicBinOkUnderBothProfiles) {
    const std::vector<std::uint8_t> payload = {
        'H', 'e', 'l', 'l', 'o', ' ', 'M', 'o', 'x', 'i', 'a', 'n'
    };
    constexpr std::uint32_t kType = 7;
    const auto encrypted = encrypt_bin_payload(payload, kType);
    MhFileHeader header{};
    header.version = 20040308u + kType + static_cast<std::uint32_t>(payload.size());
    header.type = kType;
    header.file_size = static_cast<std::uint32_t>(payload.size());
    std::vector<std::uint8_t> bytes(sizeof(header) + encrypted.size());
    std::memcpy(bytes.data(), &header, sizeof(header));
    std::memcpy(bytes.data() + sizeof(header), encrypted.data(), encrypted.size());
    TempBin file(bytes, "mxh_test_plain_both_profiles.bin");

    const auto def = read_mh_bin(file.path());
    ASSERT_TRUE(def.ok());
    EXPECT_EQ(def.value.data, payload);
    const auto current = read_mh_bin(file.path(), "playdh-current");
    ASSERT_TRUE(current.ok()) << current.message;
    EXPECT_EQ(current.value.header.type, kType);
    EXPECT_EQ(current.value.data, payload);
    const auto reference = read_mh_bin(file.path(), "sworking-2008-reference");
    ASSERT_TRUE(reference.ok());
    EXPECT_EQ(reference.value.data, payload);
}

TEST(MhFileEx, VersionEqualsSizePlainFileStaysOnR17) {
    const std::vector<std::uint8_t> payload = {
        'p', 'l', 'a', 'i', 'n', '-', 's', 'i', 'z', 'e'
    };
    constexpr std::uint32_t kType = 4;
    const auto encrypted = encrypt_bin_payload(payload, kType);
    const auto total = static_cast<std::uint32_t>(14 + payload.size());
    ASSERT_GE(total, 20u);
    std::vector<std::uint8_t> bytes(total);
    MhFileHeader header{};
    header.version = total;
    header.type = kType;
    header.file_size = static_cast<std::uint32_t>(payload.size());
    std::memcpy(bytes.data(), &header, sizeof(header));
    bytes[12] = 0x11;
    std::memcpy(bytes.data() + 13, encrypted.data(), encrypted.size());
    bytes.back() = 0x22;
    TempBin file(bytes, "mxh_test_plain_size_coincidence.bin");

    const auto expected = decrypt_bin_payload(
        {bytes.data() + 4, bytes.size() - 4}, 0);
    const auto def = read_mh_bin(file.path());
    ASSERT_TRUE(def.ok()) << static_cast<int>(def.error);
    EXPECT_EQ(def.value.header.type, 0u);
    EXPECT_EQ(def.value.header.file_size, total - 4);
    EXPECT_EQ(def.value.data, expected);
    EXPECT_EQ(def.message.find("obfuscated container"), std::string::npos);

    const auto reference = read_server_mh_bin(file.path(), "sworking-2008-reference");
    ASSERT_TRUE(reference.ok());
    EXPECT_EQ(reference.value.data, expected);

    const auto current = read_mh_bin(file.path(), "playdh-current");
    EXPECT_EQ(current.error, MhError::ObfuscatedContainer);
    EXPECT_NE(current.message.find("obfuscated container"), std::string::npos);
}

TEST(MhFileEx, ShortSizePrefixStaysOnR17UnderPlaydhCurrent) {
    std::vector<std::uint8_t> bytes(19, 0x5A);
    bytes[0] = 19;
    bytes[1] = 0;
    bytes[2] = 0;
    bytes[3] = 0;
    TempBin file(bytes, "mxh_test_short_size_prefix.bin");
    const auto def = read_mh_bin(file.path());
    ASSERT_TRUE(def.ok()) << static_cast<int>(def.error);
    EXPECT_EQ(def.value.header.type, 0u);
    EXPECT_EQ(def.value.data.size(), 15u);
    const auto current = read_mh_bin(file.path(), "playdh-current");
    ASSERT_TRUE(current.ok()) << current.message;
    EXPECT_EQ(current.value.data, def.value.data);
}
