#include "mxh/render/SkyScene.hpp"

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
constexpr float kSkyScale = 0.04f;

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
                 const std::string& name, ComPtr<ID3D11ShaderResourceView>& srv) {
    if (name.empty()) return false;
    std::vector<std::uint8_t> encoded;
    if (!readStorageFile(storage, name.c_str(), encoded)) return false;
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

std::array<float, 3> transformPoint(const std::array<float, 3>& p,
                                    const std::array<float, 16>& m) {
    return {
        p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12],
        p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
        p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]
    };
}
}

struct SkyScene::Impl {
    I4DyuchiGXRenderer* renderer = nullptr;
    std::vector<IDIMeshObject*> meshes;
    std::vector<ComPtr<ID3D11ShaderResourceView>> textures;
    ~Impl() { for (auto* mesh : meshes) if (mesh) mesh->Release(); }
};

SkyScene::SkyScene() : impl_(std::make_unique<Impl>()) {}
SkyScene::~SkyScene() = default;

bool SkyScene::load(I4DyuchiGXRenderer* renderer, I4DyuchiFileStorage* storage,
                    const char* mod_name, std::string* error) {
    if (!renderer || !storage || !mod_name) return false;
    for (auto* mesh : impl_->meshes) if (mesh) mesh->Release();
    impl_->meshes.clear(); impl_->textures.clear(); impl_->renderer = renderer;
    std::vector<std::uint8_t> bytes;
    mxh::compat::StmStaticModel scene;
    if (!readStorageFile(storage, mod_name, bytes) ||
        !mxh::compat::parse_mod(bytes, scene, error)) return false;

    ID3D11Device* device = nullptr;
    if (!renderer->GetD3DDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)) || !device)
        return false;
    impl_->textures.resize(scene.materials.size());
    for (std::size_t i = 0; i < scene.materials.size(); ++i)
        loadTexture(storage, device, scene.materials[i].texture_name, impl_->textures[i]);

    for (const auto& source : scene.meshes) {
        if (source.positions.empty() || source.positions.size() > 65535u) continue;
        std::vector<VECTOR3> positions(source.positions.size());
        std::vector<VECTOR3> normals(source.positions.size(), VECTOR3{0, -1, 0});
        std::vector<TVERTEX> texcoords(source.positions.size());
        for (std::size_t i = 0; i < source.positions.size(); ++i) {
            const auto p = transformPoint(source.positions[i], source.transform);
            positions[i] = {p[0] * kSkyScale, p[1] * kSkyScale, p[2] * kSkyScale};
            if (i < source.normals.size())
                normals[i] = {-source.normals[i][0], -source.normals[i][1], -source.normals[i][2]};
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
            std::vector<std::uint16_t> reversed = sourceGroup.indices;
            for (std::size_t i = 0; i + 2 < reversed.size(); i += 3)
                std::swap(reversed[i + 1], reversed[i + 2]);
            FACE_DESC face{};
            face.pIndex = reversed.data();
            face.dwFacesNum = static_cast<std::uint32_t>(reversed.size() / 3);
            face.dwMtlIndex = sourceGroup.material_index;
            if (mesh->InsertFaceGroup(&face)) materials.push_back(sourceGroup.material_index);
        }
        mesh->EndInitialize();
        for (std::uint32_t group = 0; group < materials.size(); ++group)
            if (materials[group] < impl_->textures.size())
                mesh->SetFaceGroupDiffuseSRV(group, impl_->textures[materials[group]].Get());
        impl_->meshes.push_back(mesh);
    }
    MLOG_INFO("[sky] original MOD loaded meshes=%u/%u textures=%u/%u",
              meshCount(), static_cast<unsigned>(scene.meshes.size()), loadedTextureCount(),
              static_cast<unsigned>(scene.materials.size()));
    return !impl_->meshes.empty();
}

void SkyScene::render() {
    if (!impl_->renderer) return;
    for (auto* mesh : impl_->meshes)
        impl_->renderer->RenderMeshObject(mesh, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
}

std::uint32_t SkyScene::meshCount() const noexcept {
    return static_cast<std::uint32_t>(impl_->meshes.size());
}
std::uint32_t SkyScene::loadedTextureCount() const noexcept {
    return static_cast<std::uint32_t>(std::count_if(impl_->textures.begin(), impl_->textures.end(),
        [](const auto& texture) { return texture != nullptr; }));
}
} // namespace mxh::gx
