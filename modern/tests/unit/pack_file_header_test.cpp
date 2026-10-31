// Synthetic .pak fixtures for the 92-byte and 96-byte headers.
//
// The original client Map.pak is not embedded. ROG measured entry 0 as
// name_len 8, real 174888, total 174929, so water.tif starts at
// 96 + 174929 = 175025 = 0x2ABB1 and its data starts at 0x2ABDB.
// This fixture uses those entry-0 sizes and a 9-byte "water.tif" name.
// The stand-in entry-0 name is another 8-byte string, and the water
// payload is not the original 166508 bytes. 0x2AB31 was a hex typo for
// 0x2ABB1. The count 0x2C03 = 11267 is used only as a declared count
// that cannot fit in a tiny file.

#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using mxh::compat::PackFile;

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xffu));
}

void poke_u32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    out.at(offset) = static_cast<std::uint8_t>(value & 0xffu);
    out.at(offset + 1) = static_cast<std::uint8_t>((value >> 8) & 0xffu);
    out.at(offset + 2) = static_cast<std::uint8_t>((value >> 16) & 0xffu);
    out.at(offset + 3) = static_cast<std::uint8_t>((value >> 24) & 0xffu);
}

std::vector<std::uint8_t> make_worktree_pak(
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>& entries,
    std::uint32_t header_count) {
    std::vector<std::uint8_t> out(92, 0);
    poke_u32(out, 0, 1);
    poke_u32(out, 4, header_count);
    for (const auto& [name, data] : entries) {
        const auto name_len = static_cast<std::uint32_t>(name.size());
        const auto real = static_cast<std::uint32_t>(data.size());
        const std::uint32_t total = 32u + name_len + 1u + real;
        append_u32(out, total);
        append_u32(out, real);
        append_u32(out, name_len);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        out.insert(out.end(), name.begin(), name.end());
        out.push_back(0);
        out.insert(out.end(), data.begin(), data.end());
    }
    return out;
}

std::vector<std::uint8_t> make_original_pak(
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>& entries,
    std::uint32_t count) {
    std::vector<std::uint8_t> out(96, 0);
    poke_u32(out, 0, 1);
    poke_u32(out, 4, 0);
    poke_u32(out, 92, count);
    std::size_t cursor = 96;
    for (const auto& [name, data] : entries) {
        const auto name_len = static_cast<std::uint32_t>(name.size());
        const auto real = static_cast<std::uint32_t>(data.size());
        const std::uint32_t total = 32u + name_len + 1u + real;
        append_u32(out, total);
        append_u32(out, real);
        append_u32(out, name_len);
        append_u32(out, static_cast<std::uint32_t>(cursor));
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        out.insert(out.end(), name.begin(), name.end());
        out.push_back(0);
        out.insert(out.end(), data.begin(), data.end());
        cursor += total;
    }
    return out;
}

class TempPak {
public:
    explicit TempPak(std::vector<std::uint8_t> bytes,
                     std::string filename = "sample.pak") {
        static int seq = 0;
        dir_ = std::filesystem::temp_directory_path() /
               ("mxh-packfile-96-" + std::to_string(++seq) + "-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(dir_);
        path_ = dir_ / filename;
        std::ofstream out(path_, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
        EXPECT_TRUE(out.good());
    }

    ~TempPak() {
        std::error_code ec;
        std::filesystem::remove_all(dir_, ec);
    }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path dir_;
    std::filesystem::path path_;
};

}  // namespace

TEST(PackFileHeader, Worktree92ByteHeader) {
    const std::vector<std::uint8_t> alpha{'A', '1'};
    const std::vector<std::uint8_t> beta{'B', '2', '3'};
    const auto bytes = make_worktree_pak({{"Map\\A.bin", alpha}, {"Map\\B.bin", beta}}, 2);

    EXPECT_EQ(bytes.size() > 92u, true);
    EXPECT_EQ(bytes[4], 2u);
    EXPECT_TRUE(PackFile::is_pak(bytes));

    std::string error = "stale";
    const auto pack = PackFile::open_buffer(bytes, &error);
    ASSERT_NE(pack, nullptr) << error;
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(pack->header().version, 1u);
    EXPECT_EQ(pack->header().file_item_num, 2u);
    EXPECT_EQ(pack->file_count(), 2u);
    EXPECT_EQ(pack->parsed_count(), 2u);
    ASSERT_EQ(pack->entries().size(), 2u);
    EXPECT_EQ(pack->entries()[0].entry_offset, 92u);
    EXPECT_EQ(pack->entries()[1].entry_offset, 92u + 32u + 9u + 1u + 2u);
    EXPECT_NE(pack->find("map\\a.bin"), nullptr);
    EXPECT_EQ(pack->read("Map\\B.bin"), beta);

    TempPak file(bytes);
    const auto from_disk = PackFile::open(file.path());
    ASSERT_NE(from_disk, nullptr);
    EXPECT_EQ(from_disk->read("Map\\A.bin"), alpha);
}

TEST(PackFileHeader, Worktree92IgnoresCorruptTotal) {
    auto bytes = make_worktree_pak(
        {{"A.bin", {1, 2}}, {"B.bin", {3, 4, 5}}}, 2);
    // total at the first entry is huge. The 92-byte loop must keep walking
    // by layout; stepping cur + total would skip B.bin.
    poke_u32(bytes, 92, 0x00FFFFF0u);

    std::string error;
    const auto pack = PackFile::open_buffer(bytes, &error);
    ASSERT_NE(pack, nullptr) << error;
    EXPECT_EQ(pack->parsed_count(), 2u);
    EXPECT_EQ(pack->file_count(), 2u);
    EXPECT_EQ(pack->read("B.bin"), (std::vector<std::uint8_t>{3, 4, 5}));
}

TEST(PackFileHeader, Worktree92ShortFileStillReturnsPartial) {
    const auto bytes = make_worktree_pak({}, 2);
    ASSERT_EQ(bytes.size(), 92u);
    std::string error = "stale";
    const auto pack = PackFile::open_buffer(bytes, &error);
    ASSERT_NE(pack, nullptr);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(pack->file_count(), 2u);
    EXPECT_EQ(pack->parsed_count(), 0u);
}

TEST(PackFileHeader, Original96ByteHeaderWaterEntryOffset) {
    constexpr std::uint32_t kNameLen = 8;
    constexpr std::uint32_t kReal = 174888;
    constexpr std::uint32_t kTotal = 32u + kNameLen + 1u + kReal;
    static_assert(kTotal == 174929u);
    // Measured original Map.pak: water.tif entry start and data start.
    constexpr std::uint32_t kWaterEntryOffset = 96u + kTotal;
    static_assert(kWaterEntryOffset == 175025u);
    static_assert(kWaterEntryOffset == 0x2ABB1u);
    constexpr std::uint32_t kWaterDataOffset = kWaterEntryOffset + 32u + 9u + 1u;
    static_assert(kWaterDataOffset == 175067u);
    static_assert(kWaterDataOffset == 0x2ABDBu);

    std::vector<std::uint8_t> tile(kReal, 0xA5);
    tile.front() = 'T';
    tile.back() = 'Z';
    const std::vector<std::uint8_t> water{'W', 'A', 'T', 'E', 'R'};
    const auto bytes = make_original_pak(
        {{"map0.tga", tile}, {"water.tif", water}}, 2);
    EXPECT_GT(bytes.size(), static_cast<std::size_t>(kWaterDataOffset));
    EXPECT_EQ(bytes[0], 1);
    EXPECT_EQ(bytes[4], 0);
    EXPECT_TRUE(PackFile::is_pak(bytes));

    std::string error = "stale";
    const auto pack = PackFile::open_buffer(std::vector<std::uint8_t>(bytes), &error);
    ASSERT_NE(pack, nullptr) << error;
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(pack->header().file_item_num, 0u);
    EXPECT_EQ(pack->file_count(), 2u);
    EXPECT_EQ(pack->parsed_count(), 2u);
    ASSERT_EQ(pack->entries().size(), 2u);
    EXPECT_EQ(pack->entries()[0].name, "map0.tga");
    EXPECT_EQ(pack->entries()[0].entry_offset, 96u);
    EXPECT_EQ(pack->entries()[0].data_offset, 137u);
    EXPECT_EQ(pack->entries()[0].size, kReal);
    EXPECT_EQ(pack->entries()[1].name, "water.tif");
    EXPECT_EQ(pack->entries()[1].entry_offset, kWaterEntryOffset);
    EXPECT_EQ(pack->entries()[1].data_offset, kWaterDataOffset);
    EXPECT_EQ(pack->read("WATER.TIF"), water);
    const auto tile_back = pack->read("map0.tga");
    ASSERT_EQ(tile_back.size(), kReal);
    EXPECT_EQ(tile_back.front(), static_cast<std::uint8_t>('T'));
    EXPECT_EQ(tile_back.back(), static_cast<std::uint8_t>('Z'));

    TempPak file(bytes);
    const auto from_disk = PackFile::open(file.path(), &error);
    ASSERT_NE(from_disk, nullptr) << error;
    const auto* water_entry = from_disk->find("water.tif");
    ASSERT_NE(water_entry, nullptr);
    EXPECT_EQ(water_entry->entry_offset, kWaterEntryOffset);
    EXPECT_EQ(water_entry->data_offset, kWaterDataOffset);
}

TEST(PackFileHeader, Original96KeepsOnDiskNameBytes) {
    // GBK for 水面.tif, the same bytes the Python fixture locks.
    const std::string gbk("\xcb\xae\xc3\xe6.tif", 8);
    const std::vector<std::uint8_t> payload{'S'};
    const auto bytes = make_original_pak({{gbk, payload}}, 1);
    std::string error;
    const auto pack = PackFile::open_buffer(bytes, &error);
    ASSERT_NE(pack, nullptr) << error;
    EXPECT_EQ(pack->entries().at(0).name, gbk);
    EXPECT_EQ(pack->read(gbk), payload);
    const std::string utf8 = "\xe6\xb0\xb4\xe9\x9d\xa2.tif";
    EXPECT_EQ(pack->find(utf8), nullptr);
}

TEST(PackFileHeader, TruncatedHeaderFailsClosed) {
    auto bytes = make_original_pak({{"water.tif", {1, 2, 3, 4}}}, 1);
    ASSERT_GT(bytes.size(), 96u);
    bytes.resize(95);
    EXPECT_FALSE(PackFile::is_pak(bytes));

    std::string error;
    const auto pack = PackFile::open_buffer(bytes, &error);
    EXPECT_EQ(pack, nullptr);
    EXPECT_NE(error.find("truncated pak header"), std::string::npos) << error;

    std::vector<std::uint8_t> only_92(92, 0);
    only_92[0] = 1;
    EXPECT_FALSE(PackFile::is_pak(only_92));
    error.clear();
    EXPECT_EQ(PackFile::open_buffer(only_92, &error), nullptr);
    EXPECT_NE(error.find("truncated pak header"), std::string::npos) << error;
}

TEST(PackFileHeader, ZeroCountAtOffset92FailsClosed) {
    // Same shape FilesystemFileStorage / EffectCatalog use as "not a pak":
    // version 1, everything else zero, including the count at offset 92.
    std::vector<std::uint8_t> bytes(96, 0);
    bytes[0] = 1;
    EXPECT_FALSE(PackFile::is_pak(bytes));
    std::string error;
    EXPECT_EQ(PackFile::open_buffer(bytes, &error), nullptr);
    EXPECT_NE(error.find("entry count at offset 92 is 0"), std::string::npos) << error;

    TempPak file(bytes, "Effect.pak");
    testing::internal::CaptureStderr();
    error.clear();
    EXPECT_EQ(PackFile::open(file.path(), &error), nullptr);
    const std::string logged = testing::internal::GetCapturedStderr();
    EXPECT_NE(error.find("entry count at offset 92 is 0"), std::string::npos) << error;
    EXPECT_EQ(logged.find("ambiguous/unrecognized"), std::string::npos) << logged;
}

TEST(PackFileHeader, EntryOverrunsEofFailsClosed) {
    auto bytes = make_original_pak({{"water.tif", std::vector<std::uint8_t>(16, 0x11)}}, 1);
    ASSERT_GT(bytes.size(), 96u + 33u);
    bytes.pop_back();
    EXPECT_TRUE(PackFile::is_pak(bytes));

    std::string error;
    const auto pack = PackFile::open_buffer(bytes, &error);
    EXPECT_EQ(pack, nullptr);
    EXPECT_NE(error.find("overruns EOF"), std::string::npos) << error;
    EXPECT_EQ(error.find("larger than the file"), std::string::npos) << error;
}

TEST(PackFileHeader, EntryCountLargerThanFileFailsClosed) {
    constexpr std::uint32_t kOriginalMapCount = 0x2C03;
    static_assert(kOriginalMapCount == 11267u);

    const auto header_only = make_original_pak({}, kOriginalMapCount);
    ASSERT_EQ(header_only.size(), 96u);
    EXPECT_TRUE(PackFile::is_pak(header_only));

    std::string error;
    const auto pack = PackFile::open_buffer(header_only, &error);
    EXPECT_EQ(pack, nullptr);
    EXPECT_NE(error.find("11267"), std::string::npos) << error;
    EXPECT_NE(error.find("larger than the file allows"), std::string::npos) << error;

    // Three declared entries, but only one minimum-sized slot after the header.
    auto tiny = make_original_pak({{"a", {1}}}, 3);
    std::string error_tiny;
    EXPECT_EQ(PackFile::open_buffer(tiny, &error_tiny), nullptr);
    EXPECT_NE(error_tiny.find("entry count 3"), std::string::npos) << error_tiny;
    EXPECT_NE(error_tiny.find("larger than the file allows"), std::string::npos)
        << error_tiny;
}

TEST(PackFileHeader, TrailingBytesFailClosed) {
    auto bytes = make_original_pak({{"water.tif", {9}}, {"a.bin", {8, 7}}}, 2);
    bytes.push_back(0xAB);
    std::string error;
    const auto pack = PackFile::open_buffer(bytes, &error);
    EXPECT_EQ(pack, nullptr);
    EXPECT_NE(error.find("remain after the declared count"), std::string::npos) << error;
    EXPECT_EQ(error.find("EOF arrived early"), std::string::npos) << error;
}

TEST(PackFileHeader, CorruptEntryDoesNotReadPastEof) {
    auto bytes = make_original_pak({{"water.tif", {1, 2, 3, 4}}}, 1);
    // name_len is huge, total does not match. The reader must fail before
    // treating that length as a byte count.
    poke_u32(bytes, 96 + 8, 0xFFFFFFF0u);
    poke_u32(bytes, 96, 0);
    std::string error;
    EXPECT_EQ(PackFile::open_buffer(bytes, &error), nullptr);
    EXPECT_NE(error.find("corrupt pak entry"), std::string::npos) << error;

    auto missing_nul = make_original_pak({{"water.tif", {1, 2, 3, 4}}}, 1);
    const std::size_t nul_at = 96u + 32u + std::string("water.tif").size();
    ASSERT_LT(nul_at, missing_nul.size());
    missing_nul[nul_at] = 0x20;
    error.clear();
    EXPECT_EQ(PackFile::open_buffer(missing_nul, &error), nullptr);
    EXPECT_NE(error.find("NUL"), std::string::npos) << error;
}

std::vector<std::uint8_t> make_decoy_original(std::uint32_t decoy, std::uint32_t count) {
    std::vector<std::uint8_t> bytes(96 + static_cast<std::size_t>(count) * 33, 0);
    poke_u32(bytes, 0, 1);
    poke_u32(bytes, 4, decoy);
    poke_u32(bytes, 92, count);
    for (std::uint32_t index = 0; index < count; ++index) {
        poke_u32(bytes, 96 + static_cast<std::size_t>(index) * 33, 33);
    }
    return bytes;
}

// Same overlapping bytes as FilesystemFileStorage's ambiguous fixture:
// PlayDH count 2 from offset 92 (66 then 4166 bytes) and original count 66
// from offset 96 both finish at offset 4324.
std::vector<std::uint8_t> make_ambiguous_pak() {
    constexpr std::size_t kEof = 4324;
    std::vector<std::uint8_t> bytes(kEof, 0);
    const auto write_min = [&](std::size_t at) { poke_u32(bytes, at, 33); };
    poke_u32(bytes, 0, 1);
    poke_u32(bytes, 4, 2);
    write_min(96);
    write_min(129);
    poke_u32(bytes, 162, 2083);
    poke_u32(bytes, 166, 2050);
    std::size_t at = 2245;
    for (int index = 0; index < 63; ++index) {
        write_min(at);
        at += 33;
    }
    EXPECT_EQ(at, kEof);
    poke_u32(bytes, 92, 66);
    poke_u32(bytes, 96, 33);
    poke_u32(bytes, 158, 4166);
    poke_u32(bytes, 162, 2083);
    poke_u32(bytes, 166, 2050);
    return bytes;
}

TEST(PackFileHeader, ReadsOriginal96WithNonzeroOffset4) {
    struct Sample {
        const char* name;
        std::uint32_t decoy;
        std::uint32_t count;
    };
    const Sample samples[] = {
        {"titan", 99, 294},
        {"farm", 181, 244},
        {"pet", 63, 1547},
    };
    for (const auto& sample : samples) {
        const auto bytes = make_decoy_original(sample.decoy, sample.count);
        const auto probe = mxh::compat::probe_pak_layouts(bytes);
        EXPECT_FALSE(probe.playdh92_exact_eof) << sample.name;
        EXPECT_TRUE(probe.original96_exact_eof) << sample.name;

        std::string error;
        const auto pack = PackFile::open_buffer(bytes, &error);
        ASSERT_NE(pack, nullptr) << sample.name << " " << error;
        EXPECT_TRUE(error.empty()) << sample.name;
        EXPECT_EQ(pack->header().version, 1u) << sample.name;
        EXPECT_EQ(pack->header().file_item_num, sample.decoy) << sample.name;
        EXPECT_EQ(pack->file_count(), sample.count) << sample.name;
        EXPECT_EQ(pack->parsed_count(), sample.count) << sample.name;
        ASSERT_EQ(pack->entries().size(), sample.count) << sample.name;
        EXPECT_EQ(pack->entries().front().entry_offset, 96u) << sample.name;
        EXPECT_EQ(pack->entries().front().size, 0u) << sample.name;
        EXPECT_EQ(pack->entries().back().entry_offset,
                  96u + (sample.count - 1u) * 33u) << sample.name;
        const auto mid = sample.count / 2u;
        EXPECT_EQ(pack->entries()[mid].entry_offset, 96u + mid * 33u) << sample.name;
    }
}

TEST(PackFileHeader, RejectsHeaderBothWalksReachEof) {
    const auto bytes = make_ambiguous_pak();
    const auto probe = mxh::compat::probe_pak_layouts(bytes);
    EXPECT_TRUE(probe.playdh92_exact_eof);
    EXPECT_TRUE(probe.original96_exact_eof);

    std::string error = "stale";
    EXPECT_EQ(PackFile::open_buffer(bytes, &error), nullptr);
    EXPECT_EQ(error, "ambiguous/unrecognized header");

    TempPak file(bytes, "both.pak");
    testing::internal::CaptureStderr();
    error = "stale";
    EXPECT_EQ(PackFile::open(file.path(), &error), nullptr);
    const std::string logged = testing::internal::GetCapturedStderr();
    EXPECT_EQ(error, "ambiguous/unrecognized header");
    EXPECT_NE(logged.find("ERROR [pak] ambiguous/unrecognized header: both.pak"),
              std::string::npos) << logged;
}

TEST(PackFileHeader, RejectsHeaderNeitherWalkReachesEof) {
    std::vector<std::uint8_t> bytes(200, 0);
    poke_u32(bytes, 0, 1);
    poke_u32(bytes, 4, 3);
    poke_u32(bytes, 92, 5);
    const auto probe = mxh::compat::probe_pak_layouts(bytes);
    EXPECT_FALSE(probe.playdh92_exact_eof);
    EXPECT_FALSE(probe.original96_exact_eof);

    std::string error = "stale";
    EXPECT_EQ(PackFile::open_buffer(bytes, &error), nullptr);
    EXPECT_EQ(error, "ambiguous/unrecognized header");

    TempPak file(bytes, "neither.pak");
    testing::internal::CaptureStderr();
    error = "stale";
    EXPECT_EQ(PackFile::open(file.path(), &error), nullptr);
    const std::string logged = testing::internal::GetCapturedStderr();
    EXPECT_EQ(error, "ambiguous/unrecognized header");
    EXPECT_NE(logged.find("ERROR [pak] ambiguous/unrecognized header: neither.pak"),
              std::string::npos) << logged;
}

TEST(PackFileHeader, RejectsNonPakBlob) {
    const std::vector<std::uint8_t> garbage{0x01, 0x02, 0x03};
    EXPECT_FALSE(PackFile::is_pak(garbage));
    std::string error;
    EXPECT_EQ(PackFile::open_buffer(garbage, &error), nullptr);
    EXPECT_EQ(error, "not a pak");
}

// Synthetic archives generated in this test. Payloads are not from the repo tree.
TEST(PackFileHeader, SyntheticNpcPetTitanArchivesRoundTrip) {
    struct Spec {
        const char* archive;
        const char* entry;
        std::vector<std::uint8_t> payload;
    };
    const Spec specs[] = {
        {"npc.pak", "npc\\synth.bin", {'n', 'p', 'c', '-', '1'}},
        {"Pet.pak", "pet\\synth.bin", {'p', 'e', 't', '-', '2'}},
        {"Titan.pak", "titan\\synth.bin", {'t', 'i', 'n', '-', '3'}},
    };
    for (const auto& spec : specs) {
        const auto bytes = make_worktree_pak({{spec.entry, spec.payload}}, 1);
        EXPECT_TRUE(PackFile::is_pak(bytes)) << spec.archive;
        std::string error = "stale";
        const auto pack = PackFile::open_buffer(bytes, &error);
        ASSERT_NE(pack, nullptr) << spec.archive << " " << error;
        EXPECT_TRUE(error.empty()) << spec.archive;
        EXPECT_EQ(pack->file_count(), 1u) << spec.archive;
        EXPECT_EQ(pack->parsed_count(), 1u) << spec.archive;
        EXPECT_EQ(pack->read(spec.entry), spec.payload) << spec.archive;

        TempPak file(bytes, spec.archive);
        const auto from_disk = PackFile::open(file.path());
        ASSERT_NE(from_disk, nullptr) << spec.archive;
        EXPECT_EQ(from_disk->parsed_count(), 1u) << spec.archive;
        EXPECT_EQ(from_disk->read(spec.entry), spec.payload) << spec.archive;
    }
}
