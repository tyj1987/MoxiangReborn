#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "mxh/render/frustum.hpp"
#include "mxh/render/math.hpp"

namespace mxh::gx {
struct I4DyuchiGXRenderer;
struct I4DyuchiFileStorage;

enum class SceneEntityType : std::uint8_t {
    Monster,
    Npc,
};

enum class SceneAction : std::uint8_t {
    Idle,
    Moving,
    Attack,
    Dead,
};

// Converts legacy 1-based motion enums to the zero-based CHX motion table.
// Invalid or unavailable action slots fall back to the standard motion.
[[nodiscard]] std::size_t chooseSceneMotionIndex(
    SceneEntityType type, bool player, SceneAction action,
    std::size_t motionCount) noexcept;

struct SceneEntity {
    std::uint32_t object_id = 0;
    std::uint16_t visual_kind = 0;
    float world_x = 0;
    float world_y = 0;
    float world_z = 0;
    SceneEntityType type = SceneEntityType::Monster;
    float facing_yaw = 0;
    std::uint32_t current_life = 0;
    std::uint32_t max_life = 0;
    SceneAction action = SceneAction::Idle;
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
    float facing_yaw = 0;
    std::uint32_t current_life = 0;
    std::uint32_t max_life = 0;
    SceneAction action = SceneAction::Idle;
};

struct WorldSnapshot {
    std::optional<ScenePlayer> local_player;
    std::vector<ScenePlayer> remote_players;
    std::vector<SceneEntity> entities;
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
    void synchronize(const WorldSnapshot& snapshot);
    void render();
    // Push a fresh camera frustum to the scene. Pass the view*projection
    // matrix from the same camera that drives the terrain (e.g. via
    // TerrainScene::viewProj()). The next render() call will skip any
    // entity whose world-space AABB is fully outside the frustum, but
    // the player model is always rendered (camera-centric). Pass an
    // empty optional to disable culling (the default).
    void setCameraFrustum(std::optional<Frustum> frustum) noexcept;
    [[nodiscard]] std::uint32_t loadedModelCount() const noexcept;
    [[nodiscard]] std::uint32_t instanceCount() const noexcept;
    [[nodiscard]] std::uint32_t playerInstanceCount() const noexcept;
    [[nodiscard]] std::uint32_t npcInstanceCount() const noexcept;
    [[nodiscard]] std::uint32_t culledInstanceCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
