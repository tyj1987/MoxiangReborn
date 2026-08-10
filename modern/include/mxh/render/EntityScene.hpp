#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <span>
#include <string>

namespace mxh::gx {
struct I4DyuchiGXRenderer;
struct I4DyuchiFileStorage;

struct SceneEntity {
    std::uint32_t object_id = 0;
    std::uint16_t visual_kind = 0;
    float world_x = 0;
    float world_y = 0;
    float world_z = 0;
};

struct ScenePlayer {
    std::uint32_t object_id = 0;
    std::uint8_t gender = 0;
    std::uint8_t face_type = 0;
    std::uint8_t hair_type = 0;
    std::array<std::uint16_t, 10> weared_item_idx{};
    float world_x = 0;
    float world_y = 0;
    float world_z = 0;
};

// Original MonsterList.bin -> CHX -> MOD entity rendering bridge.
class EntityScene {
public:
    EntityScene();
    ~EntityScene();
    EntityScene(const EntityScene&) = delete;
    EntityScene& operator=(const EntityScene&) = delete;

    [[nodiscard]] bool load(I4DyuchiGXRenderer* renderer,
                            I4DyuchiFileStorage* storage,
                            std::string* error = nullptr);
    void synchronize(std::span<const SceneEntity> entities);
    void synchronizePlayer(const ScenePlayer& player);
    void render();
    [[nodiscard]] std::uint32_t loadedModelCount() const noexcept;
    [[nodiscard]] std::uint32_t instanceCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
