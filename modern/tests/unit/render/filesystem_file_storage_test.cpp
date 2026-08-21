#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/compat/chx_model.hpp"

#include <gtest/gtest.h>

#include <filesystem>
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
