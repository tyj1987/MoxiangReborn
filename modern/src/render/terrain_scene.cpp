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
    float camera_yaw = 0;
    float camera_distance = 6.0f;
    MATRIX4 view_proj{};  // last view*projection from configureCamera
    bool view_proj_valid = false;
    // A legacy HFL palette entry named "1" is intentionally untextured;
    // keep it separate from real missing-resource placeholders so release
    // gates do not report a false visual fallback.
    std::uint32_t palette_entries = 0;
    std::uint32_t unresolved_textures = 0;
    float terrain_min_height = 0.0f;
    float terrain_max_height = 0.0f;

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
    impl_->palette_entries = 0;
    impl_->unresolved_textures = 0;
    std::vector<std::uint8_t> hflBytes;
    if (!readStorageFile(storage, hfl_name, hflBytes) ||
        !mxh::compat::parse_hfl(hflBytes, impl_->terrain, error)) return false;
    if (!impl_->terrain.heights.empty()) {
        const auto [min_it, max_it] = std::minmax_element(
            impl_->terrain.heights.begin(), impl_->terrain.heights.end());
        impl_->terrain_min_height = *min_it * kSceneScale;
        impl_->terrain_max_height = *max_it * kSceneScale;
    } else {
        impl_->terrain_min_height = impl_->terrain_max_height = 0.0f;
    }

    ID3D11Device* device = nullptr;
    if (!renderer->GetD3DDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)) || !device)
        return false;
    impl_->textures.resize(impl_->terrain.textures.size());
    std::uint32_t paletteEntries = 0;
    std::vector<bool> usedTextures(impl_->terrain.textures.size(), false);
    for (const auto integrated : impl_->terrain.tiles) {
        const auto textureIndex = integrated & 0x3fffu;
        if (textureIndex < usedTextures.size()) usedTextures[textureIndex] = true;
    }
    for (std::size_t i = 0; i < impl_->terrain.textures.size(); ++i) {
        if (!usedTextures[i]) continue;
        std::vector<std::uint8_t> encoded;
        std::string textureName = impl_->terrain.textures[i].name;
        if (!readStorageFile(storage, textureName.c_str(), encoded)) {
            // The shipped Map.pak contains the engine-converted DDS form while
            // old HFL descriptors retain their authoring-time .tga names.
            const auto dot = textureName.find_last_of('.');
            if (dot == std::string::npos) {
                // Legacy HFL uses the literal name "1" for an intentionally
                // untextured palette entry. It is not a missing asset and
                // must not be replaced with a debug texture or counted as an
                // unresolved resource.
                if (textureName == "1") {
                    ++paletteEntries;
                    continue;
                }
                ++impl_->unresolved_textures;
                continue;
            }
            textureName.replace(dot, std::string::npos, ".dds");
            if (!readStorageFile(storage, textureName.c_str(), encoded)) {
                MLOG_WARN("[terrain] texture missing: %s", textureName.c_str());
                ++impl_->unresolved_textures;
                continue;
            }
        }
        const auto decoded = dx11::loadTextureFromMemory(encoded.data(), static_cast<std::uint32_t>(encoded.size()));
        if (decoded.pixels.empty()) {
            MLOG_WARN("[terrain] texture decode failed: %s", textureName.c_str());
            ++impl_->unresolved_textures;
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
        if (!impl_->textures[i]) ++impl_->unresolved_textures;
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
                            // DX11's FrontCounterClockwise state evaluates the
                            // projected winding, which is reversed by the
                            // left-handed X/Z ground basis.  Emit the upward
                            // normal first so steep Map21 faces are not
                            // discarded as backfaces.
                            indices.insert(indices.end(), {base, static_cast<std::uint16_t>(base+2), static_cast<std::uint16_t>(base+1),
                                                           base, static_cast<std::uint16_t>(base+3), static_cast<std::uint16_t>(base+2)});
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
    if (paletteEntries)
        MLOG_INFO("[terrain] intentional empty palette entries=%u (legacy name '1')", paletteEntries);
    impl_->palette_entries = paletteEntries;
    return !impl_->chunks.empty();
}

void TerrainScene::configureCamera(float aspect) {
    if (!impl_->renderer || impl_->terrain.heights.empty()) return;
    CAMERA_DESC camera{};
    constexpr float kSceneScale = 0.001f;
    const auto& d = impl_->terrain.desc;
    // Terrain vertices are authored around the origin (the mesh builder
    // subtracts half the map extent from every X/Z position).  Keep the
    // camera in that same coordinate system instead of assuming the legacy
    // 25.6-unit half-size used by one particular map.
    const float half_width = d.width * kSceneScale * 0.5f;
    const float half_height = d.height * kSceneScale * 0.5f;
    if (impl_->follow_player) {
        // Third-person player camera. Camera sits 6 units behind and 3
        // units above the player in scaled world coords, looking at the
        // player. The view clips everything beyond 200 units, which is
        // enough for the 512x512-unit world.
        const float yaw = impl_->camera_yaw;
        const float px = impl_->player_x * kSceneScale - half_width;
        const float pz = impl_->player_z * kSceneScale - half_height;
        const float back = impl_->camera_distance;
        const float up = impl_->camera_distance * 0.5f;
        const float ground_y = heightAt(impl_->player_x, impl_->player_z) * kSceneScale;
        const float look_y = ground_y + 1.0f;
        const float camera_x = impl_->player_x - back * std::sin(yaw) / kSceneScale;
        const float camera_z = impl_->player_z - back * std::cos(yaw) / kSceneScale;
        // The legacy camera follows the player along the ground, but must not
        // end up behind a nearby ridge.  Sample the actual HFL surface along
        // the player-to-camera segment and lift the camera above the highest
        // sampled point.  This keeps the character visible without disabling
        // depth testing or inventing a map-independent elevation.
        float path_max_ground = ground_y;
        for (int sample = 1; sample <= 8; ++sample) {
            const float t = static_cast<float>(sample) / 8.0f;
            const float sample_x = impl_->player_x + (camera_x - impl_->player_x) * t;
            const float sample_z = impl_->player_z + (camera_z - impl_->player_z) * t;
            path_max_ground = std::max(path_max_ground,
                                       heightAt(sample_x, sample_z) * kSceneScale);
        }
        const float camera_y = std::max(look_y + up, path_max_ground + 1.5f);
        camera.v3From = {px - back * std::sin(yaw), camera_y,
                          pz - back * std::cos(yaw)};
        camera.v3To   = {px, look_y, pz};
        // Follow-camera space uses world Y as vertical.  Using the overview
        // map-orientation up vector (Z) here rolls the view and can project
        // the player/nearby monsters outside the deterministic entity crop.
        camera.v3Up   = {0, 1, 0};
    } else {
        // Overview camera at the map centre, looking down. Deterministic
        // full-terrain screenshot view regardless of player position.
        // Some maps contain cliffs whose elevation is well above the old
        // fixed 35-unit camera height.  Aim at the terrain's vertical centre
        // and lift the camera above the highest sample so the overview never
        // starts inside the mesh (which otherwise produces a mostly-black
        // frame even though every texture loaded successfully).
        const float terrain_mid = (impl_->terrain_min_height +
                                   impl_->terrain_max_height) * 0.5f;
        const float extent = std::max(d.width, d.height) * kSceneScale;
        const float camera_height = std::max(35.0f,
            impl_->terrain_max_height + std::max(12.0f, extent * 0.75f));
        camera.v3From = {0.0f, camera_height, 0.0f};
        camera.v3To   = {0.0f,  terrain_mid, 0.0f};
        camera.v3Up   = {0, 0, 1};
    }
    camera.fFovY  = 3.14159265f / 3.0f;
    // Map 10 spans roughly 50 000 world units per axis; the legacy 200-unit
    // far-plane clipped every HFL corner and the surrounding sky/horizon
    // rendered into the BeginRender clear colour (0xff000000).  Extending
    // the far-plane to 800 units lets the entire 512-unit Map 10 terrain
    // remain in view and the sky MOD fill the previously-black band.
    camera.fFar   = 800.0f;
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
    // Cache view*projection for downstream consumers (e.g. EntityScene
    // frustum culling). G5 M-R5: row-vector-times-matrix convention;
    // Gribb-Hartmann column-based plane extraction handles the result.
    MATRIX4 vp{};
    MatrixMultiply2(&vp, &view, &projection);
    impl_->view_proj = vp;
    impl_->view_proj_valid = true;
}

const MATRIX4& TerrainScene::viewProj() const noexcept {
    return impl_->view_proj;
}

void TerrainScene::followPlayer(float world_x, float world_z) {
    if (!impl_->follow_player)
        MLOG_INFO("[terrain] player camera active at (%.0f,%.0f)", world_x, world_z);
    impl_->follow_player = true;
    impl_->player_x = world_x;
    impl_->player_z = world_z;
}

void TerrainScene::setCameraYaw(float radians) noexcept {
    impl_->camera_yaw = radians;
}

float TerrainScene::cameraYaw() const noexcept {
    return impl_->camera_yaw;
}

void TerrainScene::setCameraDistance(float distance) noexcept {
    impl_->camera_distance = std::clamp(distance, 3.0f, 12.0f);
}

float TerrainScene::cameraDistance() const noexcept {
    return impl_->camera_distance;
}

float TerrainScene::heightAt(float world_x, float world_z) const noexcept {
    if (impl_->terrain.heights.empty()) return 0.0f;
    const auto& d = impl_->terrain.desc;
    if (d.height_count_x == 0 || d.height_count_z == 0 ||
        d.face_size <= 0.0f) return 0.0f;

    // HFL stores samples on the face grid.  Sampling the lower-left sample
    // (the previous behavior) made the player and every projected entity
    // visibly pop at each face boundary.  Preserve the original world-space
    // origin while interpolating the four surrounding samples exactly like
    // the DX11 height-field path.
    const float gx = std::max(0.0f, world_x) / d.face_size;
    const float gz = std::max(0.0f, world_z) / d.face_size;
    const float max_x = static_cast<float>(d.height_count_x - 1);
    const float max_z = static_cast<float>(d.height_count_z - 1);
    const float clamped_x = std::min(gx, max_x);
    const float clamped_z = std::min(gz, max_z);
    const auto x0 = static_cast<std::uint32_t>(std::floor(clamped_x));
    const auto z0 = static_cast<std::uint32_t>(std::floor(clamped_z));
    const auto x1 = std::min(x0 + 1, d.height_count_x - 1);
    const auto z1 = std::min(z0 + 1, d.height_count_z - 1);
    const float fx = clamped_x - static_cast<float>(x0);
    const float fz = clamped_z - static_cast<float>(z0);
    const auto& heights = impl_->terrain.heights;
    const float h00 = heights[static_cast<std::size_t>(z0) * d.height_count_x + x0];
    const float h10 = heights[static_cast<std::size_t>(z0) * d.height_count_x + x1];
    const float h01 = heights[static_cast<std::size_t>(z1) * d.height_count_x + x0];
    const float h11 = heights[static_cast<std::size_t>(z1) * d.height_count_x + x1];
    return h00 * (1.0f - fx) * (1.0f - fz)
         + h10 * fx * (1.0f - fz)
         + h01 * (1.0f - fx) * fz
         + h11 * fx * fz;
}

float TerrainScene::worldWidth() const noexcept {
    return static_cast<float>(impl_->terrain.desc.width);
}

float TerrainScene::worldHeight() const noexcept {
    return static_cast<float>(impl_->terrain.desc.height);
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

std::uint32_t TerrainScene::placeholderTextureCount() const noexcept {
    // No synthetic terrain texture is ever generated. Report unresolved
    // material bindings through the public placeholder gate so a non-zero
    // value remains a hard visual failure.
    return impl_ ? impl_->unresolved_textures : 0;
}

std::uint32_t TerrainScene::unresolvedTextureCount() const noexcept {
    return impl_ ? impl_->unresolved_textures : 0;
}
} // namespace mxh::gx
