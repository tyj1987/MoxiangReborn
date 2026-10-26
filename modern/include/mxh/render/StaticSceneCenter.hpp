#pragma once

// Pure (DX-free) helpers for re-centring original 4Dyuchi STM geometry into
// the scene space used by TerrainScene.  The server / legacy [Server]Map
// collision code works in absolute world coordinates with no map-centre
// offset (CTileManager::GetTileIndex: nx = int(fx / fTILE_SIZE)), so the only
// requirement on the client is that STM statics and terrain subtract the SAME
// centre.  TerrainScene derives it from desc.width * kSceneScale * 0.5
// (Map 10 / Map 21: 25.0, Map 12: 25.6).

namespace mxh::gx::static_center {

inline constexpr float kSceneScale = 0.001f;
// Legacy fallback used only when no TerrainScene centre is available.
inline constexpr float kLegacyMapCenter = 25.6f;

struct Center {
    float x = kLegacyMapCenter;
    float z = kLegacyMapCenter;
};

[[nodiscard]] constexpr float worldToSceneX(float world_x, Center c) noexcept {
    return world_x * kSceneScale - c.x;
}
[[nodiscard]] constexpr float worldToSceneZ(float world_z, Center c) noexcept {
    return world_z * kSceneScale - c.z;
}
[[nodiscard]] constexpr bool sameCenter(Center a, Center b) noexcept {
    return a.x == b.x && a.z == b.z;
}

} // namespace mxh::gx::static_center
