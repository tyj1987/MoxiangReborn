# MX_ENV_GROUNDDROP_CLOTH_POUCH_001

Original neutral wuxia cloth pouch for a visible world ground drop. This is a presentation asset only. It does not encode item type, count, rarity, spawn, position, pickup success, or inventory state.

## Files and provenance

- Editable source: `MX_ENV_GROUNDDROP_CLOTH_POUCH_001.blend` (Blender 5.2.1 LTS; production mesh collection has the same asset ID).
- Unity interchange: `MX_ENV_GROUNDDROP_CLOTH_POUCH_001.fbx`.
- Portable backup: `MX_ENV_GROUNDDROP_CLOTH_POUCH_001.glb`.
- PBR maps: `Textures/*_basecolor.png`, `*_roughness.png`, `*_normal.png` (256 square, generated locally by `build_cloth_pouch.py`). No prior art, legacy texture, screenshot, or paid provider bytes were used.
- Transparent near and far views: `Previews/*.png` (512 square RGBA).
- `manifest.json` binds source, exports, textures, previews, and build script by SHA-256. `validation.json` records a Blender import round trip. Run `verify_cloth_pouch.py` with Blender to repeat it.

## Geometry contract

- 11 static mesh objects, 547 source vertices, 978 triangles. Each mesh has UV0 and applied object transforms.
- Bounds in Blender meters: X -0.15366..0.15055, Y -0.15403..0.14630, Z 0.00520..0.29400. Width 0.30421 m, depth 0.30033 m, height 0.28880 m.
- Pivot `(0,0,0)` lies at ground contact beneath the pouch. Blender is +Z up and -Y front. FBX export uses -Z forward, +Y up; Unity should see Y up with import scale 1. GLB carries glTF Y-up conversion.
- Form: irregular bulging woven cloth body, gathered neck, dark inner fold, cinched indigo cord, knot and two tails. It is intentionally generic, without item or rarity marking.

## Unity URP handoff

1. After review, copy the FBX and three PNG textures into a new folder under `unity/MoxiangClient/Assets/Moxiang/ArtV2/`; keep the `.blend` in `artsource` only. No Unity project file is changed by this delivery.
2. Make a Unity prefab or variant with model scale 1 and origin at the ground. Use an opaque URP/Lit material: basecolor map as sRGB, normal map imported as Normal Map, metallic 0. Unity uses smoothness, so invert the supplied linear roughness map and place that value in the channel expected by the chosen URP material workflow (usually metallic-map alpha). The FBX has separate external texture files; inspect the material assignment after import.
3. Put a simple `BoxCollider` on the **prefab root** that receives `GroundDropPickup`, approximately center `(0, 0.15, 0)` m and size `(0.35, 0.32, 0.35)` m. Fit it in scene and check camera ray hits. A child-only collider is unsafe for root `OnMouseDown`. Keep `Is Trigger` off unless the actual click implementation is changed. Disable automatic mesh colliders.
4. Assign that reviewed prefab to `ServerEntityRegistry.groundDropPrefab`. The registry alone instantiates it at the server event's mapped scene position, adds `ServerGroundDrop` and `GroundDropPickup`, and only removes it on the existing authoritative events. Do not add demo drops or fabricate pickup confirmations.

Contract inspected before production: `unity/MoxiangClient/Assets/Moxiang/Runtime/ServerEntityRegistry.cs` SHA-256 `db5695b77e8fe2b83e328de79e3c6cb23ebcfeb02561fae38f5730494f37f749`; `GroundDropPickup.cs` SHA-256 `7a18e34b67b87d2161a5ce2682d2e3904412ecc679974e3898f540c67369cb2e`. The first instantiates only when `groundDropPrefab` is assigned and uses `MapCoordinates(...).ToScene(...)` with identity rotation. The second handles click through `OnMouseDown` on the root script. Recheck code if those files change.

## Verification and limits

Blender 5.2.1 LTS reimported both exports. Their measured bounds, mesh counts, and triangle counts matched the source. Source PBR textures are packed in the `.blend`; export files, independent textures, and previews matched `manifest.json`. Near and far PNGs have transparent backgrounds.

Unity Editor import, live camera click, `DropSpawn` to `PickupAck`, inventory update, and destruction are outside this asset task and have not been claimed as passed. The code integration owner must test them with a real server drop.

