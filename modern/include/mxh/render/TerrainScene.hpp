#pragma once

#include <cstdint>
#include <memory>
#include <string>

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
    [[nodiscard]] float heightAt(float world_x, float world_z) const noexcept;
    [[nodiscard]] std::uint32_t chunkCount() const noexcept;
    [[nodiscard]] std::uint32_t loadedTextureCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
