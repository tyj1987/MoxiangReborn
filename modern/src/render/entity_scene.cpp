#include "mxh/render/EntityScene.hpp"

#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/anm_motion.hpp"
#include "mxh/compat/character_appearance_catalog.hpp"
#include "mxh/compat/monster_catalog.hpp"
#include "mxh/compat/stm_static_model.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/IRenderer.hpp"
#include "dx11/texture_loader.hpp"
#include "dx11/mesh_object.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <optional>
#include <limits>
#include <functional>
#include <unordered_map>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

namespace mxh::gx {
namespace {
using Microsoft::WRL::ComPtr;
constexpr float kSceneScale = 0.001f;
constexpr float kMapCenter = 25.6f;

bool readFile(I4DyuchiFileStorage* storage, const char* name,
              std::vector<std::uint8_t>& bytes) {
    void* file = storage->FSOpenFile(const_cast<char*>(name), FSFILE_ACCESSMODE_BINARY);
    if (!file) return false;
    const auto size = storage->FSSeek(file, 0, FSFILE_SEEK_END);
    storage->FSSeek(file, 0, FSFILE_SEEK_SET);
    bytes.resize(size);
    const auto count = size ? storage->FSRead(file, bytes.data(), size) : 0;
    storage->FSCloseFile(file);
    return size != 0 && count == size;
}

bool loadTexture(I4DyuchiFileStorage* storage, ID3D11Device* device,
                 const std::string& name, ComPtr<ID3D11ShaderResourceView>& srv) {
    std::vector<std::uint8_t> encoded;
    if (name.empty()) return false;
    std::string resolved = dx11::compiledTextureName(name);
    if (resolved != name && !readFile(storage, resolved.c_str(), encoded)) resolved = name;
    if (encoded.empty() && !readFile(storage, resolved.c_str(), encoded)) return false;
    const auto decoded = dx11::loadTextureFromMemory(encoded.data(),
        static_cast<std::uint32_t>(encoded.size()));
    if (decoded.pixels.empty()) return false;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = decoded.width; desc.Height = decoded.height; desc.MipLevels = 1;
    desc.ArraySize = 1; desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{decoded.pixels.data(), decoded.width * 4u, 0};
    ComPtr<ID3D11Texture2D> texture;
    return SUCCEEDED(device->CreateTexture2D(&desc, &initial, &texture)) &&
           SUCCEEDED(device->CreateShaderResourceView(texture.Get(), nullptr, &srv));
}

std::array<float, 3> transformPoint(const std::array<float, 3>& p,
                                    const std::array<float, 16>& m) {
    return {p[0]*m[0] + p[1]*m[4] + p[2]*m[8] + m[12],
            p[0]*m[1] + p[1]*m[5] + p[2]*m[9] + m[13],
            p[0]*m[2] + p[1]*m[6] + p[2]*m[10] + m[14]};
}

std::array<float, 16> identityMatrix() {
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
}

std::array<float, 16> multiplyMatrix(const std::array<float, 16>& a,
                                     const std::array<float, 16>& b) {
    std::array<float, 16> result{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                result[row*4 + col] += a[row*4 + k] * b[k*4 + col];
    return result;
}

std::array<float, 16> quaternionMatrix(const std::array<float, 4>& q) {
    const float x=q[0], y=q[1], z=q[2], w=q[3];
    return {1-2*y*y-2*z*z, 2*x*y-2*z*w, 2*x*z+2*y*w, 0,
            2*x*y+2*z*w, 1-2*x*x-2*z*z, 2*y*z-2*x*w, 0,
            2*x*z-2*y*w, 2*y*z+2*x*w, 1-2*x*x-2*y*y, 0,
            0,0,0,1};
}

std::array<float, 16> composeMotion(const mxh::compat::StmBone& bone,
                                    const mxh::compat::AnmMotionObject* track,
                                    float frame) {
    auto scale = bone.scale;
    auto position = bone.position;
    std::array<float, 4> rotation{};
    const float half = -bone.rotation_angle * 0.5f;
    const float sine = std::sin(half);
    rotation = {bone.rotation_axis[0]*sine, bone.rotation_axis[1]*sine,
                bone.rotation_axis[2]*sine, std::cos(half)};
    if (track) {
        scale = track->sampleScale(frame, scale);
        position = track->samplePosition(frame, position);
        rotation = track->sampleRotation(frame, rotation);
    }
    auto scaleMatrix = identityMatrix();
    scaleMatrix[0]=scale[0]; scaleMatrix[5]=scale[1]; scaleMatrix[10]=scale[2];
    auto positionMatrix = identityMatrix();
    positionMatrix[12]=position[0]; positionMatrix[13]=position[1]; positionMatrix[14]=position[2];
    return multiplyMatrix(multiplyMatrix(scaleMatrix, quaternionMatrix(rotation)), positionMatrix);
}

}

struct EntityScene::Impl {
    struct Model {
        struct AnimatedPart {
            dx11::MeshObject* mesh = nullptr;
            mxh::compat::StmMesh source;
            std::vector<mxh::compat::StmBone> bones;
        };
        std::vector<IDIMeshObject*> meshes;
        std::vector<ComPtr<ID3D11ShaderResourceView>> textures;
        std::vector<AnimatedPart> animated_parts;
        std::optional<mxh::compat::AnmMotion> idle_motion;
        std::chrono::steady_clock::time_point animation_started = std::chrono::steady_clock::now();
        bool animation_confirmed = false;
        float visual_scale = 1.0f;
        VECTOR3 minimum{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        VECTOR3 maximum{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
        ~Model() { for (auto* mesh : meshes) if (mesh) mesh->Release(); }
    };
    I4DyuchiGXRenderer* renderer = nullptr;
    I4DyuchiFileStorage* storage = nullptr;
    ID3D11Device* device = nullptr;
    std::optional<mxh::compat::MonsterCatalog> catalog;
    std::array<std::optional<mxh::compat::CharacterAppearanceCatalog>, 2> appearances;
    std::vector<mxh::game::ItemInfo> item_catalog;
    std::unordered_map<std::uint16_t, std::unique_ptr<Model>> models;
    std::unordered_map<std::uint32_t, std::unique_ptr<Model>> playerModels;
    std::vector<SceneEntity> instances;
    std::optional<ScenePlayer> player;

    Model* loadModel(std::uint16_t kind, const ScenePlayer* playerInfo = nullptr) {
        if (playerInfo) {
            if (const auto it = playerModels.find(playerInfo->object_id); it != playerModels.end())
                return it->second.get();
        } else if (const auto it = models.find(kind); it != models.end()) return it->second.get();
        mxh::compat::MonsterVisual playerVisual;
        std::string faceMod, hairMod;
        const auto* visual = catalog ? catalog->find(kind) : nullptr;
        if (!visual && kind >= 65000u) {
            const auto encoded = static_cast<unsigned>(kind - 65000u);
            const auto gender = std::min(encoded / 25u, 1u);
            const auto face = (encoded % 25u) / 5u;
            const auto hair = encoded % 5u;
            if (appearances[gender]) {
                playerVisual.kind = kind;
                playerVisual.chx_name = appearances[gender]->body.base_object;
                playerVisual.scale = 1.0f;
                if (face < appearances[gender]->faces.size())
                    faceMod = appearances[gender]->faces[face];
                if (hair < appearances[gender]->hairs.size())
                    hairMod = appearances[gender]->hairs[hair];
                visual = &playerVisual;
            }
        }
        if (!visual) return nullptr;
        std::vector<std::uint8_t> chxBytes;
        if (!readFile(storage, visual->chx_name.c_str(), chxBytes)) return nullptr;
        const auto chx = mxh::compat::ChxModel::parse(chxBytes);
        if (!chx) return nullptr;
        std::optional<mxh::compat::AnmMotion> idleMotion;
        if (kind >= 65000u && !chx->motions.empty()) {
            std::vector<std::uint8_t> motionBytes;
            if (readFile(storage, chx->motions.front().c_str(), motionBytes))
                idleMotion = mxh::compat::AnmMotion::parse(motionBytes);
        }
        auto model = std::make_unique<Model>();
        model->idle_motion = idleMotion;
        model->visual_scale = visual->scale;
        auto modFiles = chx->mod_files;
        if (kind >= 65000u) {
            if (modFiles.size() > 0 && !hairMod.empty()) modFiles[0] = hairMod;
            if (modFiles.size() > 1 && !faceMod.empty()) modFiles[1] = faceMod;
            if (playerInfo && appearances[std::min<unsigned>(playerInfo->gender, 1u)]) {
                const auto& appearance = *appearances[std::min<unsigned>(playerInfo->gender, 1u)];
                modFiles = mxh::game::resolve_equipped_character_mods(
                    modFiles, appearance.body.mod_files, item_catalog,
                    playerInfo->weared_item_idx);
            }
        }
        for (const auto& modName : modFiles) {
            std::vector<std::uint8_t> modBytes;
            mxh::compat::StmStaticModel mod;
            std::string error;
            if (!readFile(storage, modName.c_str(), modBytes) ||
                !mxh::compat::parse_mod(modBytes, mod, &error)) continue;
            std::unordered_map<std::uint32_t, std::array<float, 16>> boneWorld;
            std::function<std::array<float, 16>(const mxh::compat::StmBone&)> resolveBone;
            resolveBone = [&](const mxh::compat::StmBone& bone) {
                if (const auto found = boneWorld.find(bone.index); found != boneWorld.end())
                    return found->second;
                auto world = idleMotion ? composeMotion(bone, idleMotion->find(bone.name), 0.0f) : bone.transform;
                if (idleMotion && bone.parent_index != 0xffffffffu) {
                    const auto parent = std::find_if(mod.bones.begin(), mod.bones.end(),
                        [&](const auto& value) { return value.index == bone.parent_index; });
                    if (parent != mod.bones.end()) world = multiplyMatrix(world, resolveBone(*parent));
                }
                boneWorld.emplace(bone.index, world);
                return world;
            };
            for (const auto& bone : mod.bones) resolveBone(bone);
            const auto materialBase = model->textures.size();
            model->textures.resize(materialBase + mod.materials.size());
            for (std::size_t i = 0; i < mod.materials.size(); ++i)
                loadTexture(storage, device, mod.materials[i].texture_name,
                            model->textures[materialBase + i]);
            for (const auto& source : mod.meshes) {
                if (source.positions.empty() || source.positions.size() > 65535u) continue;
                std::vector<VECTOR3> positions(source.positions.size());
                std::vector<VECTOR3> normals(source.positions.size(), VECTOR3{0, 1, 0});
                std::vector<TVERTEX> texcoords(source.positions.size());
                for (std::size_t i = 0; i < source.positions.size(); ++i) {
                    auto p = transformPoint(source.positions[i], source.transform);
                    std::array<float, 3> skinnedNormal{};
                    if (i < source.physique.size() && !source.physique[i].empty()) {
                        p = {};
                        for (const auto& influence : source.physique[i]) {
                            const auto bone = std::find_if(mod.bones.begin(), mod.bones.end(),
                                [&](const auto& value) { return value.index == influence.bone_index; });
                            if (bone == mod.bones.end()) continue;
                            const auto& boneMatrix = boneWorld[bone->index];
                            const auto bp = transformPoint(influence.offset, boneMatrix);
                            for (int axis = 0; axis < 3; ++axis)
                                p[axis] += bp[axis] * influence.weight;
                            const auto& m = boneMatrix;
                            const std::array<float, 3> bn{
                                influence.normal_offset[0]*m[0] + influence.normal_offset[1]*m[4] + influence.normal_offset[2]*m[8],
                                influence.normal_offset[0]*m[1] + influence.normal_offset[1]*m[5] + influence.normal_offset[2]*m[9],
                                influence.normal_offset[0]*m[2] + influence.normal_offset[1]*m[6] + influence.normal_offset[2]*m[10]};
                            for (int axis = 0; axis < 3; ++axis)
                                skinnedNormal[axis] += bn[axis] * influence.weight;
                        }
                    }
                    const float scale = kSceneScale * visual->scale;
                    positions[i] = {p[0]*scale, p[1]*scale, p[2]*scale};
                    model->minimum.x = std::min(model->minimum.x, positions[i].x);
                    model->minimum.y = std::min(model->minimum.y, positions[i].y);
                    model->minimum.z = std::min(model->minimum.z, positions[i].z);
                    model->maximum.x = std::max(model->maximum.x, positions[i].x);
                    model->maximum.y = std::max(model->maximum.y, positions[i].y);
                    model->maximum.z = std::max(model->maximum.z, positions[i].z);
                    if (i < source.physique.size() && !source.physique[i].empty())
                        normals[i] = {skinnedNormal[0], skinnedNormal[1], skinnedNormal[2]};
                    else if (i < source.normals.size())
                        normals[i] = {source.normals[i][0], source.normals[i][1], source.normals[i][2]};
                    if (i < source.texcoords.size())
                        texcoords[i] = {source.texcoords[i][0], source.texcoords[i][1]};
                }
                MESH_DESC desc{};
                desc.dwVertexNum = static_cast<std::uint32_t>(positions.size());
                desc.pv3WorldList = positions.data(); desc.pv3NormalLocal = normals.data();
                desc.dwTexVertexNum = static_cast<std::uint32_t>(texcoords.size());
                desc.ptvTexCoordList = texcoords.data();
                auto* mesh = renderer->CreateMeshObject(CMeshFlag(source.mesh_flags));
                if (!mesh || !mesh->StartInitialize(&desc, nullptr, nullptr)) {
                    if (mesh) mesh->Release(); continue;
                }
                std::vector<std::uint32_t> materials;
                for (const auto& group : source.face_groups) {
                    FACE_DESC face{}; face.pIndex = const_cast<std::uint16_t*>(group.indices.data());
                    face.dwFacesNum = static_cast<std::uint32_t>(group.indices.size()/3);
                    face.dwMtlIndex = group.material_index;
                    if (mesh->InsertFaceGroup(&face)) materials.push_back(group.material_index);
                }
                mesh->EndInitialize();
                for (std::uint32_t group = 0; group < materials.size(); ++group) {
                    const auto texture = materialBase + materials[group];
                    if (texture < model->textures.size())
                        mesh->SetFaceGroupDiffuseSRV(group, model->textures[texture].Get());
                }
                model->meshes.push_back(mesh);
                if (idleMotion && !source.physique.empty()) {
                    if (auto* dynamicMesh = dynamic_cast<dx11::MeshObject*>(mesh))
                        model->animated_parts.push_back({dynamicMesh, source, mod.bones});
                }
            }
        }
        if (model->meshes.empty()) return nullptr;
        auto* result = model.get();
        if (playerInfo) playerModels.emplace(playerInfo->object_id, std::move(model));
        else models.emplace(kind, std::move(model));
        MLOG_INFO("[entity] original model kind=%u chx=%s meshes=%u bounds=(%.3f,%.3f,%.3f)",
                  static_cast<unsigned>(kind), visual->chx_name.c_str(),
                  static_cast<unsigned>(result->meshes.size()),
                  result->maximum.x - result->minimum.x,
                  result->maximum.y - result->minimum.y,
                  result->maximum.z - result->minimum.z);
        return result;
    }

    void updateAnimation(Model& model) {
        if (!model.idle_motion || model.animated_parts.empty()) return;
        const auto& motion = *model.idle_motion;
        const auto frameCount = motion.last_frame - motion.first_frame + 1u;
        if (frameCount == 0 || motion.frame_speed == 0) return;
        const float seconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - model.animation_started).count();
        const float frame = static_cast<float>(motion.first_frame) +
            std::fmod(seconds * static_cast<float>(motion.frame_speed),
                      static_cast<float>(frameCount));
        bool updated = false;
        for (auto& part : model.animated_parts) {
            std::unordered_map<std::uint32_t, std::array<float, 16>> boneWorld;
            std::function<std::array<float, 16>(const mxh::compat::StmBone&)> resolveBone;
            resolveBone = [&](const mxh::compat::StmBone& bone) {
                if (const auto found = boneWorld.find(bone.index); found != boneWorld.end())
                    return found->second;
                auto world = composeMotion(bone, motion.find(bone.name), frame);
                if (bone.parent_index != 0xffffffffu) {
                    const auto parent = std::find_if(part.bones.begin(), part.bones.end(),
                        [&](const auto& value) { return value.index == bone.parent_index; });
                    if (parent != part.bones.end()) world = multiplyMatrix(world, resolveBone(*parent));
                }
                boneWorld.emplace(bone.index, world);
                return world;
            };
            for (const auto& bone : part.bones) resolveBone(bone);

            std::vector<VECTOR3> positions(part.source.positions.size());
            std::vector<VECTOR3> normals(part.source.positions.size(), VECTOR3{0, 1, 0});
            for (std::size_t i = 0; i < part.source.positions.size(); ++i) {
                auto p = transformPoint(part.source.positions[i], part.source.transform);
                std::array<float, 3> skinnedNormal{};
                if (i < part.source.physique.size() && !part.source.physique[i].empty()) {
                    p = {};
                    for (const auto& influence : part.source.physique[i]) {
                        const auto bone = std::find_if(part.bones.begin(), part.bones.end(),
                            [&](const auto& value) { return value.index == influence.bone_index; });
                        if (bone == part.bones.end()) continue;
                        const auto& matrix = boneWorld[bone->index];
                        const auto bp = transformPoint(influence.offset, matrix);
                        for (int axis = 0; axis < 3; ++axis)
                            p[axis] += bp[axis] * influence.weight;
                        const std::array<float, 3> transformedNormal{
                            influence.normal_offset[0]*matrix[0] + influence.normal_offset[1]*matrix[4] + influence.normal_offset[2]*matrix[8],
                            influence.normal_offset[0]*matrix[1] + influence.normal_offset[1]*matrix[5] + influence.normal_offset[2]*matrix[9],
                            influence.normal_offset[0]*matrix[2] + influence.normal_offset[1]*matrix[6] + influence.normal_offset[2]*matrix[10]};
                        for (int axis = 0; axis < 3; ++axis)
                            skinnedNormal[axis] += transformedNormal[axis] * influence.weight;
                    }
                    normals[i] = {skinnedNormal[0], skinnedNormal[1], skinnedNormal[2]};
                } else if (i < part.source.normals.size()) {
                    normals[i] = {part.source.normals[i][0], part.source.normals[i][1], part.source.normals[i][2]};
                }
                const float scale = kSceneScale * model.visual_scale;
                positions[i] = {p[0]*scale, p[1]*scale, p[2]*scale};
            }
            updated = part.mesh->updateVertices(positions, normals) || updated;
        }
        if (updated && !model.animation_confirmed && frame >= motion.first_frame + 3.0f) {
            model.animation_confirmed = true;
            MLOG_INFO("[entity] original idle animation active frame=%.2f parts=%u",
                      frame, static_cast<unsigned>(model.animated_parts.size()));
        }
    }
};

EntityScene::EntityScene() : impl_(std::make_unique<Impl>()) {}
EntityScene::~EntityScene() = default;

bool EntityScene::load(I4DyuchiGXRenderer* renderer, I4DyuchiFileStorage* storage,
                       std::string* error) {
    if (!renderer || !storage) return false;
    impl_->renderer = renderer; impl_->storage = storage;
    if (!renderer->GetD3DDevice(__uuidof(ID3D11Device),
            reinterpret_cast<void**>(&impl_->device)) || !impl_->device) return false;
    std::vector<std::uint8_t> bytes;
    if (!readFile(storage, "Resource/MonsterList.bin", bytes)) {
        if (error) *error = "MonsterList.bin unavailable";
        return false;
    }
    impl_->catalog = mxh::compat::MonsterCatalog::parse_bin(bytes);
    if (!impl_->catalog) {
        if (error) *error = "MonsterList.bin parse failed";
        return false;
    }
    for (unsigned gender = 0; gender < 2; ++gender) {
        const char suffix = gender == 0 ? 'M' : 'W';
        const std::string root = "Resource/Client/";
        std::vector<std::uint8_t> bodyBytes, faceBytes, hairBytes;
        if (!readFile(storage, (root + "ModList_" + suffix + ".bin").c_str(), bodyBytes) ||
            !readFile(storage, (root + "FaceList_" + suffix + ".bin").c_str(), faceBytes) ||
            !readFile(storage, (root + "HairList_" + suffix + ".bin").c_str(), hairBytes)) continue;
        auto body = mxh::compat::CharacterAppearanceCatalog::parse_mod_list_bin(bodyBytes);
        auto faces = mxh::compat::CharacterAppearanceCatalog::parse_part_list_bin(faceBytes);
        auto hairs = mxh::compat::CharacterAppearanceCatalog::parse_part_list_bin(hairBytes);
        if (body && faces && hairs) {
            mxh::compat::CharacterAppearanceCatalog appearance;
            appearance.body = std::move(*body);
            appearance.faces = std::move(*faces);
            appearance.hairs = std::move(*hairs);
            impl_->appearances[gender] = std::move(appearance);
        }
    }
    std::vector<std::uint8_t> itemBytes;
    if (readFile(storage, "Resource/Client/ItemList.bin", itemBytes) ||
        readFile(storage, "Resource/ItemList.bin", itemBytes)) {
        const auto parsed = mxh::game::parse_item_list_bytes(itemBytes);
        impl_->item_catalog = parsed.items;
        MLOG_INFO("[entity] original ItemList loaded entries=%u errors=%u",
                  static_cast<unsigned>(parsed.items.size()), parsed.parse_errors);
    }
    MLOG_INFO("[entity] original MonsterList loaded entries=%u",
              static_cast<unsigned>(impl_->catalog->entries().size()));
    return true;
}

void EntityScene::synchronize(std::span<const SceneEntity> entities) {
    impl_->instances.assign(entities.begin(), entities.end());
    for (const auto& entity : impl_->instances) impl_->loadModel(entity.visual_kind);
}

void EntityScene::synchronizePlayer(const ScenePlayer& player) {
    if (impl_->player && (impl_->player->object_id != player.object_id ||
        impl_->player->gender != player.gender ||
        impl_->player->face_type != player.face_type ||
        impl_->player->hair_type != player.hair_type ||
        impl_->player->weared_item_idx != player.weared_item_idx)) {
        impl_->playerModels.erase(impl_->player->object_id);
    }
    impl_->player = player;
    const auto kind = static_cast<std::uint16_t>(65000u + std::min<unsigned>(player.gender, 1u) * 25u +
        std::min<unsigned>(player.face_type, 4u) * 5u + std::min<unsigned>(player.hair_type, 4u));
    impl_->loadModel(kind, &player);
}

void EntityScene::render() {
    if (!impl_->renderer) return;
    if (impl_->player) {
        const auto& player = *impl_->player;
        const auto kind = static_cast<std::uint16_t>(65000u + std::min<unsigned>(player.gender, 1u) * 25u +
            std::min<unsigned>(player.face_type, 4u) * 5u + std::min<unsigned>(player.hair_type, 4u));
        if (auto* model = impl_->loadModel(kind, &player)) {
            impl_->updateAnimation(*model);
            MATRIX4 world = MatrixIdentity();
            world._41 = player.world_x * kSceneScale - kMapCenter;
            world._42 = player.world_y * kSceneScale - model->minimum.y;
            world._43 = player.world_z * kSceneScale - kMapCenter;
            for (auto* mesh : model->meshes) {
                mesh->SetWorldTransform(&world);
                impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
            }
        }
    }
    for (const auto& entity : impl_->instances) {
        auto* model = impl_->loadModel(entity.visual_kind);
        if (!model) continue;
        MATRIX4 world = MatrixIdentity();
        world._41 = entity.world_x * kSceneScale - kMapCenter;
        world._42 = entity.world_y * kSceneScale - model->minimum.y;
        world._43 = entity.world_z * kSceneScale - kMapCenter;
        for (auto* mesh : model->meshes) {
            mesh->SetWorldTransform(&world);
            impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
        }
    }
}

std::uint32_t EntityScene::loadedModelCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->models.size());
}
std::uint32_t EntityScene::instanceCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->instances.size());
}
} // namespace mxh::gx
