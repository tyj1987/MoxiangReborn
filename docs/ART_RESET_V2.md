# Moxiang Asset Reset V2

## 0. Purpose

This document defines the production baseline for the Unity client visual reset. The project keeps gameplay, protocol, map semantics and legacy data compatibility intact while replacing presentation assets with a modern, independently produced asset stack.

This is not a legacy-resource beautification pass. Legacy assets are reference/provenance inputs. New production assets are authored from source files under `artsource/`, exported through a controlled gate, imported into Unity under `Assets/Moxiang/ArtV2/`, validated, and only then used by runtime prefabs/scenes.

## 1. Current verified baseline

- Active repository: `moxiang/repo`.
- Active Unity client: `unity/MoxiangClient`.
- Unity baseline: **6000.6.0f1**.
- Render pipeline package: **URP 17.6.0**.
- Existing remaster modules already include imported map/model/motion tests, Wuguan training hall production tests, terrain tooling and an independent VFX assembly.
- Existing Wuguan training hall remains a reviewed legacy/remaster sample. It is not automatically migrated or rewritten by the V2 pipeline.
- The repository currently has substantial uncommitted work. Asset Reset V2 must add isolated files and must not reset, clean, rewrite or normalize unrelated work.

The previous architectural recommendation to move immediately to another Unity LTS is suspended. Engine migration is not part of Phase 0. The first priority is a stable art production contract on the already working 6000.6.0f1 baseline.

## 2. Source-of-truth boundaries

```
artsource/
  Blender/        editable .blend authoring source
  Textures/       editable source textures / masks
  Concept/        approved concept and paintover references
  Animation/      animation authoring/interchange source
  Audio/          source audio sessions and stems

unity/MoxiangClient/Assets/Moxiang/ArtV2/
  Characters/
  NPCs/
  Monsters/
  Weapons/
  Environment/
  Terrain/
  Vegetation/
  VFX/
  UI/
  Audio/
```

Rules:

1. `artsource/` is the editable art source of truth.
2. Unity does not receive working `.blend` files.
3. Unity receives exported runtime artifacts such as FBX, textures, audio, materials and prefabs.
4. `Assets/Moxiang/Art/` is legacy/remaster reference content and is not managed by the V2 importer.
5. Gameplay code, protocol structures and authoritative values are outside the scope of the asset pipeline.
6. Every V2 asset must be classifiable by its category directory.
7. Import rules are deterministic and enforced by `MoxiangAssetPostprocessor`.
8. Asset budgets are enforced by `MoxiangAssetValidator` and the build gate.

## 3. Visual target

Direction: realistic-proportion wuxia, restrained stylization, physically plausible materials, low-noise environment palette, high-readability combat silhouettes, and an authored Chinese martial-arts VFX language.

Avoid:
- mobile-style emission overload;
- uncontrolled saturated FX;
- one-off shaders per asset;
- one-character-one-skeleton duplication;
- one-building-one-unique-4K-texture production;
- runtime use of DCC source files.

Prefer:
- modular architecture kits;
- trim sheets, tiling materials, decals and vertex masks;
- shared humanoid skeletons where compatible;
- skeleton families for monsters;
- reusable weapon modules and sockets;
- shared master shaders;
- repeatable VFX primitives and material functions.

## 4. Phase 0 budgets

These are admission limits, not visual targets. Profiling may reduce them later.

| Category | Triangle limit per imported model | Material limit | Texture max |
|---|---:|---:|---:|
| Characters | 120,000 | 8 | 4096 |
| NPCs | 60,000 | 6 | 2048 |
| Monsters | 100,000 | 8 | 4096 |
| Weapons | 60,000 | 4 | 4096 |
| Environment | 100,000 | 8 | 4096 |
| Terrain | 250,000 | 8 | 4096 |
| Vegetation | 50,000 | 4 | 2048 |
| VFX | 50,000 | 8 | 2048 |
| UI | n/a | n/a | 4096 |
| Audio | n/a | n/a | n/a |

LOD guidance:
- LOD0: 100%
- LOD1: about 50%
- LOD2: about 20%
- LOD3: 5-10%

Final acceptance is profiler-based. The initial PC performance reference remains Windows x64 / 1080p / 60 FPS with an RTX 3060-class test machine.

## 5. Export contract

Blender authoring rules:

- Metric units.
- Unity-facing transform: Y up after export, positive forward convention documented by the exporter.
- Apply scale before export; runtime root must resolve to scale 1.
- No cameras or lights in production FBX exports unless the asset class explicitly requires them.
- Runtime materials are authored in Unity; embedded/imported FBX materials are disabled.
- UV0 is mandatory for textured geometry.
- UV1/lightmap UV is required for static baked environment assets where applicable.
- Tangents/normals must be deterministic.
- Collision proxies use explicit naming, not renderer guessing.
- Sockets/anchors use stable semantic names.
- Editable high-poly, bake cages and raw scans stay outside the Unity runtime tree.

Naming convention for new assets should start with `MX_` for primary runtime assets. Transitional imports are allowed but will be reported as warnings until the vertical slice freezes the final naming matrix.

## 6. Texture role convention

Preferred suffixes:

- `_BaseColor`, `_Albedo`, `_Diffuse`: sRGB color.
- `_Normal`, `_N`, `_NOR`: normal map, linear.
- `_AO`, `_Mask`, `_Metallic`, `_Roughness`, `_Smoothness`, `_ORM`: linear data.
- UI textures: Sprite import, no mipmaps unless explicitly overridden by a reviewed UI rule.

The V2 postprocessor sets deterministic defaults; deliberate exceptions must be encoded as policy, not hand-edited on individual importers.

## 7. Vertical slice

The existing Wuguan training hall is the first integration reference because it already has:

- PBR material production;
- high/low geometry evidence;
- baked preview/lightmap workflow;
- map 44 runtime integration;
- training-point semantics;
- production tests;
- server-identity hit presentation tests.

The V2 slice extends it rather than replacing it immediately.

Acceptance target for the first complete slice:

1. One reviewed environment kit.
2. One player-quality character.
3. Three NPC quality tiers.
4. Three normal monsters and one boss-quality monster.
5. At least three weapon families.
6. Five representative martial-arts skills with shared VFX primitives.
7. Production HUD + inventory + dialogue skin direction.
8. Day/night/indoor lighting profiles.
9. Automated asset validation report with zero errors.
10. Windows x64 development build remains green.

## 8. Production lines

### Environment

Legacy map -> semantic layout -> blockout -> modular kit -> materials -> lighting -> collision/nav -> LOD -> scene streaming validation.

### Characters and NPCs

Approved body/skeleton -> sculpt/high -> retopo -> UV -> bake -> texture -> hair/cards -> clothes/modules -> skin -> animation retarget -> LOD -> prefab.

### Monsters

Skeleton family -> silhouette -> high/low -> material variants -> animation set -> hit/death contracts -> VFX attachment points -> prefab.

### Weapons

Blade/body/guard/handle/sheath modules -> PBR -> sockets -> trails/hit points -> LOD -> prefab.

### VFX

Cast -> charge -> motion -> trail/projectile -> hit -> ground/persistent -> buff -> camera/audio hooks. Reuse material/shader primitives; do not build each skill as an isolated effect stack.

### UI

Information architecture remains gameplay-driven; visual layer is rebuilt through design tokens, typography, spacing, iconography, panel materials and reusable controls.

## 9. Automation gates

### Unity import gate

`MoxiangAssetPostprocessor` handles deterministic defaults for all files inside `Assets/Moxiang/ArtV2/`.

### Validation gate

`MoxiangAssetValidator` checks:

- managed category path;
- unsupported DCC runtime files;
- model triangle/material budgets;
- model import settings;
- texture size/mipmap/color-space/type policy;
- basic naming hygiene.

Report:
`modern/out/unity-remaster/asset-reset-v2/asset-validation.json`.

### Build gate

The V2 validator is attached to Unity pre-build validation. A managed asset with an error blocks the build. Warnings do not block Phase 0 builds.

### Inventory gate

`scripts/asset-reset-inventory.py` produces:
`modern/out/unity-remaster/asset-reset-v2/inventory.json`.

This report is intentionally file-level and deterministic; deeper model/render statistics remain Unity-side.

## 10. Blender automation backlog

Implement as the next tool slice under `artsource/Blender/tools/` or a dedicated repository tool package:

- MX Validate
- MX Naming
- MX Apply Transform
- MX UV Audit
- MX LOD Generate
- MX Collision Generate
- MX Bake Setup
- MX Export Unity
- MX Preview Turntable

The export command must generate an export receipt containing source file hash, export hash, Blender version, asset category, scale convention and exported object list.

## 11. Addressables decision

The current Unity package manifest does not include Addressables. Do not add it during Phase 0 merely to satisfy architecture documentation. First establish clean import/validation behavior and a green vertical slice. Add Addressables in the next controlled package-change phase with tests for groups, keys, update build and content dependency ownership.

## 12. CI target

Future GitLab asset job:

```
inventory
 -> source validation
 -> Blender headless export validation
 -> Unity batch import
 -> EditMode asset tests
 -> MoxiangAssetValidator
 -> development player build
 -> artifact/report publication
```

No CI stage may claim release readiness from a successful import alone.

## 13. Immediate implementation sequence

P0:
- keep Unity 6000.6.0f1/URP 17.6.0 pinned;
- create `artsource/` and `ArtV2/` boundaries;
- land importer, validator and tests;
- inventory current art state;
- preserve existing dirty worktree.

P1:
- convert Wuguan into the formal Art V2 vertical-slice acceptance target;
- introduce a shared material library and shader policy;
- add first Blender exporter/receipt;
- establish first character/weapon/VFX sample.

P2:
- add Addressables with controlled package change;
- introduce batch asset catalog and LegacyId -> MoxiangAssetId mapping;
- add CI jobs and artifact retention.

P3:
- scale production to maps, NPCs, monsters, weapons, skills and UI while maintaining zero-error asset validation.
