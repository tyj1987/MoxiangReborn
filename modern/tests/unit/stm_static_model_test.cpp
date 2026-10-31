#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/pack_file.hpp"
#include "mxh/compat/stm_static_model.hpp"

#include "../support/handwritten_fixture.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<std::uint8_t> read_basename(const mxh::compat::PackFile& pack,
                                       std::string wanted) {
    std::transform(wanted.begin(), wanted.end(), wanted.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& entry : pack.entries()) {
        const auto& name = entry.name;
        const auto sep = name.find_last_of("/\\");
        std::string base = (sep == std::string::npos) ? name : name.substr(sep + 1);
        std::transform(base.begin(), base.end(), base.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (base == wanted) return pack.read(entry.name);
    }
    return {};
}

mxh::test::HandwrittenMesh triangle_mesh(std::string name, std::uint32_t index,
                                        std::uint32_t material_index) {
    mxh::test::HandwrittenMesh mesh;
    mesh.index = index;
    mesh.name = std::move(name);
    mesh.material_index = material_index;
    mesh.positions = {{{0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 5.0f, 0.0f}}};
    mesh.indices = {0, 1, 2};
    return mesh;
}

}  // namespace

// Real map 12 static scene. Counts, the byte size, and the collision
// offset are properties of that file.
TEST(StmStaticModel, ParsesRealMap12Scene) {
    const auto root = mxh::test::require_playdh_root();
    const auto path = root / "Map.pak";
    ASSERT_TRUE(std::filesystem::is_regular_file(path))
        << mxh::test::kPlayDhRootEnv << " is set but Map.pak is missing";
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
    ASSERT_FALSE(model.collision_bounds.empty());
    for (const auto& bounds : model.collision_bounds) {
        EXPECT_LE(bounds.min[0], bounds.max[0]);
        EXPECT_LE(bounds.min[1], bounds.max[1]);
        EXPECT_LE(bounds.min[2], bounds.max[2]);
    }
}

TEST(StmStaticModel, RejectsTruncatedData) {
    const std::uint8_t bytes[4]{1, 0, 0, 0};
    mxh::compat::StmStaticModel model;
    EXPECT_FALSE(mxh::compat::parse_stm(bytes, model));
}

// Real sky model stored beside the map scene.
TEST(StmStaticModel, ParsesRealMap12SkyModel) {
    const auto root = mxh::test::require_playdh_root();
    const auto path = root / "Map.pak";
    ASSERT_TRUE(std::filesystem::is_regular_file(path))
        << mxh::test::kPlayDhRootEnv << " is set but Map.pak is missing";
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

// Hand-written chx plus one skinned .mod. The real monster pack is
// RealL001BonesAndPhysique.
TEST(StmStaticModel, ParsesRealL001BonesAndPhysique) {
    mxh::test::HandwrittenMaterial material;
    material.index = 4;
    material.texture_name = "syn_limb";
    material.diffuse = 0x00a0b0c0u;
    material.transparency = 0.5f;
    material.flags = 3;
    mxh::test::HandwrittenMesh mesh;
    mesh.index = 8;
    mesh.name = "syn_limb";
    mesh.material_index = 4;
    mesh.positions = {{{1.0f, 2.0f, 3.0f}}};
    mesh.with_physique = true;
    mesh.physique_bone_index = 4;
    mesh.physique_weight = 0.75f;
    mesh.physique_offset = {{1.0f, 2.0f, 3.0f}};
    mesh.physique_normal = {{0.0f, 0.0f, 1.0f}};
    const auto mod = mxh::test::build_mod(material, mesh, true, "syn_bone", 6);
    ASSERT_FALSE(mod.empty());
    const auto text = mxh::compat::ChxModel::serialize_text({"syn_limb.mod"}, {});
    const std::vector<std::uint8_t> chx(text.begin(), text.end());
    const auto pak = mxh::test::build_original96_pak({
        {"mob/L001.chx", chx},
        {"mob/syn_limb.mod", mod},
    });
    ASSERT_FALSE(pak.empty());
    mxh::test::HandwrittenFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const auto path = dir.write_bytes("syn.pak", pak);
    ASSERT_FALSE(path.empty());
    std::string error;
    const auto pack = mxh::compat::PackFile::open(path, &error);
    ASSERT_NE(pack, nullptr) << error;
    const auto parsed = mxh::compat::ChxModel::parse(read_basename(*pack, "L001.chx"));
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->mod_files.size(), 1u);
    std::size_t bones = 0;
    std::size_t physique_vertices = 0;
    for (const auto& name : parsed->mod_files) {
        mxh::compat::StmStaticModel model;
        ASSERT_TRUE(mxh::compat::parse_mod(read_basename(*pack, name), model, &error))
            << name << ": " << error;
        bones += model.bones.size();
        ASSERT_EQ(model.bones.size(), 1u);
        EXPECT_EQ(model.bones[0].name, "syn_bone");
        EXPECT_EQ(model.bones[0].index, 6u);
        for (const auto& part : model.meshes) {
            for (const auto& influences : part.physique) {
                if (influences.empty()) continue;
                ++physique_vertices;
                ASSERT_EQ(influences.size(), 1u);
                EXPECT_EQ(influences[0].bone_index, 4u);
                EXPECT_FLOAT_EQ(influences[0].weight, 0.75f);
            }
        }
    }
    EXPECT_EQ(bones, 1u);
    EXPECT_EQ(physique_vertices, 1u);
}

// Real L001 character. Bones and physique must be present in that pack.
TEST(StmStaticModel, RealL001BonesAndPhysique) {
    const auto root = mxh::test::require_playdh_root();
    const auto path = root / "monster.pak";
    ASSERT_TRUE(std::filesystem::is_regular_file(path))
        << mxh::test::kPlayDhRootEnv << " is set but monster.pak is missing";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto parsed = mxh::compat::ChxModel::parse(read_basename(*pack, "L001.chx"));
    ASSERT_TRUE(parsed.has_value());
    ASSERT_FALSE(parsed->mod_files.empty());
    std::size_t bones = 0;
    std::size_t physique_vertices = 0;
    for (const auto& name : parsed->mod_files) {
        mxh::compat::StmStaticModel model;
        std::string error;
        ASSERT_TRUE(mxh::compat::parse_mod(read_basename(*pack, name), model, &error))
            << name << ": " << error;
        bones += model.bones.size();
        for (const auto& mesh : model.meshes) {
            physique_vertices += static_cast<std::size_t>(std::count_if(
                mesh.physique.begin(), mesh.physique.end(),
                [](const auto& influences) { return !influences.empty(); }));
        }
    }
    EXPECT_GT(bones, 0u);
    EXPECT_GT(physique_vertices, 0u);
}

// Real default male character: the manifest lists five parts and each part
// has a mesh.
TEST(StmStaticModel, ParsesEveryDefaultMaleCharacterPart) {
    const auto root = mxh::test::require_playdh_root();
    const auto path = root / "Character.pak";
    ASSERT_TRUE(std::filesystem::is_regular_file(path))
        << mxh::test::kPlayDhRootEnv << " is set but Character.pak is missing";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto parsed = mxh::compat::ChxModel::parse(read_basename(*pack, "man.chx"));
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->mod_files.size(), 5u);
    for (const auto& name : parsed->mod_files) {
        mxh::compat::StmStaticModel model;
        std::string error;
        ASSERT_TRUE(mxh::compat::parse_mod(read_basename(*pack, name), model, &error))
            << name << ": " << error;
        EXPECT_FALSE(model.meshes.empty()) << name;
    }
}

TEST(StmStaticModel, ParsesSyntheticScene) {
    mxh::test::HandwrittenMaterial material;
    material.index = 3;
    material.diffuse = 0x01020304u;
    material.transparency = 0.25f;
    material.texture_name = "syn_mat";
    material.flags = 9;
    const auto mesh = triangle_mesh("syn_mesh", 1, 3);
    const auto stm = mxh::test::build_stm(material, mesh, true);
    ASSERT_FALSE(stm.empty());
    const auto pak = mxh::test::build_original96_pak({{"scene.stm", stm}});
    ASSERT_FALSE(pak.empty());
    mxh::test::HandwrittenFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const auto path = dir.write_bytes("syn.pak", pak);
    ASSERT_FALSE(path.empty());
    std::string error;
    const auto pack = mxh::compat::PackFile::open(path, &error);
    ASSERT_NE(pack, nullptr) << error;
    const auto bytes = pack->read("scene.stm");
    ASSERT_EQ(bytes, stm);
    mxh::compat::StmStaticModel model;
    ASSERT_TRUE(mxh::compat::parse_stm(bytes, model, &error)) << error;
    EXPECT_EQ(model.version, 1u);
    ASSERT_EQ(model.materials.size(), 1u);
    EXPECT_EQ(model.materials[0].index, 3u);
    EXPECT_EQ(model.materials[0].diffuse, 0x01020304u);
    EXPECT_FLOAT_EQ(model.materials[0].transparency, 0.25f);
    EXPECT_EQ(model.materials[0].texture_name, "syn_mat");
    EXPECT_EQ(model.materials[0].flags, 9u);
    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].name, "syn_mesh");
    EXPECT_EQ(model.meshes[0].positions.size(), 3u);
    EXPECT_EQ(model.meshes[0].normals.size(), 3u);
    ASSERT_EQ(model.meshes[0].face_groups.size(), 1u);
    EXPECT_EQ(model.meshes[0].face_groups[0].material_index, 3u);
    ASSERT_EQ(model.meshes[0].face_groups[0].indices.size(), 3u);
    std::size_t faces = 0;
    for (const auto& group : model.meshes[0].face_groups) {
        faces += group.indices.size() / 3;
        for (const auto index : group.indices)
            EXPECT_LT(index, model.meshes[0].positions.size());
    }
    EXPECT_EQ(faces, 1u);
    EXPECT_LT(model.collision_offset, bytes.size());
    ASSERT_EQ(model.collision_bounds.size(), 1u);
    EXPECT_FLOAT_EQ(model.collision_bounds[0].min[0], 0.0f);
    EXPECT_FLOAT_EQ(model.collision_bounds[0].min[1], 0.0f);
    EXPECT_FLOAT_EQ(model.collision_bounds[0].min[2], 0.0f);
    EXPECT_FLOAT_EQ(model.collision_bounds[0].max[0], 2.0f);
    EXPECT_FLOAT_EQ(model.collision_bounds[0].max[1], 3.0f);
    EXPECT_FLOAT_EQ(model.collision_bounds[0].max[2], 4.0f);
}

TEST(StmStaticModel, ParsesSyntheticMod) {
    mxh::test::HandwrittenMaterial material;
    material.index = 6;
    material.diffuse = 0x11223344u;
    material.transparency = 0.125f;
    material.texture_name = "syn_sky";
    material.flags = 2;
    const auto mesh = triangle_mesh("syn_sky_mesh", 2, 6);
    const auto mod = mxh::test::build_mod(material, mesh, false, "", 0);
    ASSERT_FALSE(mod.empty());
    const auto pak = mxh::test::build_original96_pak({{"sky_syn.mod", mod}});
    ASSERT_FALSE(pak.empty());
    mxh::test::HandwrittenFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const auto path = dir.write_bytes("syn.pak", pak);
    ASSERT_FALSE(path.empty());
    std::string error;
    const auto pack = mxh::compat::PackFile::open(path, &error);
    ASSERT_NE(pack, nullptr) << error;
    mxh::compat::StmStaticModel model;
    ASSERT_TRUE(mxh::compat::parse_mod(pack->read("sky_syn.mod"), model, &error)) << error;
    ASSERT_EQ(model.materials.size(), 1u);
    EXPECT_EQ(model.materials[0].index, 6u);
    EXPECT_EQ(model.materials[0].texture_name, "syn_sky");
    EXPECT_EQ(model.materials[0].flags, 2u);
    EXPECT_FLOAT_EQ(model.materials[0].transparency, 0.125f);
    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].name, "syn_sky_mesh");
    EXPECT_EQ(model.meshes[0].positions.size(), 3u);
    EXPECT_TRUE(model.bones.empty());
}

TEST(StmStaticModel, ParsesSyntheticCharacterParts) {
    const char* part_names[] = {"syn_part_a.mod", "syn_part_b.mod"};
    std::vector<std::pair<std::string, std::vector<std::uint8_t>>> entries;
    const auto text = mxh::compat::ChxModel::serialize_text(
        {part_names[0], part_names[1]}, {});
    entries.emplace_back("actor.chx", std::vector<std::uint8_t>(text.begin(), text.end()));
    for (int i = 0; i < 2; ++i) {
        mxh::test::HandwrittenMaterial material;
        material.index = static_cast<std::uint32_t>(10 + i);
        material.texture_name = (i == 0) ? "syn_part_a" : "syn_part_b";
        material.flags = static_cast<std::uint32_t>(i);
        const auto mesh = triangle_mesh(material.texture_name, static_cast<std::uint32_t>(i + 1),
                                        material.index);
        const auto mod = mxh::test::build_mod(material, mesh, false, "", 0);
        ASSERT_FALSE(mod.empty()) << part_names[i];
        entries.emplace_back(part_names[i], mod);
    }
    const auto pak = mxh::test::build_original96_pak(std::move(entries));
    ASSERT_FALSE(pak.empty());
    mxh::test::HandwrittenFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const auto path = dir.write_bytes("syn.pak", pak);
    ASSERT_FALSE(path.empty());
    std::string error;
    const auto pack = mxh::compat::PackFile::open(path, &error);
    ASSERT_NE(pack, nullptr) << error;
    const auto parsed = mxh::compat::ChxModel::parse(read_basename(*pack, "actor.chx"));
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->mod_files.size(), 2u);
    for (const auto& name : parsed->mod_files) {
        mxh::compat::StmStaticModel model;
        ASSERT_TRUE(mxh::compat::parse_mod(read_basename(*pack, name), model, &error))
            << name << ": " << error;
        ASSERT_EQ(model.meshes.size(), 1u) << name;
        EXPECT_FALSE(model.meshes[0].positions.empty()) << name;
    }
}
