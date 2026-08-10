#include "mxh/render/TerrainScene.hpp"

#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/render/IFileStorage.hpp"
#include "mxh/render/IRenderer.hpp"
#include "dx11/texture_loader.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

namespace mxh::gx {
namespace {
using Microsoft::WRL::ComPtr;
constexpr float kSceneScale = 0.001f;

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

MATRIX4 fromColumnMajor(const float values[16]) {
    MATRIX4 result{};
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            result.m[row][column] = values[column * 4 + row];
    return result;
}

TVERTEX rotatedUv(float u, float v, std::uint32_t rotation) {
    switch (rotation & 3u) {
    case 1: return {1.0f - v, u};
    case 2: return {1.0f - u, 1.0f - v};
    case 3: return {v, 1.0f - u};
    default: return {u, v};
    }
}
}

struct TerrainScene::Impl {
    I4DyuchiGXRenderer* renderer = nullptr;
    std::vector<IDIMeshObject*> chunks;
    std::vector<ComPtr<ID3D11ShaderResourceView>> textures;
    mxh::compat::HflHeightField terrain;
    bool follow_player = false;
    float player_x = 0;
    float player_z = 0;

    ~Impl() {
        for (auto* chunk : chunks) if (chunk) chunk->Release();
    }
};

TerrainScene::TerrainScene() : impl_(std::make_unique<Impl>()) {}
TerrainScene::~TerrainScene() = default;

bool TerrainScene::load(I4DyuchiGXRenderer* renderer, I4DyuchiFileStorage* storage,
                        const char* hfl_name, std::string* error) {
    if (!renderer || !storage || !hfl_name) return false;
    for (auto* chunk : impl_->chunks) if (chunk) chunk->Release();
    impl_->chunks.clear(); impl_->textures.clear(); impl_->renderer = renderer;
    std::vector<std::uint8_t> hflBytes;
    if (!readStorageFile(storage, hfl_name, hflBytes) ||
        !mxh::compat::parse_hfl(hflBytes, impl_->terrain, error)) return false;

    ID3D11Device* device = nullptr;
    if (!renderer->GetD3DDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)) || !device)
        return false;
    impl_->textures.resize(impl_->terrain.textures.size());
    std::uint32_t placeholderTextures = 0;
    for (std::size_t i = 0; i < impl_->terrain.textures.size(); ++i) {
        std::vector<std::uint8_t> encoded;
        std::string textureName = impl_->terrain.textures[i].name;
        if (!readStorageFile(storage, textureName.c_str(), encoded)) {
            // The shipped Map.pak contains the engine-converted DDS form while
            // old HFL descriptors retain their authoring-time .tga names.
            const auto dot = textureName.find_last_of('.');
            if (dot == std::string::npos) {
                ++placeholderTextures;
                continue;
            }
            textureName.replace(dot, std::string::npos, ".dds");
            if (!readStorageFile(storage, textureName.c_str(), encoded)) {
                MLOG_WARN("[terrain] texture missing: %s", textureName.c_str());
                continue;
            }
        }
        const auto decoded = dx11::loadTextureFromMemory(encoded.data(), static_cast<std::uint32_t>(encoded.size()));
        if (decoded.pixels.empty()) {
            MLOG_WARN("[terrain] texture decode failed: %s", textureName.c_str());
            continue;
        }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = decoded.width; desc.Height = decoded.height;
        desc.MipLevels = 1; desc.ArraySize = 1; desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA initial{decoded.pixels.data(), decoded.width * 4u, 0};
        ComPtr<ID3D11Texture2D> texture;
        if (SUCCEEDED(device->CreateTexture2D(&desc, &initial, &texture)))
            device->CreateShaderResourceView(texture.Get(), nullptr, &impl_->textures[i]);
    }

    const auto& terrain = impl_->terrain;
    constexpr std::uint32_t kChunkTiles = 32;
    const std::uint32_t samplesPerTile = terrain.desc.faces_per_tile_axis;
    const auto sampleHeight = [&](std::uint32_t x, std::uint32_t z) {
        x = std::min(x, terrain.desc.height_count_x - 1);
        z = std::min(z, terrain.desc.height_count_z - 1);
        return terrain.heights[static_cast<std::size_t>(z) * terrain.desc.height_count_x + x];
    };
    const auto normalAt = [&](std::uint32_t x, std::uint32_t z) {
        const float left = sampleHeight(x ? x - 1 : x, z);
        const float right = sampleHeight(x + 1, z);
        const float down = sampleHeight(x, z ? z - 1 : z);
        const float up = sampleHeight(x, z + 1);
        VECTOR3 n{left - right, 2.0f * terrain.desc.face_size, down - up};
        const float length = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
        if (length > 0) { n.x /= length; n.y /= length; n.z /= length; }
        return n;
    };

    for (std::uint32_t chunkZ = 0; chunkZ < terrain.desc.tile_count_z; chunkZ += kChunkTiles) {
        for (std::uint32_t chunkX = 0; chunkX < terrain.desc.tile_count_x; chunkX += kChunkTiles) {
            const auto endX = std::min(chunkX + kChunkTiles, terrain.desc.tile_count_x);
            const auto endZ = std::min(chunkZ + kChunkTiles, terrain.desc.tile_count_z);
            std::vector<VECTOR3> positions, normals;
            std::vector<TVERTEX> uvs;
            std::vector<std::vector<std::uint16_t>> groups(terrain.textures.size());
            for (std::uint32_t tz = chunkZ; tz < endZ; ++tz) {
                for (std::uint32_t tx = chunkX; tx < endX; ++tx) {
                    const auto integrated = terrain.tiles[static_cast<std::size_t>(tz) * terrain.desc.tile_count_x + tx];
                    const std::uint32_t textureIndex = integrated & 0x3fffu;
                    if (textureIndex >= groups.size()) continue;
                    const std::uint32_t rotation = integrated >> 14;
                    for (std::uint32_t subZ = 0; subZ < samplesPerTile; ++subZ) {
                        for (std::uint32_t subX = 0; subX < samplesPerTile; ++subX) {
                            const std::uint32_t hx = tx * samplesPerTile + subX;
                            const std::uint32_t hz = tz * samplesPerTile + subZ;
                            const auto base = static_cast<std::uint16_t>(positions.size());
                            const std::array<std::array<std::uint32_t, 2>, 4> corners{{
                                {{hx, hz}}, {{hx + 1, hz}}, {{hx + 1, hz + 1}}, {{hx, hz + 1}} }};
                            for (const auto& corner : corners) {
                                positions.push_back({corner[0] * terrain.desc.face_size * kSceneScale - terrain.desc.width * kSceneScale * 0.5f,
                                                     sampleHeight(corner[0], corner[1]) * kSceneScale,
                                                     corner[1] * terrain.desc.face_size * kSceneScale - terrain.desc.height * kSceneScale * 0.5f});
                                normals.push_back(normalAt(corner[0], corner[1]));
                                const float u = static_cast<float>(corner[0] - tx * samplesPerTile) / samplesPerTile;
                                const float v = static_cast<float>(corner[1] - tz * samplesPerTile) / samplesPerTile;
                                uvs.push_back(rotatedUv(u, v, rotation));
                            }
                            auto& indices = groups[textureIndex];
                            // DX11 state uses FrontCounterClockwise=TRUE. In the
                            // engine's left-handed X/Z ground plane this order
                            // is front-facing when viewed from above.
                            indices.insert(indices.end(), {base, static_cast<std::uint16_t>(base+1), static_cast<std::uint16_t>(base+2),
                                                           base, static_cast<std::uint16_t>(base+2), static_cast<std::uint16_t>(base+3)});
                        }
                    }
                }
            }
            MESH_DESC meshDesc{};
            meshDesc.dwVertexNum = static_cast<std::uint32_t>(positions.size());
            meshDesc.pv3WorldList = positions.data(); meshDesc.pv3NormalLocal = normals.data();
            meshDesc.dwTexVertexNum = static_cast<std::uint32_t>(uvs.size());
            meshDesc.ptvTexCoordList = uvs.data();
            IDIMeshObject* mesh = renderer->CreateMeshObject(CMeshFlag{});
            if (!mesh || !mesh->StartInitialize(&meshDesc, nullptr, nullptr)) {
                if (mesh) mesh->Release(); continue;
            }
            std::vector<std::uint32_t> boundTextures;
            for (std::uint32_t textureIndex = 0; textureIndex < groups.size(); ++textureIndex) {
                auto& indices = groups[textureIndex];
                if (indices.empty()) continue;
                FACE_DESC face{}; face.pIndex = indices.data();
                face.dwFacesNum = static_cast<std::uint32_t>(indices.size() / 3u);
                face.dwMtlIndex = textureIndex;
                if (mesh->InsertFaceGroup(&face)) boundTextures.push_back(textureIndex);
            }
            mesh->EndInitialize();
            for (std::uint32_t group = 0; group < boundTextures.size(); ++group) {
                const auto textureIndex = boundTextures[group];
                mesh->SetFaceGroupDiffuseSRV(group, impl_->textures[textureIndex].Get());
            }
            impl_->chunks.push_back(mesh);
        }
    }
    MLOG_INFO("[terrain] original HFL loaded chunks=%u textures=%u/%u heights=%ux%u tiles=%ux%u",
              static_cast<unsigned>(impl_->chunks.size()), loadedTextureCount(),
              static_cast<unsigned>(impl_->textures.size()), terrain.desc.height_count_x,
              terrain.desc.height_count_z, terrain.desc.tile_count_x, terrain.desc.tile_count_z);
    if (placeholderTextures)
        MLOG_INFO("[terrain] palette placeholders=%u (legacy name '1')", placeholderTextures);
    return !impl_->chunks.empty();
}

void TerrainScene::configureCamera(float aspect) {
    if (!impl_->renderer || impl_->terrain.heights.empty()) return;
    CAMERA_DESC camera{};
    // Establishing overview camera until the server-driven player camera is
    // connected. Keep the eye outside the 51.2 km map so the full terrain is
    // in front of the near plane and screenshot acceptance is deterministic.
    if (impl_->follow_player) {
        const auto& d = impl_->terrain.desc;
        const auto sx = std::min(static_cast<std::uint32_t>(impl_->player_x / d.face_size), d.height_count_x - 1);
        const auto sz = std::min(static_cast<std::uint32_t>(impl_->player_z / d.face_size), d.height_count_z - 1);
        const float y = impl_->terrain.heights[static_cast<std::size_t>(sz) * d.height_count_x + sx] * kSceneScale;
        // Legacy GameIn initialises CMHCamera with angleX=30 degrees,
        // angleY=0, distance=1000 and CHARHEIGHT=140. Apply the same values
        // in the modern scene's 0.001 world scale.
        camera.v3To = {impl_->player_x * kSceneScale - d.width * kSceneScale * 0.5f,
                       y + 0.14f,
                       impl_->player_z * kSceneScale - d.height * kSceneScale * 0.5f};
        camera.v3From = {camera.v3To.x, camera.v3To.y + 0.5f,
                         camera.v3To.z - 0.8660254f};
        camera.v3Up = {0, 1, 0};
        camera.fFovY = 3.14159265f / 3.0f;
        camera.fFar = 80.0f;
    } else {
        camera.v3From = {0.0f, 70.0f, 0.0f};
        camera.v3To = {0.0f, 0.0f, 0.0f}; camera.v3Up = {0, 0, 1};
        camera.fFovY = 3.14159265f / 3.0f;
        camera.fFar = 150.0f;
    }
    camera.fAspect = aspect;
    camera.fNear = 0.1f;
    VECTOR3 forward{camera.v3To.x - camera.v3From.x,
                    camera.v3To.y - camera.v3From.y,
                    camera.v3To.z - camera.v3From.z};
    float length = std::sqrt(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
    forward = {forward.x/length, forward.y/length, forward.z/length};
    VECTOR3 right{camera.v3Up.y*forward.z - camera.v3Up.z*forward.y,
                  camera.v3Up.z*forward.x - camera.v3Up.x*forward.z,
                  camera.v3Up.x*forward.y - camera.v3Up.y*forward.x};
    length = std::sqrt(right.x*right.x + right.y*right.y + right.z*right.z);
    right = {right.x/length, right.y/length, right.z/length};
    const VECTOR3 up{forward.y*right.z - forward.z*right.y,
                     forward.z*right.x - forward.x*right.z,
                     forward.x*right.y - forward.y*right.x};
    const float viewValues[16] = {
        right.x,right.y,right.z,-(right.x*camera.v3From.x + right.y*camera.v3From.y + right.z*camera.v3From.z),
        up.x,up.y,up.z,-(up.x*camera.v3From.x + up.y*camera.v3From.y + up.z*camera.v3From.z),
        forward.x,forward.y,forward.z,-(forward.x*camera.v3From.x + forward.y*camera.v3From.y + forward.z*camera.v3From.z),
        0,0,0,1};
    MATRIX4 view = fromColumnMajor(viewValues);
    const float f = 1.0f / std::tan(camera.fFovY * 0.5f), zn = camera.fNear, zf = camera.fFar;
    const float projectionValues[16] = {
        f/aspect,0,0,0, 0,f,0,0, 0,0,zf/(zf-zn),-zn*zf/(zf-zn), 0,0,1,0};
    MATRIX4 projection = fromColumnMajor(projectionValues);
    MATRIX4 billboard = MatrixIdentity(); VIEW_VOLUME volume{};
    volume.From = camera.v3From; volume.fFar = camera.fFar;
    impl_->renderer->SetViewFrusturm(&volume, &camera, &view, &projection, &billboard);
}

void TerrainScene::followPlayer(float world_x, float world_z) {
    if (!impl_->follow_player)
        MLOG_INFO("[terrain] player camera active at (%.0f,%.0f)", world_x, world_z);
    impl_->follow_player = true;
    impl_->player_x = world_x;
    impl_->player_z = world_z;
}

float TerrainScene::heightAt(float world_x, float world_z) const noexcept {
    if (impl_->terrain.heights.empty()) return 0.0f;
    const auto& d = impl_->terrain.desc;
    const auto sx = std::min(static_cast<std::uint32_t>(std::max(0.0f, world_x) / d.face_size), d.height_count_x - 1);
    const auto sz = std::min(static_cast<std::uint32_t>(std::max(0.0f, world_z) / d.face_size), d.height_count_z - 1);
    return impl_->terrain.heights[static_cast<std::size_t>(sz) * d.height_count_x + sx];
}

void TerrainScene::render() {
    if (!impl_->renderer) return;
    for (auto* chunk : impl_->chunks)
        impl_->renderer->RenderMeshObject(chunk, 0, 0, 255, nullptr, 0, nullptr, 0, 0, 0, 0);
}

std::uint32_t TerrainScene::chunkCount() const noexcept { return static_cast<std::uint32_t>(impl_->chunks.size()); }
std::uint32_t TerrainScene::loadedTextureCount() const noexcept {
    return static_cast<std::uint32_t>(std::count_if(impl_->textures.begin(), impl_->textures.end(),
        [](const auto& texture) { return texture != nullptr; }));
}
} // namespace mxh::gx
