#include "mxh/compat/pack_file.hpp"
#include "mxh/compat/stm_static_model.hpp"
#include "mxh/compat/chx_model.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <algorithm>
#include <cctype>

namespace {
std::filesystem::path findMapPack() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "Map.pak";
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}

std::filesystem::path findMonsterPack() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "monster.pak";
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}

std::filesystem::path findCharacterPack() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "Character.pak";
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}
}

TEST(StmStaticModel, ParsesRealMap12Scene) {
    const auto path = findMapPack();
    if (path.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto bytes = pack->read("12.stm");
    ASSERT_EQ(bytes.size(), 4670294u);
    mxh::compat::StmStaticModel model;
    std::string error;
    ASSERT_TRUE(mxh::compat::parse_stm(bytes, model, &error)) << error;
    EXPECT_EQ(model.version, 1u);
    EXPECT_FALSE(model.materials.empty());
    EXPECT_FALSE(model.meshes.empty());
    EXPECT_EQ(model.materials.size(), 63u);
    EXPECT_EQ(model.meshes.size(), 334u);
    std::size_t vertices = 0, faces = 0;
    for (const auto& mesh : model.meshes) {
        vertices += mesh.positions.size();
        for (const auto& group : mesh.face_groups) {
            faces += group.indices.size() / 3;
            for (const auto index : group.indices) EXPECT_LT(index, mesh.positions.size());
        }
    }
    EXPECT_EQ(vertices, 120395u);
    EXPECT_EQ(faces, 58601u);
    EXPECT_EQ(model.collision_offset, 4624722u);
    EXPECT_LT(model.collision_offset, bytes.size());
}

TEST(StmStaticModel, RejectsTruncatedData) {
    const std::uint8_t bytes[4]{1, 0, 0, 0};
    mxh::compat::StmStaticModel model;
    EXPECT_FALSE(mxh::compat::parse_stm(bytes, model));
}

TEST(StmStaticModel, ParsesRealMap12SkyModel) {
    const auto path = findMapPack();
    if (path.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto bytes = pack->read("sky_07gaebong.mod");
    ASSERT_EQ(bytes.size(), 30044u);
    mxh::compat::StmStaticModel model;
    std::string error;
    ASSERT_TRUE(mxh::compat::parse_mod(bytes, model, &error)) << error;
    EXPECT_EQ(model.materials.size(), 8u);
    EXPECT_EQ(model.meshes.size(), 8u);
}

TEST(StmStaticModel, ParsesRealL001BonesAndPhysique) {
    const auto path = findMonsterPack();
    if (path.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto readBasename = [&](std::string wanted) {
        std::transform(wanted.begin(), wanted.end(), wanted.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (const auto& entry : pack->entries()) {
            auto base = std::filesystem::path(entry.name).filename().string();
            std::transform(base.begin(), base.end(), base.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (base == wanted) return pack->read(entry.name);
        }
        return std::vector<std::uint8_t>{};
    };
    const auto chxBytes = readBasename("L001.chx");
    const auto chx = mxh::compat::ChxModel::parse(chxBytes);
    ASSERT_TRUE(chx.has_value());
    ASSERT_FALSE(chx->mod_files.empty());
    std::size_t bones = 0, physiqueVertices = 0;
    for (const auto& name : chx->mod_files) {
        const auto bytes = readBasename(name);
        mxh::compat::StmStaticModel model;
        std::string error;
        ASSERT_TRUE(mxh::compat::parse_mod(bytes, model, &error)) << name << ": " << error;
        bones += model.bones.size();
        for (const auto& mesh : model.meshes)
            physiqueVertices += std::count_if(mesh.physique.begin(), mesh.physique.end(),
                [](const auto& influences) { return !influences.empty(); });
    }
    EXPECT_GT(bones, 0u);
    EXPECT_GT(physiqueVertices, 0u);
}

TEST(StmStaticModel, ParsesEveryDefaultMaleCharacterPart) {
    const auto path = findCharacterPack();
    if (path.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto readBasename = [&](std::string wanted) {
        std::transform(wanted.begin(), wanted.end(), wanted.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (const auto& entry : pack->entries()) {
            auto base = std::filesystem::path(entry.name).filename().string();
            std::transform(base.begin(), base.end(), base.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (base == wanted) return pack->read(entry.name);
        }
        return std::vector<std::uint8_t>{};
    };
    const auto chx = mxh::compat::ChxModel::parse(readBasename("man.chx"));
    ASSERT_TRUE(chx.has_value());
    ASSERT_EQ(chx->mod_files.size(), 5u);
    for (const auto& name : chx->mod_files) {
        mxh::compat::StmStaticModel model;
        std::string error;
        ASSERT_TRUE(mxh::compat::parse_mod(readBasename(name), model, &error))
            << name << ": " << error;
        EXPECT_FALSE(model.meshes.empty()) << name;
    }
}
