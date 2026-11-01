#pragma once

// Hand-written HFL, STM, MOD, and original-96 pak bytes for unit tests.
// Every value is chosen in this header. Nothing is copied from a PlayDH
// archive. Paks are built in memory and written only under a temp directory
// created here. This header never opens the repository tree.

#include "playdh_env.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mxh::test {

inline std::filesystem::path require_playdh_root() {
    if (const auto root = playdh_dir_override()) return *root;
    throw std::runtime_error(std::string(kPlayDhRootEnv) + " is unset");
}

// Deletes only paths this object created. Never calls remove_all.
class HandwrittenFixtureDir {
public:
    HandwrittenFixtureDir() {
        std::random_device device;
        std::mt19937_64 rng(device());
        std::uniform_int_distribution<std::uint64_t> dist;
        std::ostringstream nonce;
        nonce << std::hex << dist(rng) << dist(rng);
        path_ = std::filesystem::temp_directory_path() /
                ("mxh-handwritten-" + nonce.str());
        std::error_code ec;
        if (std::filesystem::create_directory(path_, ec) && !ec) created_ = true;
    }

    ~HandwrittenFixtureDir() { remove_created(); }

    HandwrittenFixtureDir(const HandwrittenFixtureDir&) = delete;
    HandwrittenFixtureDir& operator=(const HandwrittenFixtureDir&) = delete;

    bool created() const { return created_; }
    const std::filesystem::path& path() const { return path_; }

    std::filesystem::path write_bytes(const std::filesystem::path& relative,
                                      const std::vector<std::uint8_t>& bytes) {
        if (!created_ || relative.empty() || relative.is_absolute()) return {};
        const auto file = (path_ / relative).lexically_normal();
        const auto rel = file.lexically_relative(path_);
        if (rel.empty() || *rel.begin() == "..") return {};
        std::error_code ec;
        if (std::filesystem::exists(file, ec)) return {};
        std::filesystem::path cursor = path_;
        for (const auto& part : relative.parent_path()) {
            cursor /= part;
            if (std::filesystem::is_directory(cursor, ec)) continue;
            if (!std::filesystem::create_directory(cursor, ec) || ec) return {};
            dirs_.push_back(cursor);
        }
        std::ofstream out(file, std::ios::binary);
        if (!out.is_open()) return {};
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        if (!out.good()) return {};
        files_.push_back(file);
        return file;
    }

private:
    void remove_created() {
        if (!created_) return;
        std::error_code ec;
        for (auto it = files_.rbegin(); it != files_.rend(); ++it)
            std::filesystem::remove(*it, ec);
        for (auto it = dirs_.rbegin(); it != dirs_.rend(); ++it)
            std::filesystem::remove(*it, ec);
        std::filesystem::remove(path_, ec);
        files_.clear();
        dirs_.clear();
        created_ = false;
    }

    std::filesystem::path path_;
    std::vector<std::filesystem::path> files_;
    std::vector<std::filesystem::path> dirs_;
    bool created_ = false;
};

inline void append_bytes(std::vector<std::uint8_t>& out,
                         const std::uint8_t* data, std::size_t size) {
    out.insert(out.end(), data, data + size);
}

inline void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    const auto* p = reinterpret_cast<const std::uint8_t*>(&value);
    append_bytes(out, p, 4);
}

inline void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    const auto* p = reinterpret_cast<const std::uint8_t*>(&value);
    append_bytes(out, p, 2);
}

inline void append_f32(std::vector<std::uint8_t>& out, float value) {
    const auto* p = reinterpret_cast<const std::uint8_t*>(&value);
    append_bytes(out, p, 4);
}

inline void append_zeros(std::vector<std::uint8_t>& out, std::size_t count) {
    out.insert(out.end(), count, 0);
}

inline void poke_u32(std::vector<std::uint8_t>& out, std::size_t offset,
                     std::uint32_t value) {
    std::memcpy(out.data() + offset, &value, 4);
}

inline void poke_f32(std::vector<std::uint8_t>& out, std::size_t offset,
                     float value) {
    std::memcpy(out.data() + offset, &value, 4);
}

inline bool ascii_name(std::string_view name) {
    if (name.empty() || name.size() >= 128) return false;
    for (unsigned char c : name) {
        if (c < 0x20 || c > 0x7e) return false;
    }
    return true;
}

struct HandwrittenHfl {
    std::uint32_t height_x = 2;
    std::uint32_t height_z = 2;
    std::uint32_t tile_x = 1;
    std::uint32_t tile_z = 1;
    float width = 400.0f;
    float tile_size = 50.0f;
    std::vector<float> heights;
    std::uint16_t texture_index = 7;
    std::string texture_name = "syn_ground";
    std::vector<std::uint16_t> tiles;
};

// Original 32-bit .hfl disk layout: version, 108-byte desc, heights,
// texture table, tile table. Pointer slots in the desc stay zero.
inline std::vector<std::uint8_t> build_hfl(const HandwrittenHfl& spec) {
    const auto height_count =
        static_cast<std::size_t>(spec.height_x) * spec.height_z;
    const auto tile_count =
        static_cast<std::size_t>(spec.tile_x) * spec.tile_z;
    if (spec.height_x == 0 || spec.height_z == 0) return {};
    if (spec.heights.size() != height_count || spec.tiles.size() != tile_count)
        return {};
    if (!ascii_name(spec.texture_name)) return {};

    std::vector<std::uint8_t> out;
    append_u32(out, 1);
    const auto desc = out.size();
    append_zeros(out, 108);
    poke_u32(out, desc + 44, 1);
    poke_u32(out, desc + 52, spec.height_x);
    poke_u32(out, desc + 56, spec.height_z);
    poke_f32(out, desc + 60, spec.width);
    poke_u32(out, desc + 96, spec.tile_x);
    poke_u32(out, desc + 100, spec.tile_z);
    poke_f32(out, desc + 104, spec.tile_size);
    for (const float height : spec.heights) append_f32(out, height);
    append_u32(out, 1);
    append_u16(out, spec.texture_index);
    const auto name_at = out.size();
    append_zeros(out, 128);
    std::memcpy(out.data() + name_at, spec.texture_name.data(),
                spec.texture_name.size());
    append_u32(out, spec.tile_x);
    append_u32(out, spec.tile_z);
    for (const auto tile : spec.tiles) append_u16(out, tile);
    return out;
}

struct HandwrittenMesh {
    std::uint32_t index = 1;
    std::string name = "syn_mesh";
    std::vector<std::array<float, 3>> positions;
    std::uint32_t material_index = 0;
    std::vector<std::uint16_t> indices;
    bool with_physique = false;
    std::uint32_t physique_bone_index = 0;
    float physique_weight = 1.0f;
    std::array<float, 3> physique_offset{{0.0f, 0.0f, 0.0f}};
    std::array<float, 3> physique_normal{{0.0f, 1.0f, 0.0f}};
};

// One STATIC_MODEL mesh body: 324-byte base object, 48-byte mesh header,
// positions, optional face group, physique header, normals.
inline std::vector<std::uint8_t> build_mesh_body(const HandwrittenMesh& mesh) {
    if (!ascii_name(mesh.name) || mesh.positions.empty()) return {};
    if (mesh.indices.size() % 3u != 0) return {};
    if (mesh.positions.size() > 65535u) return {};

    std::vector<std::uint8_t> out;
    const auto base = out.size();
    append_zeros(out, 324);
    poke_u32(out, base + 0, mesh.index);
    poke_u32(out, base + 188, 0);
    std::memcpy(out.data() + base + 196, mesh.name.data(), mesh.name.size());

    const auto header = out.size();
    append_zeros(out, 48);
    poke_u32(out, header + 4, static_cast<std::uint32_t>(mesh.positions.size()));
    poke_u32(out, header + 12, 0);
    poke_u32(out, header + 16, 0);
    const auto face_groups = mesh.indices.empty() ? 0u : 1u;
    poke_u32(out, header + 24, face_groups);
    poke_u32(out, header + 28, 0);

    for (const auto& position : mesh.positions) {
        append_f32(out, position[0]);
        append_f32(out, position[1]);
        append_f32(out, position[2]);
    }
    if (!mesh.indices.empty()) {
        const auto face = out.size();
        append_zeros(out, 28);
        poke_u32(out, face + 0, mesh.material_index);
        poke_u32(out, face + 8, static_cast<std::uint32_t>(mesh.indices.size() / 3u));
        poke_u32(out, face + 20, 0);
        poke_u32(out, face + 24, 0);
        for (const auto index : mesh.indices) append_u16(out, index);
    }

    if (mesh.with_physique) {
        append_u32(out, 1);
        append_u32(out, 1);
        append_u32(out, 0);
        out.push_back(1);
        append_u32(out, 0);
        append_u32(out, mesh.physique_bone_index);
        append_f32(out, mesh.physique_weight);
        for (const float value : mesh.physique_offset) append_f32(out, value);
        for (const float value : mesh.physique_normal) append_f32(out, value);
    } else {
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
    }
    for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
        append_f32(out, 0.0f);
        append_f32(out, 1.0f);
        append_f32(out, 0.0f);
    }
    return out;
}

struct HandwrittenMaterial {
    std::uint32_t index = 1;
    std::uint32_t diffuse = 0x01020304u;
    float transparency = 0.25f;
    std::string texture_name = "syn_mat";
    std::uint32_t flags = 9;
};

inline std::vector<std::uint8_t> build_stm_material(const HandwrittenMaterial& material) {
    if (!ascii_name(material.texture_name)) return {};
    std::vector<std::uint8_t> out(168, 0);
    poke_u32(out, 4, material.index);
    poke_u32(out, 12, material.diffuse);
    poke_f32(out, 24, material.transparency);
    std::memcpy(out.data() + 36, material.texture_name.data(),
                material.texture_name.size());
    poke_u32(out, 164, material.flags);
    return out;
}

// Eight corners of a hand-written box. min (0,0,0), max (2,3,4).
inline void append_collision_box(std::vector<std::uint8_t>& out) {
    constexpr float kCorners[8][3] = {
        {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, {2.0f, 3.0f, 0.0f},
        {0.0f, 0.0f, 4.0f}, {2.0f, 0.0f, 4.0f}, {0.0f, 3.0f, 4.0f}, {2.0f, 3.0f, 4.0f},
    };
    constexpr std::uint32_t kDescriptor = 136;
    constexpr std::uint32_t kModelHeader = kDescriptor + 4;
    append_u32(out, kModelHeader + kDescriptor);
    append_zeros(out, kDescriptor);
    append_u32(out, 1);
    for (const auto& corner : kCorners) {
        append_f32(out, corner[0]);
        append_f32(out, corner[1]);
        append_f32(out, corner[2]);
    }
    append_zeros(out, kDescriptor - 8u * 12u);
}

inline std::vector<std::uint8_t> build_stm(const HandwrittenMaterial& material,
                                          const HandwrittenMesh& mesh,
                                          bool with_collision) {
    const auto material_bytes = build_stm_material(material);
    const auto mesh_bytes = build_mesh_body(mesh);
    if (material_bytes.empty() || mesh_bytes.empty()) return {};
    std::vector<std::uint8_t> out;
    append_u32(out, 1);
    append_u32(out, 1);
    append_bytes(out, material_bytes.data(), material_bytes.size());
    append_u32(out, 1);
    append_bytes(out, mesh_bytes.data(), mesh_bytes.size());
    if (with_collision) append_collision_box(out);
    else append_u32(out, 0);
    return out;
}

inline std::vector<std::uint8_t> build_mod_material(const HandwrittenMaterial& material) {
    if (!ascii_name(material.texture_name)) return {};
    constexpr std::uint32_t kSize = 1444;
    std::vector<std::uint8_t> out(kSize, 0);
    poke_u32(out, 4, material.diffuse);
    poke_f32(out, 16, material.transparency);
    std::memcpy(out.data() + 156, material.texture_name.data(),
                material.texture_name.size());
    poke_u32(out, 1436, material.index);
    poke_u32(out, 1440, material.flags);
    return out;
}

inline std::vector<std::uint8_t> build_bone_object(std::uint32_t index,
                                                  std::string_view name) {
    if (!ascii_name(name)) return {};
    std::vector<std::uint8_t> out(324, 0);
    poke_u32(out, 0, index);
    poke_f32(out, 32, 1.0f);
    poke_f32(out, 36, 1.0f);
    poke_f32(out, 40, 1.0f);
    poke_u32(out, 192, 0xffffffffu);
    std::memcpy(out.data() + 196, name.data(), name.size());
    return out;
}

// .mod scene: 28-byte header, one material, one mesh object, optional bone.
inline std::vector<std::uint8_t> build_mod(const HandwrittenMaterial& material,
                                          const HandwrittenMesh& mesh,
                                          bool include_bone,
                                          std::string_view bone_name,
                                          std::uint32_t bone_index) {
    const auto material_bytes = build_mod_material(material);
    const auto mesh_bytes = build_mesh_body(mesh);
    if (material_bytes.empty() || mesh_bytes.empty()) return {};
    std::vector<std::uint8_t> bone_bytes;
    if (include_bone) {
        bone_bytes = build_bone_object(bone_index, bone_name);
        if (bone_bytes.empty()) return {};
    }
    std::vector<std::uint8_t> out;
    append_u32(out, 1);
    append_u32(out, include_bone ? 2u : 1u);
    append_u32(out, 1);
    append_zeros(out, 16);
    append_u32(out, 1);
    append_u32(out, static_cast<std::uint32_t>(material_bytes.size()));
    append_bytes(out, material_bytes.data(), material_bytes.size());
    append_u32(out, 0xf4000000u);
    append_u32(out, static_cast<std::uint32_t>(mesh_bytes.size()));
    append_bytes(out, mesh_bytes.data(), mesh_bytes.size());
    if (include_bone) {
        append_u32(out, 0xf5000000u);
        append_u32(out, static_cast<std::uint32_t>(bone_bytes.size()));
        append_bytes(out, bone_bytes.data(), bone_bytes.size());
    }
    return out;
}

// Variant B pak (count at offset 92, entries at 96). Offset 4 stays 0 so
// the 92-byte walk does not also validate. Same field order as
// modern/tools/pak_writer.py. Names are ASCII and sorted by casefold.
inline std::vector<std::uint8_t> build_original96_pak(
    std::vector<std::pair<std::string, std::vector<std::uint8_t>>> entries) {
    if (entries.empty()) return {};
    for (const auto& entry : entries) {
        if (!ascii_name(entry.first)) return {};
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        auto fold = [](std::string text) {
            for (auto& ch : text) {
                const auto c = static_cast<unsigned char>(ch);
                if (c >= 'A' && c <= 'Z') ch = static_cast<char>(c - 'A' + 'a');
            }
            return text;
        };
        const auto fa = fold(a.first);
        const auto fb = fold(b.first);
        if (fa != fb) return fa < fb;
        return a.first < b.first;
    });
    std::vector<std::uint8_t> out(96, 0);
    poke_u32(out, 0, 1);
    poke_u32(out, 92, static_cast<std::uint32_t>(entries.size()));
    for (const auto& entry : entries) {
        const auto start = static_cast<std::uint32_t>(out.size());
        const auto name_len = static_cast<std::uint32_t>(entry.first.size());
        const auto real = static_cast<std::uint32_t>(entry.second.size());
        const auto total = 32u + name_len + 1u + real;
        append_u32(out, total);
        append_u32(out, real);
        append_u32(out, name_len);
        append_u32(out, start);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_bytes(out, reinterpret_cast<const std::uint8_t*>(entry.first.data()),
                     entry.first.size());
        out.push_back(0);
        append_bytes(out, entry.second.data(), entry.second.size());
    }
    return out;
}

}  // namespace mxh::test
