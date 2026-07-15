// file_storage_typedef_test.cpp - Phase 10.21 file storage wire-format pinning
//
// Covers modern/include/mxh/render/file_storage_typedef.hpp —
// the FILE_ACCESS_METHOD / FSFILE_SEEK / FSFILE_ACCESSMODE
// enums plus the three wire-format structs (FSFILE_HEADER,
// FSFILE_ATOM_INFO, FSPACK_FILE_INFO). The types are 1:1 with
// the original 4DyuchiGRX_common/FileStorage_typedef.h from the
// legacy 2003-era engine. Every field is part of a binary
// on-disk format that legacy tools can still write, so layout
// drift would silently corrupt every .pak file in the game.
//
// What's tested:
//   - All 3 enum constants keep their wire-format values.
//   - All 3 structs are exactly the expected size under
//     #pragma pack(push, 1) — pinned so a future field
//     addition is caught as a deliberate test update rather
//     than a silent binary-format break.

// _MAX_PATH is a MSVC CRT macro (260) used by the on-disk
// .pak format. The hpp itself doesn't pull in the source
// header (latent bug in the header — anything that includes
// it must do this themselves). We define it BEFORE the hpp
// so the struct definitions compile.
#ifdef _WIN32
#  ifndef _MAX_PATH
#    define _MAX_PATH 260
#  endif
#endif

#include "mxh/render/file_storage_typedef.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstddef>

namespace mxh::gx::test {

// ===========================================================================
// Enums
// ===========================================================================

TEST(FileAccessMethodTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(FILE_ACCESS_METHOD_ONLY_FILE),     0u);
    EXPECT_EQ(static_cast<std::uint32_t>(FILE_ACCESS_METHOD_ONLY_PACK),     1u);
    EXPECT_EQ(static_cast<std::uint32_t>(FILE_ACCESS_METHOD_FILE_AND_PACK), 2u);
}

TEST(FsFileSeekTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(FSFILE_SEEK_SET), 0u);
    EXPECT_EQ(static_cast<std::uint32_t>(FSFILE_SEEK_CUR), 1u);
    EXPECT_EQ(static_cast<std::uint32_t>(FSFILE_SEEK_END), 2u);
}

TEST(FsFileAccessModeTest, WireFormatValues) {
    EXPECT_EQ(static_cast<std::uint32_t>(FSFILE_ACCESSMODE_BINARY), 0x00000000u);
    EXPECT_EQ(static_cast<std::uint32_t>(FSFILE_ACCESSMODE_TEXT),   0x00000001u);
}

// ===========================================================================
// FSFILE_HEADER — 32 bytes, pack(1)
// ===========================================================================

TEST(FsFileHeaderTest, SizeIs32Bytes) {
    // Field layout:
    //   char     szFileName[24];   // 24 bytes
    //   uint32_t dwFileNameLen;    // 4 bytes
    //   uint32_t dwRealFileSize;   // 4 bytes
    // Total: 32 bytes under #pragma pack(1).
    static_assert(sizeof(FSFILE_HEADER) == 32,
                  "FSFILE_HEADER must be 32 bytes (24-char name + two uint32_t, pack(1))");
    EXPECT_EQ(sizeof(FSFILE_HEADER), 32u);
}

TEST(FsFileHeaderTest, FieldOffsets) {
    FSFILE_HEADER h{};
    // szFileName at offset 0, length 24.
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.szFileName), reinterpret_cast<std::uintptr_t>(&h));
    // dwFileNameLen at offset 24.
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.dwFileNameLen) -
              reinterpret_cast<std::uintptr_t>(&h), 24u);
    // dwRealFileSize at offset 28.
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.dwRealFileSize) -
              reinterpret_cast<std::uintptr_t>(&h), 28u);
}

TEST(FsFileHeaderTest, FieldAssignmentRoundTrips) {
    FSFILE_HEADER h{};
    const char name[] = "test.bin";
    for (std::size_t i = 0; i < 24; ++i) h.szFileName[i] = '\0';
    for (std::size_t i = 0; name[i] != '\0' && i < 24; ++i) h.szFileName[i] = name[i];
    h.dwFileNameLen  = 8;
    h.dwRealFileSize = 1024;
    EXPECT_EQ(h.dwFileNameLen,  8u);
    EXPECT_EQ(h.dwRealFileSize, 1024u);
    // The first 8 bytes of szFileName are the name, the rest
    // are NUL-padded (the original code uses a fixed 24-byte
    // field, not a NUL-terminated string).
    for (std::size_t i = 0; i < 8; ++i) EXPECT_EQ(h.szFileName[i], name[i]);
    for (std::size_t i = 8; i < 24; ++i) EXPECT_EQ(h.szFileName[i], '\0');
}

TEST(FsFileHeaderTest, DefaultConstructIsZero) {
    FSFILE_HEADER h{};
    // Pack(1) default-initialized struct should be all zero bytes
    // (no padding to worry about, and the char array defaults to
    // empty NULs).
    const char* p = reinterpret_cast<const char*>(&h);
    for (std::size_t i = 0; i < sizeof(FSFILE_HEADER); ++i) {
        EXPECT_EQ(p[i], 0) << "byte " << i;
    }
}

// ===========================================================================
// FSFILE_ATOM_INFO — _MAX_PATH + two uint32_t under pack(1)
// ===========================================================================

TEST(FsFileAtomInfoTest, SizeIsMaxPathPlus8) {
    // _MAX_PATH = 260 on MSVC (the on-disk format uses a Windows
    // full-path buffer). The struct also has two uint32_t fields
    // for total _MAX_PATH + 8 = 268 bytes under pack(1).
    constexpr std::size_t kExpected = 260u + 4u + 4u;
    static_assert(sizeof(FSFILE_ATOM_INFO) == kExpected,
                  "FSFILE_ATOM_INFO must be 268 bytes (260 path + two uint32_t, pack(1))");
    EXPECT_EQ(sizeof(FSFILE_ATOM_INFO), kExpected);
}

TEST(FsFileAtomInfoTest, FieldOffsets) {
    FSFILE_ATOM_INFO a{};
    // szFileName at offset 0, length _MAX_PATH.
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&a.szFileName), reinterpret_cast<std::uintptr_t>(&a));
    // dwRealFileSize at offset _MAX_PATH.
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&a.dwRealFileSize) -
              reinterpret_cast<std::uintptr_t>(&a), 260u);
    // dwFileDataOffset at offset _MAX_PATH + 4.
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&a.dwFileDataOffset) -
              reinterpret_cast<std::uintptr_t>(&a), 260u + 4u);
}

// ===========================================================================
// FSPACK_FILE_INFO — same shape, three uint32_t instead of two
// ===========================================================================

TEST(FsPackFileInfoTest, SizeIsMaxPathPlus12) {
    constexpr std::size_t kExpected = 260u + 4u + 4u + 4u;
    static_assert(sizeof(FSPACK_FILE_INFO) == kExpected,
                  "FSPACK_FILE_INFO must be 272 bytes (260 path + three uint32_t, pack(1))");
    EXPECT_EQ(sizeof(FSPACK_FILE_INFO), kExpected);
}

TEST(FsPackFileInfoTest, FieldOffsets) {
    FSPACK_FILE_INFO p{};
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&p.szPackFileName),
              reinterpret_cast<std::uintptr_t>(&p));
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&p.dwFileNum) -
              reinterpret_cast<std::uintptr_t>(&p), 260u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&p.dwVersion) -
              reinterpret_cast<std::uintptr_t>(&p), 260u + 4u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&p.dwFlags) -
              reinterpret_cast<std::uintptr_t>(&p), 260u + 4u + 4u);
}

// ===========================================================================
// Cross-struct invariants
// ===========================================================================

TEST(FileStorageCrossStructTest, HeaderIsSmallerThanAtomInfo) {
    // FSFILE_HEADER is the compact 32-byte per-file record
    // embedded in a .pak file's table. FSFILE_ATOM_INFO is the
    // external _MAX_PATH-padded record used by the packer tool.
    EXPECT_LT(sizeof(FSFILE_HEADER),   sizeof(FSFILE_ATOM_INFO));
    EXPECT_LT(sizeof(FSFILE_HEADER),   sizeof(FSPACK_FILE_INFO));
    EXPECT_EQ(sizeof(FSFILE_ATOM_INFO), sizeof(FSPACK_FILE_INFO) - 4u);  // 4 fewer uint32_t
}

TEST(FileStorageCrossStructTest, NamesAreRawCharArrays) {
    // The legacy wire format uses a fixed char[N] buffer, not a
    // std::string. Pin this so a future "modernization" using
    // std::array<char, N> or std::string is caught as a
    // deliberate test update — it would change sizeof() in
    // cases where std::string is not small-buffer-optimized.
    static_assert(std::is_same_v<decltype(FSFILE_HEADER{}.szFileName),   char[24]>,
                  "szFileName must be a raw char[24] for binary-compat with the original .pak format");
    static_assert(std::is_same_v<decltype(FSFILE_ATOM_INFO{}.szFileName),     char[260]>,
                  "szFileName must be a raw char[_MAX_PATH] for binary-compat");
    static_assert(std::is_same_v<decltype(FSPACK_FILE_INFO{}.szPackFileName), char[260]>,
                  "szPackFileName must be a raw char[_MAX_PATH] for binary-compat");
    SUCCEED();
}

}  // namespace mxh::gx::test
