#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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

// A resource-backed object unit emitted by a BEFF script.  Unlike a
// PlaceholderVisual this always names the original CHX asset and is rendered
// through the same MOD/ANM mesh path as world entities.
struct EffectObject {
    std::uint32_t object_id = 0;
    std::string chx_name;
    float world_x = 0;
    float world_y = 0;
    float world_z = 0;
    float facing_yaw = 0;
    std::size_t motion_index = 0;
    bool has_motion_index = false;
};

enum class PlaceholderKind : std::uint8_t {
    Player,
    Npc,
    Monster,
};

// CPU-side stand-in drawn when CHX/MOD is missing. world_* are game
// units (same as SceneEntity). radius is scene-space half-extent
// (not world cm); default 0.5 is visible to the third-person camera.
struct PlaceholderVisual {
    std::uint32_t object_id = 0;
    float world_x = 0;
    float world_y = 0;
    float world_z = 0;
    float radius = 0.5f;
    PlaceholderKind kind = PlaceholderKind::Player;
};

inline constexpr float kEntitySceneScale = 0.001f;
inline constexpr float kEntityMapCenter = 25.6f;

// ARGB used by I4DyuchiGXRenderer::RenderBox for each placeholder kind.
[[nodiscard]] std::uint32_t placeholderArgb(PlaceholderKind kind) noexcept;

// Eight AABB corners in scene space, matching RenderBox's oct layout
// (bottom 0-3, top 4-7). Box sits on world_y and is 2*radius tall.
void fillPlaceholderOct(const PlaceholderVisual& visual, VECTOR3 oct[8]) noexcept;

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
    void synchronizeEffects(std::span<const EffectObject> effects);
    void clearEffects() noexcept;
    void render();
    // Debug-only opt-in for drawing RenderBox stand-ins when a model is
    // missing. Release/client runs keep this disabled so placeholders cannot
    // be mistaken for real game visuals.
    void setPlaceholderRenderingEnabled(bool enabled) noexcept;
    // Push a fresh camera frustum to the scene. Pass the view*projection
    // matrix from the same camera that drives the terrain (e.g. via
    // TerrainScene::viewProj()). The next render() call will skip any
    // entity whose world-space AABB is fully outside the frustum, but
    // the player model is always rendered (camera-centric). Pass an
    // empty optional to disable culling (the default).
    void setCameraFrustum(std::optional<Frustum> frustum) noexcept;
    // Push the (X, Z) half-extent in scaled world units that the terrain mesh
    // builder subtracts from each vertex, so entity / static / effect scenes
    // re-centre their own coordinates by the same offset. Must be called
    // after every successful `TerrainScene::load()` because the centre is
    // map-specific. The legacy constant `kEntityMapCenter = 25.6f` was only
    // correct for maps whose half-width * kSceneScale == 25.6 (Map 12
    // d.width = 51 200); Map 10 (d.width = 50 000) and Map 21 (d.width =
    // 50 000) drifted by 0.6 units until this setter existed.
    void setMapCenter(float world_x, float world_z) noexcept;
    [[nodiscard]] std::uint32_t loadedModelCount() const noexcept;
    [[nodiscard]] std::uint32_t instanceCount() const noexcept;
    [[nodiscard]] std::uint32_t playerInstanceCount() const noexcept;
    [[nodiscard]] std::uint32_t npcInstanceCount() const noexcept;
    [[nodiscard]] std::uint32_t culledInstanceCount() const noexcept;
    // Unique visual misses (missing catalog/CHX/device). Not silent:
    // each miss is counted and kept as a placeholder id for render.
    [[nodiscard]] std::uint32_t failedModelCount() const noexcept;
    [[nodiscard]] std::uint32_t unresolvedTextureCount() const noexcept;
    [[nodiscard]] std::uint32_t placeholderCount() const noexcept;
    [[nodiscard]] std::span<const PlaceholderVisual> placeholders() const noexcept;
    // Resolve the original ItemList display name for world-drop labels.
    // Empty means the profile has no matching item entry; callers must then
    // show an explicit numeric fallback rather than inventing a name.
    [[nodiscard]] std::string itemDisplayName(std::uint16_t item_id) const;
    // Resolve ItemList.ItemIdx to the corresponding image_item_path index.
    [[nodiscard]] std::optional<std::uint16_t> itemIconIndex(
        std::uint16_t item_id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mxh::gx
