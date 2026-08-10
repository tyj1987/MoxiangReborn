#include "mxh/render/StaticScene.hpp"

#include "mxh/compat/stm_static_model.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/IRenderer.hpp"
#include "dx11/texture_loader.hpp"

#include <algorithm>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

namespace mxh::gx {
namespace {
using Microsoft::WRL::ComPtr;
constexpr float kSceneScale = 0.001f;
constexpr float kMapCenter = 25.6f;

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
    if (!readStorageFile(storage, name.c_str(), encoded)) {
        const auto dot = name.find_last_of('.');
        if (dot == std::string::npos) return false;
        name.replace(dot, std::string::npos, ".dds");
        if (!readStorageFile(storage, name.c_str(), encoded)) return false;
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
    I4DyuchiGXRenderer* renderer = nullptr;
    std::vector<IDIMeshObject*> meshes;
    std::vector<ComPtr<ID3D11ShaderResourceView>> textures;
    ~Impl() { for (auto* mesh : meshes) if (mesh) mesh->Release(); }
};

StaticScene::StaticScene() : impl_(std::make_unique<Impl>()) {}
StaticScene::~StaticScene() = default;

bool StaticScene::load(I4DyuchiGXRenderer* renderer, I4DyuchiFileStorage* storage,
                       const char* stm_name, std::string* error) {
    if (!renderer || !storage || !stm_name) return false;
    for (auto* mesh : impl_->meshes) if (mesh) mesh->Release();
    impl_->meshes.clear(); impl_->textures.clear(); impl_->renderer = renderer;
    std::vector<std::uint8_t> bytes;
    mxh::compat::StmStaticModel scene;
    if (!readStorageFile(storage, stm_name, bytes) ||
        !mxh::compat::parse_stm(bytes, scene, error)) return false;

    ID3D11Device* device = nullptr;
    if (!renderer->GetD3DDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)) || !device)
        return false;
    impl_->textures.resize(scene.materials.size());
    for (std::size_t i = 0; i < scene.materials.size(); ++i)
        loadTexture(storage, device, scene.materials[i].texture_name, impl_->textures[i]);

    for (const auto& source : scene.meshes) {
        if (source.positions.empty() || source.positions.size() > 65535u) continue;
        std::vector<VECTOR3> positions(source.positions.size());
        std::vector<VECTOR3> normals(source.positions.size(), VECTOR3{0, 1, 0});
        std::vector<TVERTEX> texcoords(source.positions.size());
        for (std::size_t i = 0; i < source.positions.size(); ++i) {
            positions[i] = {source.positions[i][0] * kSceneScale - kMapCenter,
                            source.positions[i][1] * kSceneScale,
                            source.positions[i][2] * kSceneScale - kMapCenter};
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
    MLOG_INFO("[static] original STM loaded meshes=%u/%u textures=%u/%u",
              meshCount(), static_cast<unsigned>(scene.meshes.size()), loadedTextureCount(),
              static_cast<unsigned>(scene.materials.size()));
    return !impl_->meshes.empty();
}

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
} // namespace mxh::gx
