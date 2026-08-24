#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/stm_static_model.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <cstdio>
#include <set>
#include <string>
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
