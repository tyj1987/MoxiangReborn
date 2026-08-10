#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::gx {
struct I4DyuchiGXRenderer;
struct I4DyuchiFileStorage;

// DX11 bridge for the original map sky .mod scene. Geometry and textures are
// read directly from PlayDH; no converted or procedural sky assets are used.
class SkyScene {
public:
    SkyScene();
    ~SkyScene();
    SkyScene(const SkyScene&) = delete;
    SkyScene& operator=(const SkyScene&) = delete;

    [[nodiscard]] bool load(I4DyuchiGXRenderer* renderer,
                            I4DyuchiFileStorage* storage,
                            const char* mod_name,
                            std::string* error = nullptr);
    void render();
    [[nodiscard]] std::uint32_t meshCount() const noexcept;
    [[nodiscard]] std::uint32_t loadedTextureCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
