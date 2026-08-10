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

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
