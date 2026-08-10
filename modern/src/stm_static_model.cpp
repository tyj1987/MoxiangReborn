#include "mxh/compat/stm_static_model.hpp"

#include <cstring>
#include <limits>

namespace mxh::compat {
namespace {
constexpr std::size_t kNameSize = 128;
constexpr std::size_t kMaterialSize = 168;
constexpr std::size_t kBaseHeaderSize = 324;
constexpr std::size_t kMeshHeaderSize = 48;
constexpr std::size_t kFaceHeaderSize = 28;
constexpr std::size_t kTexturePlaneSize32 = 100;

void fail(std::string* error, const char* message) { if (error) *error = message; }

template<class T>
bool read(std::span<const std::uint8_t> bytes, std::size_t offset, T& value) {
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) return false;
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return true;
}

bool advance(std::size_t& cursor, std::size_t count, std::size_t unit,
             std::size_t size) {
    if (count && unit > std::numeric_limits<std::size_t>::max() / count) return false;
    const auto bytes = count * unit;
    if (cursor > size || bytes > size - cursor) return false;
    cursor += bytes;
    return true;
}

std::string fixedString(std::span<const std::uint8_t> bytes,
                        std::size_t offset, std::size_t capacity) {
    std::size_t length = 0;
    while (length < capacity && bytes[offset + length]) ++length;
    return {reinterpret_cast<const char*>(bytes.data() + offset), length};
}
}

bool parse_stm_impl(std::span<const std::uint8_t> bytes, StmStaticModel& output,
                    std::string* error, std::size_t boneRecordSize) {
    output = {};
    std::size_t cursor = 0;
    if (!read(bytes, cursor, output.version) || output.version != 1) {
        fail(error, "unsupported STM version"); return false;
    }
    cursor += 4;
    std::uint32_t materialCount = 0;
    if (!read(bytes, cursor, materialCount) || materialCount > 65536u) {
        fail(error, "invalid STM material count"); return false;
    }
    cursor += 4;
    output.materials.reserve(materialCount);
    for (std::uint32_t i = 0; i < materialCount; ++i) {
        if (cursor > bytes.size() || kMaterialSize > bytes.size() - cursor) {
            fail(error, "truncated STM material table"); return false;
        }
        StmMaterial material;
        read(bytes, cursor + 4, material.index);
        read(bytes, cursor + 12, material.diffuse);
        read(bytes, cursor + 24, material.transparency);
        material.texture_name = fixedString(bytes, cursor + 36, kNameSize);
        read(bytes, cursor + 164, material.flags);
        output.materials.push_back(std::move(material));
        cursor += kMaterialSize;
    }

    std::uint32_t objectCount = 0;
    if (!read(bytes, cursor, objectCount) || objectCount > 65536u) {
        fail(error, "invalid STM object count"); return false;
    }
    cursor += 4;
    output.meshes.reserve(objectCount);
    for (std::uint32_t object = 0; object < objectCount; ++object) {
        if (cursor > bytes.size() || kBaseHeaderSize > bytes.size() - cursor) {
            fail(error, "truncated STM base object"); return false;
        }
        StmMesh mesh;
        read(bytes, cursor, mesh.index);
        std::memcpy(mesh.transform.data(), bytes.data() + cursor + 60, 64);
        std::uint32_t childCount = 0;
        read(bytes, cursor + 188, childCount);
        mesh.name = fixedString(bytes, cursor + 196, kNameSize);
        cursor += kBaseHeaderSize;
        if (!advance(cursor, childCount, 4, bytes.size())) {
            fail(error, "truncated STM child list"); return false;
        }
        if (cursor > bytes.size() || kMeshHeaderSize > bytes.size() - cursor) {
            fail(error, "truncated STM mesh header"); return false;
        }
        std::uint32_t vertexCount = 0, extVertexCount = 0;
        std::uint32_t texcoordCount = 0, faceGroupCount = 0;
        read(bytes, cursor + 4, vertexCount);
        read(bytes, cursor + 12, extVertexCount);
        read(bytes, cursor + 16, texcoordCount);
        read(bytes, cursor + 24, faceGroupCount);
        read(bytes, cursor + 28, mesh.mesh_flags);
        if (vertexCount > 10'000'000u || texcoordCount > 10'000'000u ||
            faceGroupCount > 65536u) {
            fail(error, "implausible STM mesh dimensions"); return false;
        }
        cursor += kMeshHeaderSize;
        mesh.positions.resize(vertexCount);
        const auto positionBytes = static_cast<std::size_t>(vertexCount) * 12;
        if (cursor > bytes.size() || positionBytes > bytes.size() - cursor) {
            fail(error, "truncated STM positions"); return false;
        }
        std::memcpy(mesh.positions.data(), bytes.data() + cursor, positionBytes);
        cursor += positionBytes;
        mesh.texcoords.resize(texcoordCount);
        const auto texcoordBytes = static_cast<std::size_t>(texcoordCount) * 8;
        if (cursor > bytes.size() || texcoordBytes > bytes.size() - cursor) {
            fail(error, "truncated STM texture coordinates"); return false;
        }
        std::memcpy(mesh.texcoords.data(), bytes.data() + cursor, texcoordBytes);
        cursor += texcoordBytes;
        if (!advance(cursor, extVertexCount, 4, bytes.size())) {
            fail(error, "truncated STM extended vertices"); return false;
        }
        mesh.face_groups.reserve(faceGroupCount);
        for (std::uint32_t groupIndex = 0; groupIndex < faceGroupCount; ++groupIndex) {
            if (cursor > bytes.size() || kFaceHeaderSize > bytes.size() - cursor) {
                fail(error, "truncated STM face group"); return false;
            }
            StmFaceGroup group;
            std::uint32_t faceCount = 0, lightUv1 = 0, lightUv2 = 0;
            read(bytes, cursor, group.material_index);
            read(bytes, cursor + 8, faceCount);
            read(bytes, cursor + 20, lightUv1);
            read(bytes, cursor + 24, lightUv2);
            cursor += kFaceHeaderSize;
            if (faceCount > 10'000'000u) { fail(error, "implausible STM face count"); return false; }
            group.indices.resize(static_cast<std::size_t>(faceCount) * 3);
            const auto indexBytes = group.indices.size() * 2;
            if (cursor > bytes.size() || indexBytes > bytes.size() - cursor) {
                fail(error, "truncated STM face indices"); return false;
            }
            std::memcpy(group.indices.data(), bytes.data() + cursor, indexBytes);
            cursor += indexBytes;
            if (!advance(cursor, static_cast<std::size_t>(lightUv1) + lightUv2, 8, bytes.size())) {
                fail(error, "truncated STM light UVs"); return false;
            }
            mesh.face_groups.push_back(std::move(group));
        }
        std::uint32_t physiqueVertices = 0, totalBones = 0;
        if (!read(bytes, cursor, physiqueVertices) || !read(bytes, cursor + 4, totalBones)) {
            fail(error, "truncated STM physique header"); return false;
        }
        cursor += 12;
        const auto physiqueVertexOffset = cursor;
        if (!advance(cursor, physiqueVertices, 5, bytes.size())) {
            fail(error, "truncated STM physique"); return false;
        }
        const auto boneOffset = cursor;
        if (!advance(cursor, totalBones, boneRecordSize, bytes.size())) {
            fail(error, "truncated STM physique"); return false;
        }
        if (physiqueVertices) {
            mesh.physique.resize(physiqueVertices);
            for (std::uint32_t vertex = 0; vertex < physiqueVertices; ++vertex) {
                const auto entry = physiqueVertexOffset + static_cast<std::size_t>(vertex) * 5u;
                const auto influenceCount = bytes[entry];
                std::uint32_t firstInfluence = 0;
                read(bytes, entry + 1, firstInfluence);
                if (firstInfluence > totalBones || influenceCount > totalBones - firstInfluence) {
                    fail(error, "invalid STM physique influence range"); return false;
                }
                auto& influences = mesh.physique[vertex];
                influences.reserve(influenceCount);
                for (std::uint32_t j = 0; j < influenceCount; ++j) {
                    const auto record = boneOffset + static_cast<std::size_t>(firstInfluence + j) * boneRecordSize;
                    StmInfluence influence;
                    read(bytes, record, influence.bone_index);
                    read(bytes, record + 4, influence.weight);
                    std::memcpy(influence.offset.data(), bytes.data() + record + 8, 12);
                    std::memcpy(influence.normal_offset.data(), bytes.data() + record + 20, 12);
                    influences.push_back(influence);
                }
            }
        }
        const auto shade = mesh.mesh_flags & 0x0fu;
        const auto transform = mesh.mesh_flags & 0xf0u;
        if (shade == 3) {
            std::uint32_t planeCount = 0;
            if (!read(bytes, cursor, planeCount)) { fail(error, "truncated STM light texture"); return false; }
            cursor += 12;
            if (!advance(cursor, planeCount, kTexturePlaneSize32, bytes.size())) {
                fail(error, "truncated STM light texture planes"); return false;
            }
        } else if (transform != 0x30u) {
            mesh.normals.resize(vertexCount);
            const auto normalBytes = static_cast<std::size_t>(vertexCount) * 12;
            if (cursor > bytes.size() || normalBytes > bytes.size() - cursor) {
                fail(error, "truncated STM normals"); return false;
            }
            std::memcpy(mesh.normals.data(), bytes.data() + cursor, normalBytes);
            cursor += normalBytes;
        }
        output.meshes.push_back(std::move(mesh));
    }
    output.collision_offset = cursor;
    if (cursor + 4 > bytes.size()) { fail(error, "missing STM collision block"); return false; }
    return true;
}

bool parse_stm(std::span<const std::uint8_t> bytes, StmStaticModel& output,
               std::string* error) {
    return parse_stm_impl(bytes, output, error, 32u);
}

bool parse_mod(std::span<const std::uint8_t> bytes, StmStaticModel& output,
               std::string* error) {
    // FILE_SCENE_HEADER: version/object/material/max object counts.
    if (bytes.size() < 28) { fail(error, "truncated MOD scene header"); return false; }
    std::uint32_t version = 0, objectCount = 0, materialCount = 0;
    read(bytes, 0, version); read(bytes, 4, objectCount); read(bytes, 8, materialCount);
    if ((version != 1 && version != 2) || objectCount > 65536u || materialCount > 65536u) {
        fail(error, "unsupported MOD scene header"); return false;
    }
    std::size_t cursor = 28;
    std::vector<StmMaterial> materials;
    materials.reserve(materialCount);
    for (std::uint32_t i = 0; i < materialCount; ++i) {
        std::uint32_t type = 0, size = 0;
        if (!read(bytes, cursor, type) || !read(bytes, cursor + 4, size) ||
            size < 1444 || cursor + 8u + size > bytes.size()) {
            fail(error, "truncated MOD material"); return false;
        }
        cursor += 8;
        StmMaterial material;
        read(bytes, cursor + 28u + 1280u + 128u, material.index);
        read(bytes, cursor + 4, material.diffuse);
        read(bytes, cursor + 16, material.transparency);
        material.texture_name = fixedString(bytes, cursor + 28 + kNameSize, kNameSize);
        read(bytes, cursor + 28u + 1280u + 128u + 4u, material.flags);
        materials.push_back(std::move(material));
        cursor += size;
    }

    // Repackage mesh payloads into the STM envelope. Both formats use the
    // identical CMeshObject::ReadFile body; MOD merely prefixes type/size.
    std::vector<std::vector<std::uint8_t>> meshPayloads;
    std::vector<StmBone> bones;
    for (std::uint32_t i = 0; i < objectCount; ++i) {
        std::uint32_t type = 0, size = 0;
        if (!read(bytes, cursor, type) || !read(bytes, cursor + 4, size) ||
            cursor + 8u + size > bytes.size()) {
            fail(error, "truncated MOD object"); return false;
        }
        cursor += 8;
        if (type == 0xf4000000u)
            meshPayloads.emplace_back(bytes.begin() + cursor, bytes.begin() + cursor + size);
        else if (type == 0xf5000000u && size >= kBaseHeaderSize) {
            StmBone bone;
            read(bytes, cursor, bone.index);
            read(bytes, cursor + 4, bone.rotation_angle);
            std::memcpy(bone.position.data(), bytes.data() + cursor + 8, 12);
            std::memcpy(bone.rotation_axis.data(), bytes.data() + cursor + 20, 12);
            std::memcpy(bone.scale.data(), bytes.data() + cursor + 32, 12);
            std::memcpy(bone.transform.data(), bytes.data() + cursor + 60, 64);
            read(bytes, cursor + 192, bone.parent_index);
            bone.name = fixedString(bytes, cursor + 196, kNameSize);
            bones.push_back(std::move(bone));
        }
        cursor += size;
    }
    std::vector<std::uint8_t> synthetic;
    const auto appendU32 = [&](std::uint32_t value) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(&value);
        synthetic.insert(synthetic.end(), p, p + 4);
    };
    appendU32(1); appendU32(static_cast<std::uint32_t>(materials.size()));
    for (const auto& material : materials) {
        const auto start = synthetic.size(); synthetic.resize(start + kMaterialSize, 0);
        std::memcpy(synthetic.data() + start + 4, &material.index, 4);
        std::memcpy(synthetic.data() + start + 12, &material.diffuse, 4);
        std::memcpy(synthetic.data() + start + 24, &material.transparency, 4);
        std::memcpy(synthetic.data() + start + 36, material.texture_name.data(),
                    std::min(material.texture_name.size(), kNameSize - 1));
        std::memcpy(synthetic.data() + start + 164, &material.flags, 4);
    }
    appendU32(static_cast<std::uint32_t>(meshPayloads.size()));
    for (const auto& payload : meshPayloads)
        synthetic.insert(synthetic.end(), payload.begin(), payload.end());
    appendU32(0); // empty collision-size slot required by parse_stm
    if (!parse_stm_impl(synthetic, output, error, version >= 2 ? 44u : 32u)) return false;
    output.version = version;
    output.materials = std::move(materials);
    output.bones = std::move(bones);
    return true;
}

} // namespace mxh::compat
