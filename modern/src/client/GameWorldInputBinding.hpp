#pragma once

#include "CInGameState.hpp"
#include "WorldScreenProjection.hpp"

namespace mxh::client {

// World loading and GameIn creation can complete in either order. Rebind all
// world queries whenever either owner changes; a fresh GameIn must never keep
// the empty callbacks it had before the terrain was ready.
template <class Terrain, class StaticScene>
void bind_game_world_input(CInGameState& game, Terrain* terrain,
                           StaticScene* static_scene) {
    if (terrain) {
        game.set_world_bounds(terrain->worldWidth(), terrain->worldHeight());
        const auto height_at = [terrain](float x, float z) {
            return terrain->heightAt(x, z);
        };
        game.set_world_screen_queries(
            [terrain, height_at](float sx, float sy) {
                return pick_ground_point(
                    terrain->viewProj(), terrain->mapCenter(), height_at,
                    sx, sy, terrain->worldWidth(), terrain->worldHeight(),
                    terrain->heightSampleSpacing());
            },
            [terrain, height_at](float x, float z, float offset) {
                return project_world_point(
                    terrain->viewProj(), terrain->mapCenter(), height_at,
                    x, z, offset);
            });
    } else {
        game.set_world_screen_queries({}, {});
    }
    if (static_scene) {
        game.set_collision_query([static_scene](float x, float z, float radius) {
            return static_scene->blocksPoint(x, z, radius);
        });
    } else {
        game.set_collision_query({});
    }
}

} // namespace mxh::client
