#include "mxh/render/FilesystemFileStorage.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {
std::filesystem::path findPlayDh() {
    for (const auto& first : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (!first.is_directory()) continue;
        const auto candidate = first.path() / "PlayDH";
        if (std::filesystem::exists(candidate / "Map.pak")) return candidate;
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
