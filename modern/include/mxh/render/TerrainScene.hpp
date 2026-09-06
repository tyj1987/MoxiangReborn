#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "mxh/render/math.hpp"

namespace mxh::gx {
struct I4DyuchiGXRenderer;
struct I4DyuchiFileStorage;

// Modern scene bridge for the original 4Dyuchi HFL terrain. It preserves the
// original height samples, tile texture IDs and the two high orientation bits.
class TerrainScene {
public:
    TerrainScene();
    ~TerrainScene();
    TerrainScene(const TerrainScene&) = delete;
    TerrainScene& operator=(const TerrainScene&) = delete;

    [[nodiscard]] bool load(I4DyuchiGXRenderer* renderer,
                            I4DyuchiFileStorage* storage,
                            const char* hfl_name,
                            std::string* error = nullptr);
    void render();
    void configureCamera(float aspect);
    void followPlayer(float world_x, float world_z);
    // Camera yaw (radians, around the world Y axis) used while following
    // the player. 0 = the legacy default camera facing +Z.
    void setCameraYaw(float radians) noexcept;
    [[nodiscard]] float cameraYaw() const noexcept;
    void setCameraDistance(float distance) noexcept;
    [[nodiscard]] float cameraDistance() const noexcept;
    [[nodiscard]] float heightAt(float world_x, float world_z) const noexcept;
    [[nodiscard]] float worldWidth() const noexcept;
    [[nodiscard]] float worldHeight() const noexcept;
    // Returns the (X, Z) half-extent of the current map in scaled world units.
    // Used by downstream scenes (entity / static / effect) to re-centre their
    // own X/Z coords by the same offset the terrain mesh builder subtracts in
    // `load()`. The hard-coded `kEntityMapCenter = 25.6f` constant was correct
    // only for maps whose `desc.width * kSceneScale * 0.5 == 25.6`; this accessor
    // makes the centre map-specific so Map 10 / Map 12 / Map 21 / any future
    // map renders without a quadrant drift.
    [[nodiscard]] std::pair<float, float> mapCenter() const noexcept;
    [[nodiscard]] std::uint32_t chunkCount() const noexcept;
    [[nodiscard]] std::uint32_t loadedTextureCount() const noexcept;
    [[nodiscard]] std::uint32_t placeholderTextureCount() const noexcept;
    [[nodiscard]] std::uint32_t unresolvedTextureCount() const noexcept;
    // The most recent view*projection matrix computed by configureCamera,
    // or a default-constructed MATRIX4 if configureCamera has not been
    // called yet. Used by downstream scenes (e.g. EntityScene frustum
    // culling) to share the same camera state without recomputing it.
    [[nodiscard]] const MATRIX4& viewProj() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
