#include "mxh/render/EntityScene.hpp"

#include "mxh/render/frustum.hpp"

#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/anm_motion.hpp"
#include "mxh/compat/character_appearance_catalog.hpp"
#include "mxh/compat/monster_catalog.hpp"
#include "mxh/compat/npc_chx_catalog.hpp"
#include "mxh/compat/stm_static_model.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/IRenderer.hpp"
#include "dx11/texture_loader.hpp"
#include "dx11/mesh_object.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <optional>
#include <limits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

namespace mxh::gx {
namespace {
using Microsoft::WRL::ComPtr;
constexpr float kSceneScale = kEntitySceneScale;
constexpr float kMapCenter = kEntityMapCenter;

std::uint32_t entityModelKey(SceneEntityType type,
                             std::uint16_t kind) noexcept {
    return (static_cast<std::uint32_t>(type) << 16u) | kind;
}

const char* entityTypeName(SceneEntityType type) noexcept {
    return type == SceneEntityType::Npc ? "npc" : "monster";
}

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
    if (name.empty()) return false;
    // Character MOD files use the original TGA/TIF material names while the
    // runtime profile stores the compiled DDS sibling.  Keep the original
    // name as a fallback (some legacy profiles really do ship the source
    // texture), but try every supported compiled spelling before failing.
    std::vector<std::string> candidates;
    candidates.push_back(name);
    const auto compiled = dx11::compiledTextureName(name);
    if (compiled != name) candidates.push_back(compiled);
    std::string extension = name.size() >= 4 ? name.substr(name.size() - 4) : std::string{};
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension == ".tif") {
        auto dds = name;
        dds.replace(dds.size() - 4, 4, ".dds");
        candidates.push_back(std::move(dds));
    }
    // A small number of converted assets retain the historical duplicated
    // extension (for example foo.tga.tga); normalize that spelling too.
    for (const auto& suffix : {std::string{".tga.tga"}, std::string{".tif.tif"}}) {
        if (name.size() >= suffix.size() &&
            std::equal(suffix.rbegin(), suffix.rend(), name.rbegin(),
                       [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) ==
                                                   std::tolower(static_cast<unsigned char>(b)); })) {
            auto dds = name.substr(0, name.size() - suffix.size()) + ".dds";
            candidates.push_back(std::move(dds));
        }
    }
    std::vector<std::uint8_t> encoded;
    for (const auto& candidate : candidates) {
        if (readFile(storage, candidate.c_str(), encoded)) {
            break;
        }
    }
    if (encoded.empty()) return false;
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

MATRIX4 makeWorldMatrix(float tx, float ty, float tz, float yaw) {
    MATRIX4 world = MatrixIdentity();
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    world._11 = cosine;
    world._13 = -sine;
    world._31 = sine;
    world._33 = cosine;
    world._41 = tx;
    world._42 = ty;
    world._43 = tz;
    return world;
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

std::size_t chooseSceneMotionIndex(SceneEntityType type, bool player,
                                   SceneAction action,
                                   std::size_t motionCount) noexcept {
    if (motionCount == 0) return 0;
    std::size_t requested = 0;
    if (player) {
        if (action == SceneAction::Moving) requested = 2;       // Peace_Run (3)
        else if (action == SceneAction::Dead) requested = 25;  // Die_Normal (26)
    } else if (type == SceneEntityType::Monster) {
        if (action == SceneAction::Moving) requested = 1;      // Walk (2)
        else if (action == SceneAction::Attack) requested = 2; // Attack1 (3)
        else if (action == SceneAction::Dead) requested = 8;   // Die (9)
    }
    return requested < motionCount ? requested : 0;
}

std::uint32_t placeholderArgb(PlaceholderKind kind) noexcept {
    switch (kind) {
        case PlaceholderKind::Npc: return 0xFFFFD700u;
        case PlaceholderKind::Monster: return 0xFFFF4040u;
        case PlaceholderKind::Player:
        default: return 0xFF40E0FFu;
    }
}

void fillPlaceholderOct(const PlaceholderVisual& visual, VECTOR3 oct[8]) noexcept {
    if (!oct) return;
    const float tx = visual.world_x * kEntitySceneScale - kEntityMapCenter;
    const float ty = visual.world_y * kEntitySceneScale;
    const float tz = visual.world_z * kEntitySceneScale - kEntityMapCenter;
    const float r = visual.radius > 0.0f ? visual.radius : 0.5f;
    const float top = ty + 2.0f * r;
    oct[0] = {tx - r, ty, tz - r};
    oct[1] = {tx + r, ty, tz - r};
    oct[2] = {tx + r, ty, tz + r};
    oct[3] = {tx - r, ty, tz + r};
    oct[4] = {tx - r, top, tz - r};
    oct[5] = {tx + r, top, tz - r};
    oct[6] = {tx + r, top, tz + r};
    oct[7] = {tx - r, top, tz + r};
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
        std::vector<std::string> motion_files;
        std::vector<std::optional<mxh::compat::AnmMotion>> motions;
        std::vector<bool> motion_attempted;
        std::size_t active_motion = std::numeric_limits<std::size_t>::max();
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
    std::optional<mxh::compat::NpcChxCatalog> npc_catalog;
    std::array<std::optional<mxh::compat::CharacterAppearanceCatalog>, 2> appearances;
    std::vector<mxh::game::ItemInfo> item_catalog;
    std::unordered_map<std::uint32_t, std::unique_ptr<Model>> models;
    std::unordered_map<std::uint32_t, std::unique_ptr<Model>> effectModels;
    std::unordered_set<std::uint32_t> failed_models;
    std::unordered_map<std::uint32_t, std::unique_ptr<Model>> playerModels;
    std::vector<SceneEntity> instances;
    std::vector<EffectObject> effect_instances;
    std::optional<ScenePlayer> local_player;
    std::vector<ScenePlayer> remote_players;
    std::optional<Frustum> frustum;
    std::uint32_t culled_instances = 0;
    std::uint32_t failed_load_count = 0;
    bool placeholder_rendering_enabled = false;
    std::unordered_set<std::uint32_t> placeholder_ids;
    std::vector<PlaceholderVisual> placeholder_visuals;
    Model* loadModel(std::uint16_t kind,
                     const ScenePlayer* playerInfo = nullptr,
                     SceneEntityType type = SceneEntityType::Monster,
                     std::uint32_t objectId = 0,
                     std::string_view explicitChx = {}) {
        const auto modelKey = entityModelKey(type, kind);
        const bool effectModel = !explicitChx.empty();
        if (playerInfo) {
            if (const auto it = playerModels.find(playerInfo->object_id); it != playerModels.end())
                return it->second.get();
        } else if (effectModel) {
            if (const auto it = effectModels.find(objectId); it != effectModels.end())
                return it->second.get();
        } else {
            if (const auto it = models.find(modelKey); it != models.end())
                return it->second.get();
        }
        if (!effectModel && failed_models.contains(modelKey)) {
            if (objectId != 0) placeholder_ids.insert(objectId);
            return nullptr;
        }

        const auto fail = [&](const char* stage,
                              const std::string& path = {}) -> Model* {
            MLOG_WARN("[entity] model unavailable object=%u type=%s kind=%u path=%s stage=%s",
                      objectId, playerInfo ? "player" : entityTypeName(type),
                      static_cast<unsigned>(kind), path.c_str(), stage);
            ++failed_load_count;
            if (objectId != 0) placeholder_ids.insert(objectId);
            if (!effectModel) failed_models.insert(modelKey);
            return nullptr;
        };

        mxh::compat::MonsterVisual playerVisual;
        std::string faceMod, hairMod;
        const mxh::compat::MonsterVisual* visual = nullptr;
        if (effectModel) {
            playerVisual.kind = kind;
            playerVisual.chx_name = std::string(explicitChx);
            playerVisual.scale = 1.0f;
            visual = &playerVisual;
        } else if (!playerInfo && type == SceneEntityType::Npc) {
            const auto* chxName = npc_catalog ? npc_catalog->find(kind) : nullptr;
            if (!chxName) return fail("NpcChxList.lookup");
            playerVisual.kind = kind;
            playerVisual.chx_name = *chxName;
            playerVisual.scale = 1.0f;
            visual = &playerVisual;
        } else {
            visual = catalog ? catalog->find(kind) : nullptr;
        }
        if (!visual && kind >= 65000u) {
            const auto encoded = static_cast<unsigned>(kind - 65000u);
            const auto gender = std::min(encoded / 25u, 1u);
            const auto face = (encoded % 25u) / 5u;
            const auto hair = encoded % 5u;
            if (appearances[gender]) {
                playerVisual.kind = kind;
                playerVisual.chx_name = appearances[gender]->body.base_object;
                // Character source meshes are authored in decimetres while
                // world coordinates are centimetres; the legacy client
                // applies the corresponding 8x presentation scale here.
                playerVisual.scale = 8.0f;
                if (face < appearances[gender]->faces.size())
                    faceMod = appearances[gender]->faces[face];
                if (hair < appearances[gender]->hairs.size())
                    hairMod = appearances[gender]->hairs[hair];
                visual = &playerVisual;
            }
        }
        if (!visual) return fail("visual_catalog.lookup");
        if (!storage || !renderer) return fail("no_device", visual->chx_name);
        std::vector<std::uint8_t> chxBytes;
        if (!readFile(storage, visual->chx_name.c_str(), chxBytes))
            return fail("CHX.read", visual->chx_name);
        const auto chx = mxh::compat::ChxModel::parse(chxBytes);
        if (!chx) return fail("CHX.parse", visual->chx_name);
        auto model = std::make_unique<Model>();
        model->motion_files = chx->motions;
        model->motions.resize(chx->motions.size());
        model->motion_attempted.resize(chx->motions.size(), false);
        if (!model->motion_files.empty()) {
            std::vector<std::uint8_t> motionBytes;
            model->motion_attempted[0] = true;
            if (readFile(storage, model->motion_files.front().c_str(), motionBytes)) {
                model->motions[0] = mxh::compat::AnmMotion::parse(motionBytes);
            }
        }
        const auto* initialMotion = !model->motions.empty() && model->motions[0]
            ? &*model->motions[0] : nullptr;
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
                const auto equipped = std::count_if(
                    playerInfo->weared_item_idx.begin(),
                    playerInfo->weared_item_idx.end(),
                    [](std::uint16_t item) { return item != 0; });
                MLOG_INFO("[entity] player appearance object=%u gender=%u face=%u hair=%u equipped=%zu resolved_mods=%zu",
                          objectId, static_cast<unsigned>(playerInfo->gender),
                          static_cast<unsigned>(playerInfo->face_type),
                          static_cast<unsigned>(playerInfo->hair_type),
                          equipped, modFiles.size());
            }
        }
        if (modFiles.empty()) return fail("CHX.mod_list.empty", visual->chx_name);
        std::size_t loadedModCount = 0;
        for (const auto& modName : modFiles) {
            std::vector<std::uint8_t> modBytes;
            mxh::compat::StmStaticModel mod;
            std::string error;
            if (!readFile(storage, modName.c_str(), modBytes) ||
                !mxh::compat::parse_mod(modBytes, mod, &error))
                return fail("MOD.load", modName);
            ++loadedModCount;
            std::unordered_map<std::uint32_t, std::array<float, 16>> boneWorld;
            std::function<std::array<float, 16>(const mxh::compat::StmBone&)> resolveBone;
            resolveBone = [&](const mxh::compat::StmBone& bone) {
                if (const auto found = boneWorld.find(bone.index); found != boneWorld.end())
                    return found->second;
                auto world = initialMotion
                    ? composeMotion(bone, initialMotion->find(bone.name), 0.0f)
                    : bone.transform;
                if (initialMotion && bone.parent_index != 0xffffffffu) {
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
            for (std::size_t i = 0; i < mod.materials.size(); ++i) {
                const auto& material = mod.materials[i];
                if (!material.texture_name.empty() &&
                    !loadTexture(storage, device, material.texture_name,
                                 model->textures[materialBase + i]))
                    return fail("texture.load", material.texture_name);
            }
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
                if (auto* dxMesh = dynamic_cast<dx11::MeshObject*>(mesh))
                    dxMesh->setTwoSided(true);
                for (std::uint32_t group = 0; group < materials.size(); ++group) {
                    const auto texture = materialBase + materials[group];
                    if (texture < model->textures.size())
                        mesh->SetFaceGroupDiffuseSRV(group, model->textures[texture].Get());
                }
                model->meshes.push_back(mesh);
                if (!source.physique.empty()) {
                    if (auto* dynamicMesh = dynamic_cast<dx11::MeshObject*>(mesh))
                        model->animated_parts.push_back({dynamicMesh, source, mod.bones});
                }
            }
        }
        if (loadedModCount != modFiles.size())
            return fail("MOD.count", visual->chx_name);
        if (model->meshes.empty()) return fail("MOD.mesh_build", visual->chx_name);
        auto* result = model.get();
        if (playerInfo) playerModels.emplace(playerInfo->object_id, std::move(model));
        else if (effectModel) effectModels.emplace(objectId, std::move(model));
        else models.emplace(modelKey, std::move(model));
        MLOG_INFO("[entity] original model object=%u type=%s kind=%u chx=%s meshes=%u bounds=(%.3f,%.3f,%.3f)",
                  objectId, playerInfo ? "player" : entityTypeName(type),
                  static_cast<unsigned>(kind), visual->chx_name.c_str(),
                  static_cast<unsigned>(result->meshes.size()),
                  result->maximum.x - result->minimum.x,
                  result->maximum.y - result->minimum.y,
                  result->maximum.z - result->minimum.z);
        return result;
    }

    void updateAnimation(Model& model, SceneEntityType type,
                         bool player, SceneAction action,
                         std::optional<std::size_t> forcedMotion = std::nullopt) {
        if (model.motion_files.empty() || model.animated_parts.empty()) return;
        auto motionIndex = forcedMotion
            ? (*forcedMotion < model.motion_files.size() ? *forcedMotion : 0u)
            : chooseSceneMotionIndex(type, player, action, model.motion_files.size());
        const auto loadMotion = [&](std::size_t index)
                -> const mxh::compat::AnmMotion* {
            if (index >= model.motion_files.size()) return nullptr;
            if (!model.motion_attempted[index]) {
                model.motion_attempted[index] = true;
                std::vector<std::uint8_t> bytes;
                if (readFile(storage, model.motion_files[index].c_str(), bytes)) {
                    model.motions[index] = mxh::compat::AnmMotion::parse(bytes);
                }
                if (!model.motions[index]) {
                    MLOG_WARN("[entity] motion unavailable index=%u path=%s",
                              static_cast<unsigned>(index),
                              model.motion_files[index].c_str());
                }
            }
            return model.motions[index] ? &*model.motions[index] : nullptr;
        };
        const auto* selectedMotion = loadMotion(motionIndex);
        if (!selectedMotion && motionIndex != 0) {
            motionIndex = 0;
            selectedMotion = loadMotion(0);
        }
        if (!selectedMotion) return;
        if (model.active_motion != motionIndex) {
            model.active_motion = motionIndex;
            model.animation_started = std::chrono::steady_clock::now();
            model.animation_confirmed = false;
        }
        const auto& motion = *selectedMotion;
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
            MLOG_INFO("[entity] original animation active motion=%u frame=%.2f parts=%u",
                      static_cast<unsigned>(motionIndex), frame,
                      static_cast<unsigned>(model.animated_parts.size()));
        }
    }
};

namespace {
bool samePlayerAppearance(const ScenePlayer& left,
                          const ScenePlayer& right) noexcept {
    return left.gender == right.gender &&
           left.face_type == right.face_type &&
           left.hair_type == right.hair_type &&
           left.weared_item_idx == right.weared_item_idx;
}
}

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
    if (!readFile(storage, "Resource/Client/NpcChxList.bin", bytes)) {
        if (error) *error = "Resource/Client/NpcChxList.bin unavailable";
        return false;
    }
    impl_->npc_catalog = mxh::compat::NpcChxCatalog::parse_bin(bytes);
    if (!impl_->npc_catalog) {
        if (error) *error = "Resource/Client/NpcChxList.bin parse failed";
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
    MLOG_INFO("[entity] original NpcChxList loaded entries=%u",
              static_cast<unsigned>(impl_->npc_catalog->entries().size()));
    return true;
}

void EntityScene::synchronize(const WorldSnapshot& snapshot) {
    std::unordered_map<std::uint32_t, const ScenePlayer*> previousPlayers;
    if (impl_->local_player) {
        previousPlayers.emplace(impl_->local_player->object_id,
                                &*impl_->local_player);
    }
    for (const auto& player : impl_->remote_players) {
        previousPlayers.emplace(player.object_id, &player);
    }

    std::unordered_map<std::uint32_t, const ScenePlayer*> nextPlayers;
    if (snapshot.local_player) {
        nextPlayers.emplace(snapshot.local_player->object_id,
                            &*snapshot.local_player);
    }
    for (const auto& player : snapshot.remote_players) {
        nextPlayers.emplace(player.object_id, &player);
    }

    for (auto it = impl_->playerModels.begin(); it != impl_->playerModels.end();) {
        const auto next = nextPlayers.find(it->first);
        const auto previous = previousPlayers.find(it->first);
        if (next == nextPlayers.end() || previous == previousPlayers.end() ||
            !samePlayerAppearance(*previous->second, *next->second)) {
            it = impl_->playerModels.erase(it);
        } else {
            ++it;
        }
    }

    impl_->local_player = snapshot.local_player;
    impl_->remote_players = snapshot.remote_players;
    impl_->instances = snapshot.entities;
    impl_->placeholder_visuals.clear();
    impl_->placeholder_ids.clear();

    for (const auto& entity : impl_->instances) {
        if (!impl_->loadModel(entity.visual_kind, nullptr, entity.type,
                              entity.object_id)) {
            impl_->placeholder_visuals.push_back(PlaceholderVisual{
                entity.object_id, entity.world_x, entity.world_y,
                entity.world_z, 0.5f,
                entity.type == SceneEntityType::Npc
                    ? PlaceholderKind::Npc
                    : PlaceholderKind::Monster});
            if (entity.object_id != 0) {
                impl_->placeholder_ids.insert(entity.object_id);
            }
        }
    }
    for (const auto& [objectId, player] : nextPlayers) {
        const auto kind = static_cast<std::uint16_t>(
            65000u + std::min<unsigned>(player->gender, 1u) * 25u +
            std::min<unsigned>(player->face_type, 4u) * 5u +
            std::min<unsigned>(player->hair_type, 4u));
        if (!impl_->loadModel(kind, player, SceneEntityType::Monster, objectId)) {
            impl_->placeholder_visuals.push_back(PlaceholderVisual{
                objectId, player->world_x, player->world_y, player->world_z,
                0.5f, PlaceholderKind::Player});
            if (objectId != 0) impl_->placeholder_ids.insert(objectId);
        }
    }
}

void EntityScene::synchronizeEffects(std::span<const EffectObject> effects) {
    std::unordered_set<std::uint32_t> next_ids;
    next_ids.reserve(effects.size());
    for (const auto& effect : effects) next_ids.insert(effect.object_id);
    for (auto it = impl_->effectModels.begin(); it != impl_->effectModels.end();) {
        if (!next_ids.contains(it->first)) it = impl_->effectModels.erase(it);
        else ++it;
    }
    impl_->effect_instances.assign(effects.begin(), effects.end());
}

void EntityScene::clearEffects() noexcept {
    impl_->effect_instances.clear();
    impl_->effectModels.clear();
}

void EntityScene::render() {
    if (!impl_->renderer) return;
    impl_->culled_instances = 0;
    const auto renderPlayer = [this](const ScenePlayer& player,
                                     bool alwaysRender) {
        // Player is always rendered: it sits at the camera's focal point
        // and any p-vertex test would put it right on the near plane,
        // so a strict frustum check risks culling the player out of its
        // own view. The cost is negligible (one model) compared to the
        // NPCs.
        const auto kind = static_cast<std::uint16_t>(65000u + std::min<unsigned>(player.gender, 1u) * 25u +
            std::min<unsigned>(player.face_type, 4u) * 5u + std::min<unsigned>(player.hair_type, 4u));
        if (auto* model = impl_->loadModel(kind, &player)) {
            impl_->updateAnimation(*model, SceneEntityType::Monster,
                                   true, player.action);
            const float tx = player.world_x * kSceneScale - kMapCenter;
            const float ty = player.world_y * kSceneScale - model->minimum.y;
            const float tz = player.world_z * kSceneScale - kMapCenter;
            if (!alwaysRender && impl_->frustum) {
                const float radius = std::max({
                    std::abs(model->minimum.x), std::abs(model->maximum.x),
                    std::abs(model->minimum.z), std::abs(model->maximum.z)});
                const VECTOR3 wmin{tx - radius,
                                   model->minimum.y + ty,
                                   tz - radius};
                const VECTOR3 wmax{tx + radius,
                                   model->maximum.y + ty,
                                   tz + radius};
                if (!impl_->frustum->intersectsAABB(wmin, wmax)) {
                    ++impl_->culled_instances;
                    return;
                }
            }
            MATRIX4 world = makeWorldMatrix(
                tx, ty, tz, player.facing_yaw);
            for (auto* mesh : model->meshes) {
                mesh->SetWorldTransform(&world);
                impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
            }
        }
    };
    if (impl_->local_player) {
        renderPlayer(*impl_->local_player, true);
    }
    for (const auto& player : impl_->remote_players) {
        if (impl_->local_player &&
            player.object_id == impl_->local_player->object_id) continue;
        renderPlayer(player, false);
    }
    for (const auto& entity : impl_->instances) {
        auto* model = impl_->loadModel(entity.visual_kind, nullptr,
                                       entity.type, entity.object_id);
        if (!model) continue;
        // World transform is a pure translation (no rotation, no scale),
        // so the world-space AABB is the local AABB translated by the
        // (x, y, z) world position. Apply the frustum p-vertex test
        // before pushing draw calls to the renderer.
        const float tx = entity.world_x * kSceneScale - kMapCenter;
        const float ty = entity.world_y * kSceneScale - model->minimum.y;
        const float tz = entity.world_z * kSceneScale - kMapCenter;
        if (impl_->frustum) {
            const float radius = std::max({
                std::abs(model->minimum.x), std::abs(model->maximum.x),
                std::abs(model->minimum.z), std::abs(model->maximum.z)});
            const VECTOR3 wmin{tx - radius, model->minimum.y + ty,
                               tz - radius};
            const VECTOR3 wmax{tx + radius, model->maximum.y + ty,
                               tz + radius};
            if (!impl_->frustum->intersectsAABB(wmin, wmax)) {
                ++impl_->culled_instances;
                continue;
            }
        }
        impl_->updateAnimation(*model, entity.type, false, entity.action);
        MATRIX4 world = makeWorldMatrix(
            tx, ty, tz, entity.facing_yaw);
        for (auto* mesh : model->meshes) {
            mesh->SetWorldTransform(&world);
            impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
        }
    }
    for (const auto& effect : impl_->effect_instances) {
        if (effect.chx_name.empty()) continue;
        auto* model = impl_->loadModel(0, nullptr, SceneEntityType::Monster,
                                       effect.object_id, effect.chx_name);
        if (!model) continue;
        const float tx = effect.world_x * kSceneScale - kMapCenter;
        const float ty = effect.world_y * kSceneScale - model->minimum.y;
        const float tz = effect.world_z * kSceneScale - kMapCenter;
        if (impl_->frustum) {
            const float radius = std::max({
                std::abs(model->minimum.x), std::abs(model->maximum.x),
                std::abs(model->minimum.z), std::abs(model->maximum.z)});
            const VECTOR3 wmin{tx - radius, model->minimum.y + ty, tz - radius};
            const VECTOR3 wmax{tx + radius, model->maximum.y + ty, tz + radius};
            if (!impl_->frustum->intersectsAABB(wmin, wmax)) {
                ++impl_->culled_instances;
                continue;
            }
        }
        impl_->updateAnimation(*model, SceneEntityType::Monster, false,
                               SceneAction::Idle,
                               effect.has_motion_index
                                   ? std::optional<std::size_t>(effect.motion_index)
                                   : std::nullopt);
        const MATRIX4 world = makeWorldMatrix(tx, ty, tz, effect.facing_yaw);
        for (auto* mesh : model->meshes) {
            mesh->SetWorldTransform(&world);
            impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0,
                                              nullptr, 0, 0, 0, 0);
        }
    }
    if (!impl_->placeholder_rendering_enabled) return;
    for (const auto& placeholder : impl_->placeholder_visuals) {
        VECTOR3 oct[8]{};
        fillPlaceholderOct(placeholder, oct);
        const bool alwaysRender = impl_->local_player &&
            placeholder.object_id == impl_->local_player->object_id;
        if (!alwaysRender && impl_->frustum) {
            const VECTOR3 wmin{oct[0].x, oct[0].y, oct[0].z};
            const VECTOR3 wmax{oct[6].x, oct[6].y, oct[6].z};
            if (!impl_->frustum->intersectsAABB(wmin, wmax)) {
                ++impl_->culled_instances;
                continue;
            }
        }
        impl_->renderer->RenderBox(oct, placeholderArgb(placeholder.kind));
    }
}

void EntityScene::setPlaceholderRenderingEnabled(bool enabled) noexcept {
    impl_->placeholder_rendering_enabled = enabled;
}

void EntityScene::setCameraFrustum(std::optional<Frustum> frustum) noexcept {
    impl_->frustum = std::move(frustum);
}

std::uint32_t EntityScene::loadedModelCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->models.size() +
                                      impl_->effectModels.size());
}
std::uint32_t EntityScene::instanceCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->instances.size());
}
std::uint32_t EntityScene::playerInstanceCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->remote_players.size()) +
           (impl_->local_player ? 1u : 0u);
}
std::uint32_t EntityScene::npcInstanceCount() const noexcept {
    return static_cast<std::uint32_t>(std::count_if(
        impl_->instances.begin(), impl_->instances.end(),
        [](const SceneEntity& entity) {
            return entity.type == SceneEntityType::Npc;
        }));
}
std::uint32_t EntityScene::culledInstanceCount() const noexcept {
    return impl_->culled_instances;
}
std::uint32_t EntityScene::failedModelCount() const noexcept {
    return impl_->failed_load_count;
}
std::uint32_t EntityScene::unresolvedTextureCount() const noexcept {
    if (!impl_) return 0;
    std::uint32_t count = 0;
    const auto countModel = [&count](const auto& entry) {
        if (!entry.second) return;
        count += static_cast<std::uint32_t>(std::count_if(
            entry.second->textures.begin(), entry.second->textures.end(),
            [](const auto& texture) { return texture == nullptr; }));
    };
    for (const auto& entry : impl_->models) countModel(entry);
    for (const auto& entry : impl_->playerModels) countModel(entry);
    return count;
}
std::uint32_t EntityScene::placeholderCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->placeholder_visuals.size());
}
std::span<const PlaceholderVisual> EntityScene::placeholders() const noexcept {
    return impl_->placeholder_visuals;
}

std::string EntityScene::itemDisplayName(std::uint16_t item_id) const {
    if (!impl_ || item_id == 0) return {};
    const auto it = std::find_if(
        impl_->item_catalog.begin(), impl_->item_catalog.end(),
        [item_id](const mxh::game::ItemInfo& item) {
            return item.ItemIdx == item_id;
        });
    if (it == impl_->item_catalog.end()) return {};
    const auto* begin = it->ItemName;
    const auto* end = std::find(begin, begin + mxh::game::ITEM_MAX_NAME, '\0');
    return std::string(begin, end);
}

std::optional<std::uint16_t> EntityScene::itemIconIndex(
    std::uint16_t item_id) const {
    if (!impl_ || item_id == 0) return std::nullopt;
    const auto it = std::find_if(
        impl_->item_catalog.begin(), impl_->item_catalog.end(),
        [item_id](const mxh::game::ItemInfo& item) {
            return item.ItemIdx == item_id;
        });
    if (it == impl_->item_catalog.end() || it->Image2DNum == 0) {
        return std::nullopt;
    }
    return it->Image2DNum;
}
} // namespace mxh::gx
