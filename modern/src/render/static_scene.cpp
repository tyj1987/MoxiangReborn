#include "mxh/render/StaticScene.hpp"
#include "mxh/render/StaticSceneCenter.hpp"

#include "mxh/compat/stm_static_model.hpp"
#include "mxh/compat/stm_vertex_color.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/IRenderer.hpp"
#include "dx11/texture_loader.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

namespace mxh::gx {
namespace {
using Microsoft::WRL::ComPtr;
constexpr float kSceneScale = 0.001f;
// The map centre is supplied by the caller (TerrainScene::mapCenter()) via
// StaticScene::load(..., centre) or setMapCenter() BEFORE load. Vertices are
// baked into GPU meshes at load time, so a centre change after load cannot
// move the meshes; see setMapCenter.

bool readStorageFile(I4DyuchiFileStorage* storage, const char* name,
                     std::vector<std::uint8_t>& bytes) {
    void* file = storage->FSOpenFile(const_cast<char*>(name), FSFILE_ACCESSMODE_BINARY);
    if (!file) return false;
    const auto size = storage->FSSeek(file, 0, FSFILE_SEEK_END);
    storage->FSSeek(file, 0, FSFILE_SEEK_SET);
    bytes.resize(size);
    const auto read = size ? storage->FSRead(file, bytes.data(), size) : 0;
    storage->FSCloseFile(file);
    return size != 0 && read == size;
}

bool loadTexture(I4DyuchiFileStorage* storage, ID3D11Device* device,
                 std::string name, ComPtr<ID3D11ShaderResourceView>& srv) {
    if (name.empty()) return false;
    std::vector<std::uint8_t> encoded;
    std::vector<std::string> candidates;
    candidates.push_back(name);
    const auto addDdsVariant = [&](std::string candidate) {
        if (candidate.size() >= 4 &&
            candidate.compare(candidate.size() - 4, 4, ".tga") == 0)
            candidate.replace(candidate.size() - 4, 4, ".dds");
        else if (candidate.size() >= 4 &&
                 candidate.compare(candidate.size() - 4, 4, ".tif") == 0)
            candidate.replace(candidate.size() - 4, 4, ".dds");
        else {
            const auto dot = candidate.find_last_of('.');
            if (dot == std::string::npos) return;
            candidate.replace(dot, std::string::npos, ".dds");
        }
        if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end())
            candidates.push_back(std::move(candidate));
    };
    addDdsVariant(name);
    // A few STM exports carry the authoring extension twice (for example
    // `foo.tga.tga`).  Strip one layer before generating the engine DDS name;
    // this remains an exact package lookup and never invents pixels.
    if (name.size() >= 8 && name.compare(name.size() - 8, 8, ".tga.tga") == 0)
        addDdsVariant(name.substr(0, name.size() - 4));
    for (const auto& candidate : candidates) {
        if (readStorageFile(storage, candidate.c_str(), encoded)) {
            name = candidate;
            break;
        }
    }
    if (encoded.empty()) {
        return false;
    }
    const auto decoded = dx11::loadTextureFromMemory(encoded.data(),
        static_cast<std::uint32_t>(encoded.size()));
    if (decoded.pixels.empty()) return false;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = decoded.width; desc.Height = decoded.height;
    desc.MipLevels = 1; desc.ArraySize = 1; desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{decoded.pixels.data(), decoded.width * 4u, 0};
    ComPtr<ID3D11Texture2D> texture;
    return SUCCEEDED(device->CreateTexture2D(&desc, &initial, &texture)) &&
           SUCCEEDED(device->CreateShaderResourceView(texture.Get(), nullptr, &srv));
}
}

struct StaticScene::Impl {
    struct Bounds { float min_x, max_x, min_z, max_z; };
    I4DyuchiGXRenderer* renderer = nullptr;
    std::vector<IDIMeshObject*> meshes;
    std::vector<ComPtr<ID3D11ShaderResourceView>> textures;
    std::uint32_t unresolved_textures = 0;
    std::vector<Bounds> collision_bounds;
    // Cached (X, Z) half-extent in scaled world units. Pushed by
    // setMapCenter() once per frame from TerrainScene::mapCenter() so the
    // STM mesh re-centres by the same offset the terrain mesh builder
    // subtracts. Defaults to the legacy 25.6f constant for unit tests that
    // build a StaticScene without a corresponding TerrainScene.
    float map_center_x = static_center::kLegacyMapCenter;
    float map_center_z = static_center::kLegacyMapCenter;
    bool baked = false;
    bool vcl_mismatch = false;
    bool vcl_missing = false;
    bool vcl_path = false;
    ~Impl() { for (auto* mesh : meshes) if (mesh) mesh->Release(); }
};

StaticScene::StaticScene() : impl_(std::make_unique<Impl>()) {}
StaticScene::~StaticScene() = default;

bool StaticScene::load(I4DyuchiGXRenderer* renderer, I4DyuchiFileStorage* storage,
                       const char* stm_name, float map_center_x, float map_center_z,
                       std::string* error) {
    if (!impl_) return false;
    impl_->map_center_x = map_center_x;
    impl_->map_center_z = map_center_z;
    return load(renderer, storage, stm_name, error);
}

bool StaticScene::load(I4DyuchiGXRenderer* renderer, I4DyuchiFileStorage* storage,
                       const char* stm_name, std::string* error) {
    if (!renderer || !storage || !stm_name) return false;
    const static_center::Center centre{impl_->map_center_x, impl_->map_center_z};
    for (auto* mesh : impl_->meshes) if (mesh) mesh->Release();
    impl_->meshes.clear(); impl_->textures.clear();
    impl_->collision_bounds.clear();
    impl_->renderer = renderer;
    impl_->unresolved_textures = 0;
    impl_->vcl_mismatch = false;
    impl_->vcl_missing = false;
    impl_->vcl_path = false;
    std::vector<std::uint8_t> bytes;
    mxh::compat::StmStaticModel scene;
    if (!readStorageFile(storage, stm_name, bytes) ||
        !mxh::compat::parse_stm(bytes, scene, error)) return false;
    impl_->collision_bounds.reserve(scene.collision_bounds.size());
    for (const auto& source : scene.collision_bounds) {
        impl_->collision_bounds.push_back({
            static_center::worldToSceneX(source.min[0], centre),
            static_center::worldToSceneX(source.max[0], centre),
            static_center::worldToSceneZ(source.min[2], centre),
            static_center::worldToSceneZ(source.max[2], centre)});
    }

    ID3D11Device* device = nullptr;
    if (!renderer->GetD3DDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)) || !device)
        return false;
    impl_->textures.resize(scene.materials.size());
    for (std::size_t i = 0; i < scene.materials.size(); ++i) {
        // STM permits an empty material texture for geometry that is
        // intentionally vertex-colored/untextured. It is a valid legacy
        // material, not a missing asset and must not trigger a placeholder
        // or release-load failure.
        if (scene.materials[i].texture_name.empty()) continue;
        if (!loadTexture(storage, device, scene.materials[i].texture_name,
                         impl_->textures[i])) {
            ++impl_->unresolved_textures;
            MLOG_WARN("[static] texture unavailable: %s",
                      scene.materials[i].texture_name.c_str());
        }
    }

    // Original-look (default): legacy CVLSMeshObject lighting from <map>.vcl
    // (CoStaticModel.cpp:1151-1155/1184-1186, VLSMeshObject.cpp:75-78,154).
    // MXH_STM_LIGHTING=fixed keeps the modern fixed directional lighting as
    // a non-original option.
    const char* lightingMode = std::getenv("MXH_STM_LIGHTING");
    impl_->vcl_path = !(lightingMode && std::strcmp(lightingMode, "fixed") == 0);
    mxh::compat::StmVclAssignment vcl;
    if (impl_->vcl_path) {
        std::string vclName(stm_name);
        const auto dot = vclName.find_last_of('.');
        vclName = (dot == std::string::npos ? vclName : vclName.substr(0, dot)) + ".vcl";
        std::vector<mxh::compat::StmVclMeshInfo> infos;
        infos.reserve(scene.meshes.size());
        for (const auto& m : scene.meshes) infos.push_back({m.mesh_flags, m.positions.size()});
        std::vector<std::uint8_t> vclBytes;
        const bool present = readStorageFile(storage, vclName.c_str(), vclBytes);
        vcl = mxh::compat::assign_stm_vertex_colors(infos, present ? &vclBytes : nullptr);
        std::string source;
        if (present) {
            if (auto* fs = dynamic_cast<FilesystemFileStorage*>(storage))
                source = fs->describeSource(vclName.c_str());
            if (source.empty()) source = "storage:" + vclName;
        } else {
            source = "missing:" + vclName;
        }
        MLOG_INFO("[static] vcl source=%s bytes=%u vertices=%u matched=%s",
                  source.c_str(), static_cast<unsigned>(vcl.loaded_bytes),
                  static_cast<unsigned>(vcl.expected_vertices),
                  vcl.matched() ? "yes" : "no");
        if (vcl.status == mxh::compat::StmVclStatus::Missing) {
            impl_->vcl_missing = true;
            MLOG_WARN("[static] vcl missing: %s (vertex colours default to white, as legacy memset 0xff)",
                      vclName.c_str());
        } else if (vcl.status == mxh::compat::StmVclStatus::Mismatch) {
            impl_->vcl_mismatch = true;
            MLOG_ERROR("[static] vcl size mismatch: %s bytes=%u expected=%u (vertices*4); "
                       "vertex colours disabled for this map (white)",
                       vclName.c_str(), static_cast<unsigned>(vcl.loaded_bytes),
                       static_cast<unsigned>(vcl.expected_vertices * 4u));
        }
    } else {
        MLOG_INFO("[static] vcl skipped: MXH_STM_LIGHTING=fixed (non-original fixed lighting)");
    }

    for (std::size_t meshIndex = 0; meshIndex < scene.meshes.size(); ++meshIndex) {
        const auto& source = scene.meshes[meshIndex];
        if (source.positions.empty() || source.positions.size() > 65535u) continue;
        std::vector<VECTOR3> positions(source.positions.size());
        std::vector<VECTOR3> normals(source.positions.size(), VECTOR3{0, 1, 0});
        std::vector<TVERTEX> texcoords(source.positions.size());
        for (std::size_t i = 0; i < source.positions.size(); ++i) {
            positions[i] = {static_center::worldToSceneX(source.positions[i][0], centre),
                            source.positions[i][1] * kSceneScale,
                            static_center::worldToSceneZ(source.positions[i][2], centre)};
            if (i < source.normals.size())
                normals[i] = {source.normals[i][0], source.normals[i][1], source.normals[i][2]};
            if (i < source.texcoords.size())
                texcoords[i] = {source.texcoords[i][0], source.texcoords[i][1]};
        }
        MESH_DESC desc{};
        desc.dwVertexNum = static_cast<std::uint32_t>(positions.size());
        desc.pv3WorldList = positions.data(); desc.pv3NormalLocal = normals.data();
        desc.dwTexVertexNum = static_cast<std::uint32_t>(texcoords.size());
        desc.ptvTexCoordList = texcoords.data();
        // Only vcl-consuming (non-LM, non-billboard) meshes get a colour
        // stream; per_mesh is white unless the .vcl size matched exactly.
        std::vector<std::uint32_t> colors;
        if (impl_->vcl_path && meshIndex < vcl.per_mesh.size() &&
            vcl.per_mesh[meshIndex].size() == positions.size()) {
            colors = vcl.per_mesh[meshIndex];
            desc.pVertexColor = colors.data();
        }
        auto* mesh = renderer->CreateMeshObject(CMeshFlag(source.mesh_flags));
        if (!mesh || !mesh->StartInitialize(&desc, nullptr, nullptr)) {
            if (mesh) mesh->Release(); continue;
        }
        std::vector<std::uint32_t> materials;
        for (const auto& sourceGroup : source.face_groups) {
            if (sourceGroup.indices.empty()) continue;
            const bool valid = std::all_of(sourceGroup.indices.begin(), sourceGroup.indices.end(),
                [&](std::uint16_t index) { return index < positions.size(); });
            if (!valid) continue;
            FACE_DESC face{};
            face.pIndex = const_cast<std::uint16_t*>(sourceGroup.indices.data());
            face.dwFacesNum = static_cast<std::uint32_t>(sourceGroup.indices.size() / 3);
            face.dwMtlIndex = sourceGroup.material_index;
            if (mesh->InsertFaceGroup(&face)) materials.push_back(sourceGroup.material_index);
        }
        mesh->EndInitialize();
        for (std::uint32_t group = 0; group < materials.size(); ++group)
            if (materials[group] < impl_->textures.size())
                mesh->SetFaceGroupDiffuseSRV(group, impl_->textures[materials[group]].Get());
        impl_->meshes.push_back(mesh);
    }
    impl_->baked = true;
    MLOG_INFO("[static] original STM loaded centre=(%.3f,%.3f) meshes=%u/%u textures=%u/%u",
              centre.x, centre.z,
              meshCount(), static_cast<unsigned>(scene.meshes.size()), loadedTextureCount(),
              static_cast<unsigned>(scene.materials.size()));
    return !impl_->meshes.empty();
}

bool StaticScene::vertexColorMismatch() const noexcept { return impl_->vcl_mismatch; }
bool StaticScene::vertexColorMissing() const noexcept { return impl_->vcl_missing; }
bool StaticScene::vertexColorPathEnabled() const noexcept { return impl_->vcl_path; }

void StaticScene::render() {
    if (!impl_->renderer) return;
    for (auto* mesh : impl_->meshes)
        impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
}

std::uint32_t StaticScene::meshCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->meshes.size());
}
std::uint32_t StaticScene::loadedTextureCount() const noexcept {
    return static_cast<std::uint32_t>(std::count_if(impl_->textures.begin(), impl_->textures.end(),
        [](const auto& texture) { return texture != nullptr; }));
}
std::uint32_t StaticScene::unresolvedTextureCount() const noexcept {
    return impl_ ? impl_->unresolved_textures : 0;
}

bool StaticScene::blocksPoint(float world_x, float world_z,
                              float radius) const noexcept {
    if (!impl_ || impl_->collision_bounds.empty()) return false;
    const static_center::Center centre{impl_->map_center_x, impl_->map_center_z};
    const float scene_x = static_center::worldToSceneX(world_x, centre);
    const float scene_z = static_center::worldToSceneZ(world_z, centre);
    const float r = std::max(0.0f, radius) * kSceneScale;
    return std::any_of(impl_->collision_bounds.begin(),
                       impl_->collision_bounds.end(),
                       [scene_x, scene_z, r](const Impl::Bounds& b) {
        return scene_x >= b.min_x - r && scene_x <= b.max_x + r &&
               scene_z >= b.min_z - r && scene_z <= b.max_z + r;
    });
}

void StaticScene::setMapCenter(float world_x, float world_z) noexcept {
    if (!impl_) return;
    const static_center::Center next{world_x, world_z};
    const static_center::Center prev{impl_->map_center_x, impl_->map_center_z};
    if (static_center::sameCenter(prev, next)) return;
    if (impl_->baked) {
        // Meshes were baked with `prev`; changing the centre here would make
        // blocksPoint() disagree with the rendered geometry. Keep the baked
        // centre and report the wiring error instead of silently drifting.
        MLOG_WARN("[static] setMapCenter(%.3f,%.3f) after load ignored; baked centre=(%.3f,%.3f)."
                  " Pass TerrainScene::mapCenter() to StaticScene::load.",
                  world_x, world_z, prev.x, prev.z);
        return;
    }
    impl_->map_center_x = world_x;
    impl_->map_center_z = world_z;
}
} // namespace mxh::gx
