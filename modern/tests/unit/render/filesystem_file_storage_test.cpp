#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/pack_file.hpp"
#include "mxh/compat/stm_static_model.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {
std::filesystem::path findPlayDh() {
    auto base = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !base.empty();
         ++depth, base = base.parent_path()) {
        const auto canonical = base / "data" / "PlayDH";
        if (std::filesystem::exists(canonical / "Map.pak")) return canonical;
        const auto direct = base / "PlayDH";
        if (std::filesystem::exists(direct / "Map.pak")) return direct;
        for (const auto& first : std::filesystem::directory_iterator(base)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH";
            if (std::filesystem::exists(candidate / "Map.pak")) return candidate;
        }
    }
    return {};
}

struct TempDir {
    std::filesystem::path path;
    explicit TempDir(const char* tag) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
            (std::string(tag) + std::to_string(static_cast<long long>(stamp)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void write_u32(std::ostream& out, std::uint32_t value) {
    const unsigned char bytes[4] = {
        static_cast<unsigned char>(value & 0xffu),
        static_cast<unsigned char>((value >> 8) & 0xffu),
        static_cast<unsigned char>((value >> 16) & 0xffu),
        static_cast<unsigned char>((value >> 24) & 0xffu),
    };
    out.write(reinterpret_cast<const char*>(bytes), 4);
}

// PlayDH / worktree pak: 92-byte header, nonzero uint32 at offset 4,
// one entry starting at offset 92. Matches PackFile::open on this branch.
void write_playdh_pak(const std::filesystem::path& path,
                      std::string_view entry_name,
                      std::string_view payload) {
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out) << path;
    write_u32(out, 1);
    write_u32(out, 1);
    write_u32(out, 0);
    const char zeros[80]{};
    out.write(zeros, 80);
    const auto name_len = static_cast<std::uint32_t>(entry_name.size());
    const auto real = static_cast<std::uint32_t>(payload.size());
    const auto total = 32u + name_len + 1u + real;
    write_u32(out, total);
    write_u32(out, real);
    write_u32(out, name_len);
    write_u32(out, 0);
    write_u32(out, 0);
    write_u32(out, 0);
    write_u32(out, 0);
    write_u32(out, 0);
    out.write(entry_name.data(), static_cast<std::streamsize>(entry_name.size()));
    out.put('\0');
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    ASSERT_TRUE(out);
}

// Original pak: 96-byte header, uint32 at offset 4 is 0, entry count at
// offset 92, entries start at offset 96. The bytes follow docs/formats/pak.md.
void write_original_pak(const std::filesystem::path& path,
                        std::string_view entry_name,
                        std::string_view payload) {
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out) << path;
    write_u32(out, 1);
    write_u32(out, 0);
    write_u32(out, 0);
    const char zeros[80]{};
    out.write(zeros, 80);
    write_u32(out, 1);
    const auto name_len = static_cast<std::uint32_t>(entry_name.size());
    const auto real = static_cast<std::uint32_t>(payload.size());
    const auto total = 32u + name_len + 1u + real;
    write_u32(out, total);
    write_u32(out, real);
    write_u32(out, name_len);
    write_u32(out, 96);
    write_u32(out, 0);
    write_u32(out, 0);
    write_u32(out, 0);
    write_u32(out, 0);
    out.write(entry_name.data(), static_cast<std::streamsize>(entry_name.size()));
    out.put('\0');
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    ASSERT_TRUE(out);
}

std::string read_entry(mxh::gx::FilesystemFileStorage* storage, const char* name) {
    char mutable_name[128]{};
    std::snprintf(mutable_name, sizeof(mutable_name), "%s", name);
    void* file = storage->FSOpenFile(mutable_name, mxh::gx::FSFILE_ACCESSMODE_BINARY);
    if (!file) return {};
    const auto size = storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_END);
    storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_SET);
    std::string bytes(size, '\0');
    if (size != 0 && storage->FSRead(file, bytes.data(), size) != size) bytes.clear();
    storage->FSCloseFile(file);
    return bytes;
}

std::string utf8_filename(const std::filesystem::path& path) {
    const auto u8 = path.filename().u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

// directory_iterator keeps only regular files, so a directory named
// x.pak never reaches classify. This opener is the portable cannot-open
// seam: chmod is not reliable when the process is root or Administrator.
mxh::gx::PakHeaderPrefix deny_locked_pak_headers(const std::filesystem::path& path) {
    const auto name = utf8_filename(path);
    if (name == "m_locked.pak" || name == "\xE4\xB9\x99.pak") {
        mxh::gx::PakHeaderPrefix denied;
        denied.error = "Permission denied";
        return denied;
    }
    return mxh::gx::read_pak_header_prefix(path);
}

std::string mount_order_line(std::string_view logged) {
    const auto at = logged.find("[pak] mount order:");
    if (at == std::string_view::npos) return {};
    const auto end = logged.find('\n', at);
    if (end == std::string_view::npos) return std::string(logged.substr(at));
    return std::string(logged.substr(at, end - at));
}

mxh::gx::FilesystemFileStorage* open_storage(
    const std::filesystem::path& root,
    mxh::gx::FilesystemFileStorageOptions options = {}) {
    auto* storage = new mxh::gx::FilesystemFileStorage(root, std::move(options));
    EXPECT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    return storage;
}
}

TEST(FilesystemFileStorage, WarnsAndSkipsPakThatFailsToOpen) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
        ("mxh-pak-skip-" + std::to_string(static_cast<long long>(stamp)));
    std::filesystem::create_directories(root);
    const auto pak = root / "broken.pak";
    {
        // 96-byte version-1 header of zeros. Both counts are 0, so neither
        // walk reaches EOF. Initialize must not load it.
        std::vector<unsigned char> header(96, 0);
        header[0] = 1;
        std::ofstream out(pak, std::ios::binary);
        ASSERT_TRUE(out);
        out.write(reinterpret_cast<const char*>(header.data()),
                  static_cast<std::streamsize>(header.size()));
    }

    testing::internal::CaptureStderr();
    auto* storage = new mxh::gx::FilesystemFileStorage(root);
    const BOOL opened = storage->Initialize(
        0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE);
    const std::string logged = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(opened);
    EXPECT_NE(logged.find("ERROR"), std::string::npos) << logged;
    EXPECT_NE(logged.find("[pak] ambiguous/unrecognized header: broken.pak"),
              std::string::npos) << logged;
    EXPECT_EQ(logged.find("unsupported header"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("skipped"), std::string::npos) << logged;
    char missing[] = "missing.bin";
    EXPECT_FALSE(storage->IsExistInFileStorage(missing));
    storage->Release();
    std::filesystem::remove_all(root);
}

TEST(FilesystemFileStorage, MountsOriginalMapPack) {
    const auto root = findPlayDh();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    auto* storage = new mxh::gx::FilesystemFileStorage(root);
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    char name[] = "12.map";
    EXPECT_TRUE(storage->IsExistInFileStorage(name));
    void* file = storage->FSOpenFile(name, 0);
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_END), 463u);
    EXPECT_TRUE(storage->FSCloseFile(file));
    storage->Release();
}

TEST(FilesystemFileStorage, ResolvesMapPackBasenameForLegacyRelativePath) {
    const auto root = findPlayDh();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    auto* storage = new mxh::gx::FilesystemFileStorage(root);
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    char name[] = "Map/12.hfl";
    void* file = storage->FSOpenFile(name, 0);
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_END), 1200122u);
    EXPECT_TRUE(storage->FSCloseFile(file));
    storage->Release();
}

TEST(FilesystemFileStorage, ResolvesCanonicalNpcModelAndIdleMotion) {
    const auto root = findPlayDh();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    auto* storage = new mxh::gx::FilesystemFileStorage(root);
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));

    char name[] = "N001.chx";
    void* file = storage->FSOpenFile(name, mxh::gx::FSFILE_ACCESSMODE_BINARY);
    ASSERT_NE(file, nullptr);
    const auto size = storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_END);
    storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_SET);
    std::vector<std::uint8_t> bytes(size);
    ASSERT_EQ(storage->FSRead(file, bytes.data(), size), size);
    EXPECT_TRUE(storage->FSCloseFile(file));

    const auto chx = mxh::compat::ChxModel::parse(bytes);
    ASSERT_TRUE(chx.has_value());
    EXPECT_GT(chx->mod_count(), 0u);
    EXPECT_GT(chx->motion_count(), 0u);
    storage->Release();
}

TEST(FilesystemFileStorage, Map10TerrainAndStaticDependenciesArePresent) {
    const auto root = findPlayDh();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    auto* storage = new mxh::gx::FilesystemFileStorage(root);
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));

    auto read = [&](const char* name) {
        std::vector<std::uint8_t> bytes;
        char mutableName[128]{};
        std::snprintf(mutableName, sizeof(mutableName), "%s", name);
        void* file = storage->FSOpenFile(mutableName, mxh::gx::FSFILE_ACCESSMODE_BINARY);
        if (!file) return bytes;
        const auto size = storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_END);
        storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_SET);
        bytes.resize(size);
        if (size && storage->FSRead(file, bytes.data(), size) != size) bytes.clear();
        storage->FSCloseFile(file);
        return bytes;
    };

    const auto hflBytes = read("10.hfl");
    const auto stmBytes = read("10.stm");
    ASSERT_FALSE(hflBytes.empty());
    ASSERT_FALSE(stmBytes.empty());
    mxh::compat::HflHeightField hfl;
    mxh::compat::StmStaticModel stm;
    std::string error;
    ASSERT_TRUE(mxh::compat::parse_hfl(hflBytes, hfl, &error)) << error;
    ASSERT_TRUE(mxh::compat::parse_stm(stmBytes, stm, &error)) << error;
    ASSERT_GT(hfl.tiles.size(), 0u);
    ASSERT_GT(stm.meshes.size(), 0u);

    std::set<std::string> usedTerrainTextures;
    for (const auto integrated : hfl.tiles) {
        const auto index = integrated & 0x3fffu;
        if (index < hfl.textures.size()) usedTerrainTextures.insert(hfl.textures[index].name);
    }
    for (const auto& texture : usedTerrainTextures) {
        auto bytes = read(texture.c_str());
        if (bytes.empty()) {
            auto dds = texture;
            const auto dot = dds.find_last_of('.');
            if (dot != std::string::npos) dds.replace(dot, std::string::npos, ".dds");
            bytes = read(dds.c_str());
        }
        EXPECT_FALSE(bytes.empty()) << "missing Map10 terrain texture " << texture;
    }
    for (const auto& material : stm.materials) {
        auto bytes = read(material.texture_name.c_str());
        if (bytes.empty()) {
            auto dds = material.texture_name;
            const auto dot = dds.find_last_of('.');
            if (dot != std::string::npos) dds.replace(dot, std::string::npos, ".dds");
            bytes = read(dds.c_str());
        }
        EXPECT_FALSE(bytes.empty())
            << "missing Map10 static texture " << material.texture_name;
    }
    storage->Release();
}

TEST(FilesystemFileStorage, DefaultPakTierOrderIsPlayDhThenOriginal) {
    const mxh::gx::FilesystemFileStorageOptions options;
    ASSERT_EQ(options.pak_tier_order.size(), 2u);
    EXPECT_EQ(options.pak_tier_order[0], mxh::gx::PakFormatTier::PlayDh92);
    EXPECT_EQ(options.pak_tier_order[1], mxh::gx::PakFormatTier::Original96);
}

void poke_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    bytes.at(offset) = static_cast<std::uint8_t>(value & 0xffu);
    bytes.at(offset + 1) = static_cast<std::uint8_t>((value >> 8) & 0xffu);
    bytes.at(offset + 2) = static_cast<std::uint8_t>((value >> 16) & 0xffu);
    bytes.at(offset + 3) = static_cast<std::uint8_t>((value >> 24) & 0xffu);
}

std::uint32_t read_u32_at(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes.at(offset)) |
           (static_cast<std::uint32_t>(bytes.at(offset + 1)) << 8) |
           (static_cast<std::uint32_t>(bytes.at(offset + 2)) << 16) |
           (static_cast<std::uint32_t>(bytes.at(offset + 3)) << 24);
}

// Independent of classify_pak_header. total must equal 32+name_len+1+real,
// the name must be followed by NUL, and `count` entries from `start` must
// finish on the last byte.
bool oracle_walk_ends_at_eof(const std::vector<std::uint8_t>& bytes,
                             std::uint32_t count, std::size_t start) {
    if (start > bytes.size() || count > 1000000u) return false;
    if (count == 0) return start == bytes.size();
    std::size_t cur = start;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (bytes.size() - cur < 32) return false;
        const auto total = read_u32_at(bytes, cur);
        const auto real = read_u32_at(bytes, cur + 4);
        const auto name_len = read_u32_at(bytes, cur + 8);
        const auto expected = 32ull + static_cast<std::uint64_t>(name_len) + 1ull +
                              static_cast<std::uint64_t>(real);
        if (expected != total || total < 33) return false;
        if (static_cast<std::uint64_t>(total) > bytes.size() - cur) return false;
        if (bytes[cur + 32 + name_len] != 0) return false;
        cur += total;
    }
    return cur == bytes.size();
}

void write_bytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out) << path;
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(out);
}

// 96-byte header, decoy count at offset 4, real count at offset 92, and that
// many 33-byte entries from offset 96. The PlayDH walk of the decoy count
// does not end at EOF.
std::vector<std::uint8_t> make_original_with_decoy(std::uint32_t decoy,
                                                   std::uint32_t entry_count) {
    std::vector<std::uint8_t> bytes(96 + static_cast<std::size_t>(entry_count) * 33, 0);
    poke_u32(bytes, 0, 1);
    poke_u32(bytes, 4, decoy);
    poke_u32(bytes, 92, entry_count);
    for (std::uint32_t index = 0; index < entry_count; ++index) {
        poke_u32(bytes, 96 + static_cast<std::size_t>(index) * 33, 33);
    }
    return bytes;
}

// Both layouts validate. PlayDH count at offset 4 is 2 (entries of 66 and
// 4166 bytes from offset 92). Original count at offset 92 is 66 (33, 33,
// 2083, then 63 minimum entries from offset 96). The overlapping headers
// were checked to satisfy total == 32+name_len+1+real on both walks.
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

TEST(FilesystemFileStorage, ClassifiesPakByEntryWalk) {
    const TempDir dir("mxh-pak-classify-");
    const auto playdh = dir.path / "playdh.pak";
    const auto original = dir.path / "original.pak";
    const auto tiny = dir.path / "tiny.pak";
    write_playdh_pak(playdh, "shared.bin", "from-92");
    write_original_pak(original, "shared.bin", "from-96");
    {
        std::ofstream out(tiny, std::ios::binary);
        ASSERT_TRUE(out);
        out.write("pak", 3);
    }

    const auto playdh_header = mxh::gx::classify_pak_header(playdh);
    const auto original_header = mxh::gx::classify_pak_header(original);
    const auto tiny_header = mxh::gx::classify_pak_header(tiny);
    const auto missing_header = mxh::gx::classify_pak_header(dir.path / "missing.pak");
    EXPECT_EQ(playdh_header.kind, mxh::gx::PakHeaderKind::PlayDh92);
    EXPECT_TRUE(playdh_header.playdh92_valid);
    EXPECT_FALSE(playdh_header.original96_valid);
    EXPECT_EQ(playdh_header.entry_count, 1u);
    EXPECT_TRUE(playdh_header.open_error.empty());
    EXPECT_EQ(original_header.kind, mxh::gx::PakHeaderKind::Original96);
    EXPECT_FALSE(original_header.playdh92_valid);
    EXPECT_TRUE(original_header.original96_valid);
    EXPECT_EQ(original_header.entry_count, 1u);
    EXPECT_TRUE(original_header.open_error.empty());
    EXPECT_EQ(tiny_header.kind, mxh::gx::PakHeaderKind::ShorterThan8);
    EXPECT_FALSE(tiny_header.playdh92_valid);
    EXPECT_FALSE(tiny_header.original96_valid);
    EXPECT_TRUE(tiny_header.open_error.empty());
    EXPECT_EQ(missing_header.kind, mxh::gx::PakHeaderKind::CannotOpen);
    EXPECT_FALSE(missing_header.open_error.empty());
}

TEST(FilesystemFileStorage, ClassifiesSyntheticOriginalCounts2942441547) {
    const TempDir dir("mxh-pak-decoy-");
    struct Sample {
        const char* name;
        std::uint32_t decoy_at_offset_4;
        std::uint32_t entry_count;
    };
    const Sample samples[] = {
        {"titan.pak", 99, 294},
        {"farm.pak", 181, 244},
        {"pet.pak", 63, 1547},
    };
    for (const auto& sample : samples) {
        const auto bytes = make_original_with_decoy(sample.decoy_at_offset_4, sample.entry_count);
        ASSERT_EQ(read_u32_at(bytes, 4), sample.decoy_at_offset_4) << sample.name;
        ASSERT_EQ(read_u32_at(bytes, 92), sample.entry_count) << sample.name;
        ASSERT_FALSE(oracle_walk_ends_at_eof(bytes, sample.decoy_at_offset_4, 92)) << sample.name;
        ASSERT_TRUE(oracle_walk_ends_at_eof(bytes, sample.entry_count, 96)) << sample.name;
        const auto path = dir.path / sample.name;
        write_bytes(path, bytes);
        const auto header = mxh::gx::classify_pak_header(path);
        EXPECT_EQ(header.kind, mxh::gx::PakHeaderKind::Original96) << sample.name;
        EXPECT_FALSE(header.playdh92_valid) << sample.name;
        EXPECT_TRUE(header.original96_valid) << sample.name;
        EXPECT_EQ(header.entry_count, sample.entry_count) << sample.name;
        EXPECT_TRUE(header.open_error.empty()) << sample.name;
    }
}

TEST(FilesystemFileStorage, SkipsAmbiguousPakHeader) {
    const TempDir dir("mxh-pak-ambiguous-");
    const auto bytes = make_ambiguous_pak();
    ASSERT_EQ(read_u32_at(bytes, 4), 2u);
    ASSERT_EQ(read_u32_at(bytes, 92), 66u);
    ASSERT_TRUE(oracle_walk_ends_at_eof(bytes, 2, 92));
    ASSERT_TRUE(oracle_walk_ends_at_eof(bytes, 66, 96));
    write_bytes(dir.path / "both.pak", bytes);
    write_playdh_pak(dir.path / "m_ok.pak", "shared.bin", "from-ok");

    const auto header = mxh::gx::classify_pak_header(dir.path / "both.pak");
    EXPECT_EQ(header.kind, mxh::gx::PakHeaderKind::Unrecognized);
    EXPECT_TRUE(header.playdh92_valid);
    EXPECT_TRUE(header.original96_valid);
    EXPECT_EQ(header.entry_count, 0u);

    testing::internal::CaptureStderr();
    auto* storage = open_storage(dir.path);
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);
    const auto error_at = logged.find("[pak] ambiguous/unrecognized header: both.pak");
    EXPECT_NE(logged.find("ERROR"), std::string::npos) << logged;
    ASSERT_NE(error_at, std::string::npos) << logged;
    EXPECT_EQ(logged.find("unsupported header"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("cannot open"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("header shorter than 8 bytes"), std::string::npos) << logged;
    EXPECT_EQ(line.find("both.pak"), std::string::npos) << logged;
    EXPECT_EQ(line, "[pak] mount order: 1=m_ok.pak(92)") << logged;
    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-ok");
    storage->Release();
}

TEST(FilesystemFileStorage, SkipsUnrecognizedPakHeader) {
    const TempDir dir("mxh-pak-unrecognized-");
    // Longer than both header sizes, with a zero count in each slot. An
    // empty PlayDH walk would end at 92 and an empty original walk at 96,
    // so neither finishes at EOF.
    std::vector<std::uint8_t> bytes(128, 0);
    poke_u32(bytes, 0, 1);
    ASSERT_FALSE(oracle_walk_ends_at_eof(bytes, 0, 92));
    ASSERT_FALSE(oracle_walk_ends_at_eof(bytes, 0, 96));
    write_bytes(dir.path / "neither.pak", bytes);
    write_playdh_pak(dir.path / "m_ok.pak", "shared.bin", "from-ok");

    const auto header = mxh::gx::classify_pak_header(dir.path / "neither.pak");
    EXPECT_EQ(header.kind, mxh::gx::PakHeaderKind::Unrecognized);
    EXPECT_FALSE(header.playdh92_valid);
    EXPECT_FALSE(header.original96_valid);
    EXPECT_EQ(header.entry_count, 0u);
    EXPECT_TRUE(header.open_error.empty());

    testing::internal::CaptureStderr();
    auto* storage = open_storage(dir.path);
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);
    const auto error_at = logged.find("[pak] ambiguous/unrecognized header: neither.pak");
    EXPECT_NE(logged.find("ERROR"), std::string::npos) << logged;
    ASSERT_NE(error_at, std::string::npos) << logged;
    EXPECT_EQ(logged.find("unsupported header"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("cannot open"), std::string::npos) << logged;
    EXPECT_EQ(line.find("neither.pak"), std::string::npos) << logged;
    EXPECT_EQ(line, "[pak] mount order: 1=m_ok.pak(92)") << logged;
    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-ok");
    storage->Release();
}

TEST(FilesystemFileStorage, PakTierOrderFollowsConfiguredPriority) {
    using mxh::gx::PakFormatTier;
    using mxh::gx::PakMountCandidate;
    std::vector<PakMountCandidate> input{
        {std::filesystem::path("z_playdh.pak"), PakFormatTier::PlayDh92},
        {std::filesystem::path("a_original.pak"), PakFormatTier::Original96},
        {std::filesystem::path("m_playdh.pak"), PakFormatTier::PlayDh92},
        {std::filesystem::path("b_original.pak"), PakFormatTier::Original96},
    };

    const auto defaults = mxh::gx::FilesystemFileStorageOptions{}.pak_tier_order;
    const auto playdh_first = mxh::gx::order_pak_mounts(input, defaults);
    ASSERT_EQ(playdh_first.size(), 4u);
    EXPECT_EQ(playdh_first[0].path.filename().string(), "m_playdh.pak");
    EXPECT_EQ(playdh_first[1].path.filename().string(), "z_playdh.pak");
    EXPECT_EQ(playdh_first[2].path.filename().string(), "a_original.pak");
    EXPECT_EQ(playdh_first[3].path.filename().string(), "b_original.pak");

    const std::vector<PakFormatTier> original_first{
        PakFormatTier::Original96,
        PakFormatTier::PlayDh92,
    };
    const auto original_wins = mxh::gx::order_pak_mounts(input, original_first);
    ASSERT_EQ(original_wins.size(), 4u);
    EXPECT_EQ(original_wins[0].path.filename().string(), "a_original.pak");
    EXPECT_EQ(original_wins[1].path.filename().string(), "b_original.pak");
    EXPECT_EQ(original_wins[2].path.filename().string(), "m_playdh.pak");
    EXPECT_EQ(original_wins[3].path.filename().string(), "z_playdh.pak");

    const std::vector<PakFormatTier> playdh_only{PakFormatTier::PlayDh92};
    const auto omitted_original_is_last = mxh::gx::order_pak_mounts(input, playdh_only);
    EXPECT_EQ(omitted_original_is_last[0].path.filename().string(), "m_playdh.pak");
    EXPECT_EQ(omitted_original_is_last[2].path.filename().string(), "a_original.pak");
}

TEST(FilesystemFileStorage, CaseOnlyNameTieBreakIsRawBytes) {
    using mxh::gx::PakFormatTier;
    using mxh::gx::PakMountCandidate;
    // Case folding wins over raw byte order: 'B' (0x42) < 'a' (0x61), but
    // the folded names put a.pak first.
    EXPECT_LT(static_cast<unsigned char>('B'), static_cast<unsigned char>('a'));
    EXPECT_TRUE(mxh::gx::pak_filename_less("a.pak", "B.pak"));
    EXPECT_FALSE(mxh::gx::pak_filename_less("B.pak", "a.pak"));

    // Folded names tie. Raw unsigned bytes put the uppercase spelling first.
    EXPECT_TRUE(mxh::gx::pak_filename_less("Data.pak", "data.pak"));
    EXPECT_FALSE(mxh::gx::pak_filename_less("data.pak", "Data.pak"));
    EXPECT_TRUE(mxh::gx::pak_filename_less("ABC.pak", "abc.pak"));
    EXPECT_TRUE(mxh::gx::pak_filename_less("DATA.pak", "Data.pak"));
    EXPECT_FALSE(mxh::gx::pak_filename_less("abc.pak", "ABC.pak"));

    const std::vector<PakFormatTier> tier{PakFormatTier::PlayDh92};
    std::vector<PakMountCandidate> reversed{
        {std::filesystem::path("data.pak"), PakFormatTier::PlayDh92},
        {std::filesystem::path("abc.pak"), PakFormatTier::PlayDh92},
        {std::filesystem::path("Data.pak"), PakFormatTier::PlayDh92},
        {std::filesystem::path("ABC.pak"), PakFormatTier::PlayDh92},
    };
    const auto ordered = mxh::gx::order_pak_mounts(std::move(reversed), tier);
    ASSERT_EQ(ordered.size(), 4u);
    EXPECT_EQ(ordered[0].path.filename().string(), "ABC.pak");
    EXPECT_EQ(ordered[1].path.filename().string(), "abc.pak");
    EXPECT_EQ(ordered[2].path.filename().string(), "Data.pak");
    EXPECT_EQ(ordered[3].path.filename().string(), "data.pak");
}

TEST(FilesystemFileStorage, ResolvesSameEntryByPakNameOrder) {
    const TempDir dir("mxh-pak-name-order-");
    // z_late.pak is created first. An unsorted directory walk would mount
    // it ahead of a_early.pak; name order must still win.
    write_playdh_pak(dir.path / "z_late.pak", "shared.bin", "from-z");
    write_playdh_pak(dir.path / "a_early.pak", "shared.bin", "from-a");

    testing::internal::CaptureStderr();
    auto* storage = open_storage(dir.path);
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);

    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-a");
    EXPECT_NE(line.find("1=a_early.pak(92)"), std::string::npos) << logged;
    EXPECT_NE(line.find("2=z_late.pak(92)"), std::string::npos) << logged;
    const auto early = line.find("a_early.pak(92)");
    const auto late = line.find("z_late.pak(92)");
    ASSERT_NE(early, std::string::npos);
    ASSERT_NE(late, std::string::npos);
    EXPECT_LT(early, late);
    storage->Release();

    testing::internal::CaptureStderr();
    auto* again = open_storage(dir.path);
    const std::string logged_again = testing::internal::GetCapturedStderr();
    EXPECT_EQ(read_entry(again, "shared.bin"), "from-a");
    EXPECT_EQ(mount_order_line(logged_again), line);
    again->Release();
}

TEST(FilesystemFileStorage, LooseFileOverridesMountedPaks) {
    const TempDir dir("mxh-pak-loose-");
    write_playdh_pak(dir.path / "a_early.pak", "shared.bin", "from-a");
    write_playdh_pak(dir.path / "z_late.pak", "shared.bin", "from-z");
    {
        std::ofstream loose(dir.path / "shared.bin", std::ios::binary);
        ASSERT_TRUE(loose);
        loose << "from-loose";
    }

    testing::internal::CaptureStderr();
    auto* storage = open_storage(dir.path);
    testing::internal::GetCapturedStderr();
    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-loose");
    storage->Release();
}

TEST(FilesystemFileStorage, CaseDistinctPakFilesResolveByRawBytes) {
    const TempDir dir("mxh-pak-case-");
    // Lowercase is created first. Raw-byte order must still prefer Data.pak.
    write_playdh_pak(dir.path / "data.pak", "shared.bin", "from-lower");
    write_playdh_pak(dir.path / "Data.pak", "shared.bin", "from-upper");

    bool saw_upper = false;
    bool saw_lower = false;
    for (const auto& item : std::filesystem::directory_iterator(dir.path)) {
        const auto name = item.path().filename().string();
        if (name == "Data.pak") saw_upper = true;
        if (name == "data.pak") saw_lower = true;
    }
    if (!saw_upper || !saw_lower) {
        GTEST_SKIP() << "filesystem cannot store Data.pak and data.pak as distinct files";
    }

    testing::internal::CaptureStderr();
    auto* storage = open_storage(dir.path);
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);
    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-upper");
    EXPECT_NE(line.find("1=Data.pak(92)"), std::string::npos) << logged;
    EXPECT_NE(line.find("2=data.pak(92)"), std::string::npos) << logged;
    storage->Release();
}

TEST(FilesystemFileStorage, CrossTierResolutionFollowsConfiguredPriority) {
    const TempDir dir("mxh-pak-tier-");
    // a_original sorts before z_playdh by filename. Default tier order
    // must still prefer the PlayDH archive when both can be opened.
    write_original_pak(dir.path / "a_original.pak", "shared.bin", "from-96");
    write_playdh_pak(dir.path / "z_playdh.pak", "shared.bin", "from-92");
    ASSERT_EQ(mxh::gx::classify_pak_header(dir.path / "a_original.pak").kind,
              mxh::gx::PakHeaderKind::Original96);
    ASSERT_EQ(mxh::gx::classify_pak_header(dir.path / "z_playdh.pak").kind,
              mxh::gx::PakHeaderKind::PlayDh92);

    if (!mxh::compat::PackFile::open(dir.path / "a_original.pak")) {
        GTEST_SKIP() << "96-byte original paks are not readable until "
                        "feat/packfile-96-header (PR #7). PackFile::open still "
                        "rejects a header whose uint32 at offset 4 is 0. "
                        "Tier priority is locked by PakTierOrderFollowsConfiguredPriority "
                        "and classification by ClassifiesPakByEntryWalk. This case "
                        "starts asserting once that branch can open the fixture "
                        "written here (entry count at offset 92, entries at 96).";
    }

    testing::internal::CaptureStderr();
    auto* playdh_first = open_storage(dir.path);
    const auto playdh_log = mount_order_line(testing::internal::GetCapturedStderr());
    EXPECT_EQ(read_entry(playdh_first, "shared.bin"), "from-92");
    EXPECT_NE(playdh_log.find("1=z_playdh.pak(92)"), std::string::npos) << playdh_log;
    EXPECT_NE(playdh_log.find("2=a_original.pak(96)"), std::string::npos) << playdh_log;
    playdh_first->Release();

    mxh::gx::FilesystemFileStorageOptions original_first;
    original_first.pak_tier_order = {
        mxh::gx::PakFormatTier::Original96,
        mxh::gx::PakFormatTier::PlayDh92,
    };
    testing::internal::CaptureStderr();
    auto* original_wins = open_storage(dir.path, original_first);
    const auto original_log = mount_order_line(testing::internal::GetCapturedStderr());
    EXPECT_EQ(read_entry(original_wins, "shared.bin"), "from-96");
    EXPECT_NE(original_log.find("1=a_original.pak(96)"), std::string::npos) << original_log;
    EXPECT_NE(original_log.find("2=z_playdh.pak(92)"), std::string::npos) << original_log;
    original_wins->Release();
}

TEST(FilesystemFileStorage, Utf8PakNameSortAndLogAreStable) {
    // UTF-8 bytes, not a source-encoded literal: 乙.pak then 甲.pak.
    // 乙 is E4 B9 99, 甲 is E7 94 B2, so 乙 sorts first.
    const std::string yi = "\xE4\xB9\x99.pak";
    const std::string jia = "\xE7\x94\xB2.pak";
    const auto as_path = [](const std::string& bytes) {
        const std::u8string u8(reinterpret_cast<const char8_t*>(bytes.data()), bytes.size());
        return std::filesystem::path(u8);
    };
    const auto filename_bytes = [](const std::filesystem::path& path) {
        const auto u8 = path.filename().u8string();
        return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
    };

    const auto tier = mxh::gx::FilesystemFileStorageOptions{}.pak_tier_order;
    const std::vector<mxh::gx::PakMountCandidate> jia_first{
        {as_path(jia), mxh::gx::PakFormatTier::PlayDh92},
        {as_path(yi), mxh::gx::PakFormatTier::PlayDh92},
    };
    const std::vector<mxh::gx::PakMountCandidate> yi_first{
        {as_path(yi), mxh::gx::PakFormatTier::PlayDh92},
        {as_path(jia), mxh::gx::PakFormatTier::PlayDh92},
    };
    std::vector<mxh::gx::PakMountCandidate> sorted_jia_first;
    std::vector<mxh::gx::PakMountCandidate> sorted_yi_first;
    EXPECT_NO_THROW({
        sorted_jia_first = mxh::gx::order_pak_mounts(jia_first, tier);
        sorted_yi_first = mxh::gx::order_pak_mounts(yi_first, tier);
    });
    ASSERT_EQ(sorted_jia_first.size(), 2u);
    ASSERT_EQ(sorted_yi_first.size(), 2u);
    EXPECT_EQ(filename_bytes(sorted_jia_first[0].path), yi);
    EXPECT_EQ(filename_bytes(sorted_jia_first[1].path), jia);
    EXPECT_EQ(filename_bytes(sorted_yi_first[0].path), yi);
    EXPECT_EQ(filename_bytes(sorted_yi_first[1].path), jia);

    const TempDir dir("mxh-pak-utf8-");
    write_playdh_pak(dir.path / as_path(jia), "shared.bin", "from-jia");
    write_playdh_pak(dir.path / as_path(yi), "shared.bin", "from-yi");

    testing::internal::CaptureStderr();
    auto* storage = new mxh::gx::FilesystemFileStorage(dir.path);
    ASSERT_NO_THROW(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);
    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-yi");
    EXPECT_NE(line.find("1=" + yi + "(92)"), std::string::npos) << logged;
    EXPECT_NE(line.find("2=" + jia + "(92)"), std::string::npos) << logged;
    EXPECT_LT(line.find(yi), line.find(jia)) << line;
    storage->Release();

    testing::internal::CaptureStderr();
    auto* again = new mxh::gx::FilesystemFileStorage(dir.path);
    ASSERT_NO_THROW(again->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    const std::string logged_again = testing::internal::GetCapturedStderr();
    EXPECT_EQ(mount_order_line(logged_again), line);
    EXPECT_EQ(read_entry(again, "shared.bin"), "from-yi");
    again->Release();
}

TEST(FilesystemFileStorage, SkipsPakShorterThanEightBytes) {
    const TempDir dir("mxh-pak-short-");
    // z_short is created first. Warn order must still be the filename
    // order, and neither short file is passed to PackFile::open.
    {
        std::ofstream z(dir.path / "z_short.pak", std::ios::binary);
        ASSERT_TRUE(z);
        z.write("zz", 2);
        std::ofstream a(dir.path / "a_short.pak", std::ios::binary);
        ASSERT_TRUE(a);
        a.write("abcd", 4);
    }
    write_playdh_pak(dir.path / "m_ok.pak", "shared.bin", "from-ok");

    testing::internal::CaptureStderr();
    auto* storage = open_storage(dir.path);
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);
    const auto a_at = logged.find("a_short.pak");
    const auto z_at = logged.find("z_short.pak");

    const auto a_warn = logged.find(
        "[pak] skipped 'a_short.pak': header shorter than 8 bytes");
    const auto z_warn = logged.find(
        "[pak] skipped 'z_short.pak': header shorter than 8 bytes");

    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-ok");
    EXPECT_NE(logged.find("header shorter than 8 bytes"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("cannot open"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("unsupported header"), std::string::npos) << logged;
    ASSERT_NE(a_warn, std::string::npos) << logged;
    ASSERT_NE(z_warn, std::string::npos) << logged;
    EXPECT_LT(a_warn, z_warn) << logged;
    ASSERT_NE(a_at, std::string::npos) << logged;
    ASSERT_NE(z_at, std::string::npos) << logged;
    EXPECT_LT(a_at, z_at) << logged;
    EXPECT_NE(line.find("1=m_ok.pak(92)"), std::string::npos) << logged;
    EXPECT_EQ(line.find("short.pak"), std::string::npos) << line;
    storage->Release();
}

TEST(FilesystemFileStorage, SkipsPakThatCannotBeOpened) {
    const TempDir dir("mxh-pak-locked-");
    // Created out of warn order. Filename UTF-8 order must still be
    // a_short, m_locked, z_short, then 乙 (E4 B9 99).
    const std::string yi = "\xE4\xB9\x99.pak";
    const auto yi_path = std::filesystem::path(
        std::u8string(reinterpret_cast<const char8_t*>(yi.data()), yi.size()));
    {
        std::ofstream z(dir.path / "z_short.pak", std::ios::binary);
        ASSERT_TRUE(z);
        z.write("zz", 2);
        std::ofstream a(dir.path / "a_short.pak", std::ios::binary);
        ASSERT_TRUE(a);
        a.write("abcd", 4);
    }
    write_playdh_pak(dir.path / "m_locked.pak", "locked-only.bin", "from-locked");
    write_playdh_pak(dir.path / yi_path, "yi-only.bin", "from-yi");
    write_playdh_pak(dir.path / "m_ok.pak", "shared.bin", "from-ok");
    ASSERT_TRUE(std::filesystem::create_directory(dir.path / "not-a-file.pak"));

    // The bytes are a real PlayDH pak. Without the seam this would mount.
    EXPECT_EQ(mxh::gx::classify_pak_header(dir.path / "m_locked.pak").kind,
              mxh::gx::PakHeaderKind::PlayDh92);
    const auto denied = mxh::gx::classify_pak_header(
        dir.path / "m_locked.pak", &deny_locked_pak_headers);
    EXPECT_EQ(denied.kind, mxh::gx::PakHeaderKind::CannotOpen);
    EXPECT_EQ(denied.open_error, "Permission denied");
    const auto yi_denied = mxh::gx::classify_pak_header(
        dir.path / yi_path, &deny_locked_pak_headers);
    EXPECT_EQ(yi_denied.kind, mxh::gx::PakHeaderKind::CannotOpen);
    EXPECT_EQ(yi_denied.open_error, "Permission denied");

    auto* storage = new mxh::gx::FilesystemFileStorage(dir.path);
    storage->set_pak_header_opener_for_test(&deny_locked_pak_headers);
    testing::internal::CaptureStderr();
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    const std::string logged = testing::internal::GetCapturedStderr();
    const auto line = mount_order_line(logged);

    const std::string a_warn =
        "[pak] skipped 'a_short.pak': header shorter than 8 bytes";
    const std::string locked_warn =
        "[pak] skipped 'm_locked.pak': cannot open (Permission denied)";
    const std::string z_warn =
        "[pak] skipped 'z_short.pak': header shorter than 8 bytes";
    const std::string yi_warn =
        "[pak] skipped '" + yi + "': cannot open (Permission denied)";
    const auto a_at = logged.find(a_warn);
    const auto locked_at = logged.find(locked_warn);
    const auto z_at = logged.find(z_warn);
    const auto yi_at = logged.find(yi_warn);

    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-ok");
    EXPECT_TRUE(read_entry(storage, "locked-only.bin").empty());
    EXPECT_TRUE(read_entry(storage, "yi-only.bin").empty());
    EXPECT_EQ(logged.find("unsupported header"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("not-a-file.pak"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("a_short.pak': cannot open"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("z_short.pak': cannot open"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("m_locked.pak': header shorter"), std::string::npos) << logged;
    EXPECT_EQ(logged.find(yi + "': header shorter"), std::string::npos) << logged;
    ASSERT_NE(a_at, std::string::npos) << logged;
    ASSERT_NE(locked_at, std::string::npos) << logged;
    ASSERT_NE(z_at, std::string::npos) << logged;
    ASSERT_NE(yi_at, std::string::npos) << logged;
    EXPECT_LT(a_at, locked_at) << logged;
    EXPECT_LT(locked_at, z_at) << logged;
    EXPECT_LT(z_at, yi_at) << logged;
    EXPECT_EQ(line, "[pak] mount order: 1=m_ok.pak(92)") << logged;
    storage->Release();
}
