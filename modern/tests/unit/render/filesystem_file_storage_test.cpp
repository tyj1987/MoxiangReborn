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
// offset 92, entries start at offset 96. PackFile::open rejects this until
// feat/packfile-96-header (PR #7). The bytes follow docs/formats/pak.md.
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
        // Original-style header: version 1, dwFileItemNum 0. PackFile rejects
        // this (file_item_num must be > 0) and Initialize must skip it.
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
    EXPECT_NE(logged.find("broken.pak"), std::string::npos) << logged;
    EXPECT_NE(logged.find("skipped"), std::string::npos) << logged;
    EXPECT_NE(logged.find("unsupported header"), std::string::npos) << logged;
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

TEST(FilesystemFileStorage, ClassifiesPakByOffset4) {
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

    EXPECT_EQ(mxh::gx::classify_pak_header(playdh), mxh::gx::PakFormatTier::PlayDh92);
    EXPECT_EQ(mxh::gx::classify_pak_header(original), mxh::gx::PakFormatTier::Original96);
    EXPECT_FALSE(mxh::gx::classify_pak_header(tiny).has_value());
    EXPECT_FALSE(mxh::gx::classify_pak_header(dir.path / "missing.pak").has_value());
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
    ASSERT_EQ(mxh::gx::classify_pak_header(dir.path / "a_original.pak"),
              mxh::gx::PakFormatTier::Original96);
    ASSERT_EQ(mxh::gx::classify_pak_header(dir.path / "z_playdh.pak"),
              mxh::gx::PakFormatTier::PlayDh92);

    if (!mxh::compat::PackFile::open(dir.path / "a_original.pak")) {
        GTEST_SKIP() << "96-byte original paks are not readable until "
                        "feat/packfile-96-header (PR #7). PackFile::open still "
                        "rejects a header whose uint32 at offset 4 is 0. "
                        "Tier priority is locked by PakTierOrderFollowsConfiguredPriority "
                        "and classification by ClassifiesPakByOffset4. This case "
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

    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-ok");
    EXPECT_NE(logged.find("header shorter than 8 bytes"), std::string::npos) << logged;
    EXPECT_EQ(logged.find("unsupported header"), std::string::npos) << logged;
    ASSERT_NE(a_at, std::string::npos) << logged;
    ASSERT_NE(z_at, std::string::npos) << logged;
    EXPECT_LT(a_at, z_at) << logged;
    EXPECT_NE(line.find("1=m_ok.pak(92)"), std::string::npos) << logged;
    EXPECT_EQ(line.find("short.pak"), std::string::npos) << line;
    storage->Release();
}
