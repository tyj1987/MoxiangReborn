// Tests for mxh::compat::MhFileEx - Phase 7.5g (Bug C-34) UTF-8 roundtrip.
//
// C-34 background:
//   The runtime MapServer's `CheckUpdateFile()` reads
//   `Resource/Server/TitanServer.bin` and strcmps the decoded payload against
//   the source string `이 파일이 없으면 타이탄 업데이트 안돼요~`. After Phase 7.5b
//   the source is UTF-8, the modern build runs on a cp936 (GBK) host, and
//   /execution-charset:utf-8 was added to keep Korean codepoints intact in the
//   binary. The .bin file therefore has to decrypt to UTF-8 bytes (NOT the
//   legacy EUC-KR bytes that the original 2008 PackingMan wrote). This test
//   covers that roundtrip: encode the UTF-8 Korean string into a .bin, read it
//   back, and confirm the result byte-equals what `strcmp` in the binary will
//   compare against.

#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

using namespace mxh::compat;

namespace {

// The literal that Server.cpp line 135 uses as the strcmp() target.
// After Phase 7.5g the .bin's decoded payload must byte-equal this (NO
// surrounding quotes — GetStringInQuotation() strips them at runtime).
constexpr const char* kSentinelUtf8 =
    "\xec\x9d\xb4"                                              // 이
    "\x20"                                                      // space
    "\xed\x8c\x8c"                                              // 파
    "\xec\x9d\xbc"                                              // 일
    "\xec\x9d\xb4"                                              // 이
    "\x20"
    "\xec\x97\x86"                                              // 없
    "\xec\x9c\xbc"                                              // 으
    "\xeb\xa9\xb4"                                              // 면
    "\x20"
    "\xed\x83\x80"                                              // 타
    "\xec\x9d\xb4"                                              // 이
    "\xed\x83\x84"                                              // 탄
    "\x20"
    "\xec\x97\x85"                                              // 업
    "\xeb\x8d\xb0"                                              // 데
    "\xec\x9d\xb4"                                              // 이
    "\xed\x8a\xb8"                                              // 트
    "\x20"
    "\xec\x95\x88"                                              // 안
    "\xeb\x8f\xbc"                                              // 돼
    "\xec\x9a\x94"                                              // 요
    "\x7e";                                                     // ~

constexpr std::size_t kSentinelLen = 57;  // exact byte length of the literal.

}  // namespace

// Roundtrip the Korean sentinel as a .bin payload and confirm the runtime
// decoder reproduces the exact UTF-8 byte sequence the binary's strcmp
// constant holds.
TEST(MhFileExUtf8, C34SentinelRoundtrip) {
    std::vector<std::uint8_t> payload(kSentinelLen);
    std::memcpy(payload.data(), kSentinelUtf8, kSentinelLen);

    // Try every dwType in 1..payload.size() — all should be valid per
    // PackingMan's rand()%size+1 contract.
    for (std::uint32_t dw_type = 1; dw_type <= static_cast<std::uint32_t>(payload.size()); ++dw_type) {
        auto encrypted = encrypt_bin_payload(payload, dw_type);
        auto decrypted = decrypt_bin_payload(encrypted, dw_type);

        ASSERT_EQ(decrypted.size(), payload.size())
            << "dw_type=" << dw_type << " size mismatch";
        for (std::size_t i = 0; i < payload.size(); ++i) {
            EXPECT_EQ(decrypted[i], payload[i])
                << "dw_type=" << dw_type << " byte " << i
                << " expected 0x" << std::hex << payload[i]
                << " got 0x" << std::hex << decrypted[i];
        }
    }
}

// Write the sentinel to disk via the modern writer, then read it back via the
// modern reader. Confirms the on-disk .bin roundtrips without touching
// mxh::compat internals directly.
TEST(MhFileExUtf8, C34SentinelWriteAndRead) {
    auto tmp = std::filesystem::temp_directory_path() /
               "mxh_c34_titan_server_sentinel.bin";

    std::vector<std::uint8_t> payload(kSentinelLen);
    std::memcpy(payload.data(), kSentinelUtf8, kSentinelLen);

    // Pick a small deterministic dwType (the legacy 2008 PackingMan used
    // rand(); any 1..size is legal).
    constexpr std::uint32_t kDwType = 7;
    ASSERT_EQ(write_mh_bin(tmp, payload, kDwType), MhError::Ok);

    auto result = read_mh_bin(tmp);
    ASSERT_TRUE(result.ok()) << "read_mh_bin failed with error "
                             << static_cast<int>(result.error);

    EXPECT_EQ(result.value.header.file_size, payload.size());
    EXPECT_EQ(result.value.data.size(), payload.size());
    for (std::size_t i = 0; i < payload.size(); ++i) {
        EXPECT_EQ(result.value.data[i], payload[i])
            << "byte " << i
            << " expected 0x" << std::hex << payload[i]
            << " got 0x" << std::hex << result.value.data[i];
    }

    // The decoded payload must be the exact UTF-8 byte sequence the
    // runtime strcmp will compare against. If this fails, C-34 is back.
    EXPECT_EQ(std::memcmp(result.value.data.data(), kSentinelUtf8, kSentinelLen), 0)
        << "decoded payload does NOT byte-equal the strcmp sentinel";

    std::filesystem::remove(tmp);
}

// Sanity check: the legacy 2008 .bin (EUC-KR encoded) still loads OK and
// decrypts to the EUC-KR rendering of the same Korean text. This protects
// against accidentally breaking the existing legacy decoder path.
//
// The legacy file lives at
//   D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\Resource\Server\TitanServer.bin
// Path is expressed in UTF-8 (modern build uses /utf-8 globally).
TEST(MhFileExUtf8, LegacyEucKrTitanServerBinDecodesToEucKr) {
    namespace fs = std::filesystem;
    fs::path legacy =
        u8"D:\\墨香全套源代码（源码+资源+客户端+服务端+教程）\\墨香【源码】\\SWorking\\Resource\\Server\\TitanServer.bin";
    if (!fs::exists(legacy)) {
        GTEST_SKIP() << "Legacy TitanServer.bin not found at " << legacy.string()
                     << " — skipping C-34 regression check";
    }

    auto result = read_mh_bin(legacy);
    ASSERT_TRUE(result.ok()) << "legacy read failed: "
                             << static_cast<int>(result.error);
    EXPECT_EQ(result.value.header.file_size, 42u);
    EXPECT_EQ(result.value.data.size(), 42u);

    // The first and last decoded bytes are ASCII double-quotes (the PackingMan
    // tool always wraps the sentinel in quotes for GetStringInQuotation to
    // find).
    EXPECT_EQ(result.value.data.front(), 0x22) << "no leading quote";
    EXPECT_EQ(result.value.data.back(), 0x22) << "no trailing quote";

    // The 40 inner bytes (after GetStringInQuotation strips the quotes) must
    // be the EUC-KR rendering of the same Korean text — i.e. NOT the UTF-8
    // byte sequence. Structural check: '이' in EUC-KR is 0xC0 0xCC, in UTF-8
    // is 0xEC 0x9D 0xB4. We expect the first two decoded bytes to be 0xC0
    // 0xCC (EUC-KR), not 0xEC 0x9D (UTF-8 lead).
    EXPECT_EQ(static_cast<std::uint8_t>(result.value.data[1]), 0xC0)
        << "legacy file's first inner byte is not the EUC-KR lead for '이' (0xC0)";
    EXPECT_EQ(static_cast<std::uint8_t>(result.value.data[2]), 0xCC)
        << "legacy file's second inner byte is not the EUC-KR trail for '이' (0xCC)";
    EXPECT_EQ(static_cast<std::uint8_t>(result.value.data[3]), 0x20)
        << "legacy file's third inner byte is not a space (0x20)";
}
