#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::gx {
struct I4DyuchiGXRenderer;
struct I4DyuchiFileStorage;

// DX11 bridge for the original 4Dyuchi .stm map geometry and materials.
class StaticScene {
public:
    StaticScene();
    ~StaticScene();
    StaticScene(const StaticScene&) = delete;
    StaticScene& operator=(const StaticScene&) = delete;

    [[nodiscard]] bool load(I4DyuchiGXRenderer* renderer,
                            I4DyuchiFileStorage* storage,
                            const char* stm_name,
                            std::string* error = nullptr);
    void render();
    [[nodiscard]] std::uint32_t meshCount() const noexcept;
    [[nodiscard]] std::uint32_t loadedTextureCount() const noexcept;
    [[nodiscard]] std::uint32_t unresolvedTextureCount() const noexcept;
    // Conservative XZ collision query over loaded STM mesh bounds.  It is
    // intentionally separate from rendering so movement can fail closed when
    // a map has no static geometry.
    [[nodiscard]] bool blocksPoint(float world_x, float world_z,
                                   float radius = 0.0f) const noexcept;
    // Push the (X, Z) half-extent in scaled world units that the terrain mesh
    // builder subtracts from each vertex, so the STM mesh re-centres by the
    // same offset. Must be called after every successful `TerrainScene::load()`
    // because the centre is map-specific. The legacy constant `25.6f` was
    // only correct for maps whose half-width * kSceneScale == 25.6 (Map 12
    // d.width = 51 200); Map 10 (d.width = 50 000) and Map 21 (d.width =
    // 50 000) drifted by 0.6 units until this setter existed.
    void setMapCenter(float world_x, float world_z) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
