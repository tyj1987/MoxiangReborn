# Phase 10.1 — Model / Animation Format Documentation

> **Scope**: Reverse-engineered wire formats for the legacy Moxian 3D model files
> produced by the **MAXEXP** (3ds Max Biped exporter) and **anmexp** (animation
> exporter) plugins. Source of truth: `墨香【源码】/MAXEXP/*.cpp` and
> `墨香【源码】/anmexp/*.cpp`. No parser / writer in `modern/` today; this is
> the reference document for whoever writes one next.
>
> **Date**: 2026-07-15
> **Status**: Phase 10.1 — **DOCUMENTED, NOT IMPLEMENTED**

---

## 1. File family overview

| Ext | Producer | Consumer | Content |
|---|---|---|---|
| `.CHL` | `MAXEXP` | `4DyuchiGXRenderer` (scene) | Full scene: materials, lights, cameras, bones, meshes + hierarchy |
| `.CHR` | `MAXEXP` | `4DyuchiGXRenderer` (character) | Character model: meshes + bones + physique binding |
| `.CHX` | `4DyuchiFileStorage` (packing) | runtime | Repacked CHL/CHR (see `4DyuchiFilePack`) |
| `.MON` | `MAXEXP` | `4DyuchiGXRenderer` (monster) | Same as CHR but tagged as monster (no walking anim) |
| `.ANM` | `anmexp` | `4DyuchiGXRenderer` (motion) | Biped keyframe animation: bones + key tracks per frame |
| `.MOD` | manual / `MAXEXP` | 3D scene | Generic static mesh (no skeleton) |
| `.WP`/`.HT` | `MAXEXP` | weapon / hat slots | Same as MOD, attached to character bone |

**Important**: The exporters target **3ds Max with Biped + Physique plugins** (now
removed in Max 2018+). The plugin sources in this repo **do compile** under
VC6-era toolchains (see `MAXEXP/maxexp.vcproj`) but **will not build under MSVC
2022** without significant work. The wire format is what matters; we can write
new exporters (FBX, glTF) without ever compiling the old plugins.

---

## 2. Top-level container — TLV with type+size

Every `.CHL` / `.CHR` / `.MON` file is a sequence of type-length-value records:

```c
struct Record {
    DWORD type;   // OBJECT_TYPE_*  (see §3)
    DWORD size;   // bytes of payload that follow
    uint8_t payload[size];
};
```

Records are written back-to-back with no padding. The container is opened with
`fopen("wb")`. Reference: `MAXEXP/scene.cpp:1301-1355` (`CScene::WriteFile`).

The **first record in a `.CHL`** is preceded by a 32-byte fixed header:

```c
// MAXEXP/scene.h:11
struct FILE_SCENE_HEADER {            // 7 DWORDs = 28 bytes (Windows-aligned)
    DWORD dwVersion;          // must be 0x00000002
    DWORD dwObjectNum;        // total objects in scene (incl. materials)
    DWORD dwMaterialNum;
    DWORD dwMeshObjectNum;
    DWORD dwLightObjectNum;
    DWORD dwCameraObjectNum;
    DWORD dwBoneObjectNum;
};
```

`.CHR` / `.MON` skip the FILE_SCENE_HEADER and start directly with the first
record.

---

## 3. Object type enum

From `4DyuchiGRX_Common/typedef.h:258`:

```c
enum OBJECT_TYPE {
    OBJECT_TYPE_UNKNOWN         = 0x0f000000,
    OBJECT_TYPE_LIGHT           = 0x0f100000,
    OBJECT_TYPE_CAMERA          = 0x0f200000,
    OBJECT_TYPE_CAMERA_TARGET   = 0x0f300000,
    OBJECT_TYPE_MESH            = 0x0f400000,
    OBJECT_TYPE_BONE            = 0x0f500000,
    OBJECT_TYPE_ILLUSION_MESH   = 0x0f600000,
    OBJECT_TYPE_COLLISION_MESH  = 0x0f700000,
    OBJECT_TYPE_MATERIAL        = 0x00f00000,
    OBJECT_TYPE_MOTION          = 0x0000f000,  // .ANM only
};
```

The bytes stored in the file are little-endian DWORDs of these values. The high
nibble pattern (e.g. `0f...`) appears to be intentional — makes grepping for a
type easy and avoids collision with object indices.

---

## 4. Common header — every object record starts with this

From `MAXEXP/object.h:12`:

```c
struct FILE_BASE_OBJECT_HEADER {       // 4 + 4*16 + 4 + 4 + 128 = 200 bytes (approx)
    DWORD  dwIndex;                    // object index, unique within file
    NODE_TM TM;                        // local transform (see §4.1)
    DWORD  dwChildObjectNum;
    DWORD  dwParentObjecIndex;         // [sic] — original typo
    TCHAR  uszObjName[MAX_NAME_LEN];   // 128 TCHAR = 256 bytes (TCHAR = wchar_t)
};
```

`MAX_NAME_LEN` = 128 (`4DyuchiGRX_Common/typedef.h:13`). `TCHAR` = `wchar_t` on
this build (so all strings are UTF-16LE in the file).

### 4.1 `NODE_TM` — 4×4 transform

From `4DyuchiGRX_Common/typedef.h:581`:

```c
struct NODE_TM {
    VECTOR3  v3Pos;            // 12 B
    float    fReserved;        // 4 B  (struct alignment)
    MATRIX4  mat4;             // 64 B (4×4 floats)
    MATRIX4  mat4Inverse;      // 64 B
};
// Total: 144 B
```

`MATRIX4` is row-major 4×4 of float (`math.inl:159`). The exporter writes
`mat4 = object_local_to_parent` and `mat4Inverse = object_parent_to_local`.

---

## 5. Per-type payload formats

### 5.1 MATERIAL (`OBJECT_TYPE_MATERIAL`)

Written by `MAXEXP/material.cpp:32`. After the FILE_BASE_OBJECT_HEADER, the
payload is a `MATERIAL` struct (see `4DyuchiGRX_Common/typedef.h:1570`):

```c
struct FILE_MATERIAL_PAYLOAD {
    char   szMaterialName[MAX_NAME_LEN];    // 128 B
    DWORD  dwTexMapNum;
    // followed by dwTexMapNum × { TCHAR szTexName[MAX_NAME_LEN]; DWORD flag; }
    // followed by diffuse / bump / specular color DWORDs
    MATRIX4 matBillboard;                   // optional
};
```

Material indices are referenced by mesh face groups (see §5.3).

### 5.2 LIGHT (`OBJECT_TYPE_LIGHT`)

Written by `MAXEXP/light_obj.cpp:4`. After FILE_BASE_OBJECT_HEADER, light
properties:

```c
struct FILE_LIGHT_PAYLOAD {
    VECTOR3 v3Pos;
    VECTOR3 v3Dir;
    float   fIntensity;
    float   fRange;
    DWORD   dwLightType;   // 0=point, 1=directional, 2=spot
    DWORD   dwColor;       // 0x00RRGGBB
};
```

### 5.3 MESH (`OBJECT_TYPE_MESH`) — the meat of the format

Written by `MAXEXP/geom_obj.cpp:207`. After FILE_BASE_OBJECT_HEADER, the
payload is a **FILE_MESH_HEADER** followed by per-mesh data:

```c
// MAXEXP/geom_obj.h:11  (packed 4)
struct FILE_MESH_HEADER {
    DWORD  dwMaxVertexNum;        // capacity of vertex array
    DWORD  dwVertexNum;           // actual vertices written
    DWORD  dwOriginalVertexNum;   // vertices before optimisation
    DWORD  dwExtVertexNum;        // extended-vertex indices count
    DWORD  dwTexVertexNum;        // UV vertices
    DWORD  dwMtlIndex;            // primary material index (see MATERIAL §5.1)
    DWORD  dwFaceGroupNum;        // sub-meshes (one per material slot)
    CMeshFlag meshFlag;           // see mesh_flag.h
    DWORD  dwGridIndex;
    VECTOR3 v3Dir;                // face-culling hint
};
```

`CMeshFlag` is a small POD with `BILLBOARD` / `VERTEXLIGHT` / `LIGHTMAPLIGHT`
flags set by the export dialog (`max_common/UserDefine.h`).

**After FILE_MESH_HEADER the writer emits (in order):**

1. `m_dwVertexNum × VECTOR3` — position floats (12 B each, 3 floats)
2. `m_dwTexVertexNum × TVERTEX` — UVW floats (12 B each, 3 floats)
3. `m_dwExtVertexNum × DWORD` — extended-vertex indices (one DWORD per entry)
4. `m_dwFaceGroupNum × { FILE_FACE_GROUP_HEADER + face data }` — sub-meshes
5. **Physique binding** (if mesh has Biped skeleton) — see §5.4
6. `m_dwVertexNum × VECTOR3` — normals (skipped if `TRANSFORM_TYPE_ALIGN_VIEW`)

#### 5.3.1 Face group

From `MAXEXP/face_group.h:12`:

```c
struct FILE_FACE_GROUP_HEADER {     // 7 DWORDs = 28 B
    DWORD dwMtlIndex;               // material slot for this group
    DWORD dwIndex;                  // group index within mesh
    DWORD dwFacesNum;               // triangle count
    DWORD dwMaxFacesNum;            // capacity (== dwFacesNum post-write)
    DWORD dwVertexIndexNum;         // total unique vertex indices used
    DWORD dwLightUVNum0;            // lightmap UV count
    DWORD dwReserved;               // 0
};
```

After the header, the writer emits:

1. `dwFacesNum × 3 × WORD` — face indices (each triangle = 3 vertex indices)
2. `dwLightUVNum0 × TVERTEX` — lightmap UVs (one per face group, optional)

Faces are always triangles. `WORD` = 16-bit unsigned; mesh is therefore capped
at 65535 vertices per sub-mesh (this is why monsters with many parts split
across multiple `.CHL` sub-objects).

### 5.4 PHYSIQUE — bone-skinning binding

Written by `MAXEXP/Physique.cpp:104`. The format is a per-vertex list of
(bone-index, weight) pairs, with run-length encoding for shared bone chains:

```c
struct FILE_PHYSIQUE_HEADER {
    DWORD dwTotalVertexNum;        // == mesh's dwVertexNum
    DWORD dwTotalBoneNum;          // bones used by this mesh
    // followed by per-vertex:
    //   DWORD dwLinkCount;
    //   dwLinkCount × { WORD wBoneIndex; float fWeight; }
};
```

Weights per vertex sum to 1.0. Max 4 links per vertex is the practical limit
(the original Physique plugin was hard-coded to 4).

### 5.5 BONE (`OBJECT_TYPE_BONE`)

Written by `MAXEXP/bone_obj.cpp:7`. After FILE_BASE_OBJECT_HEADER:

```c
struct FILE_BONE_PAYLOAD {
    DWORD  dwParentBoneIndex;      // -1 if root
    // NODE_TM is in the FILE_BASE_OBJECT_HEADER — do not duplicate
    // Physique offsets for child vertices are computed at load time
};
```

### 5.6 CAMERA / CAMERA_TARGET (`OBJECT_TYPE_CAMERA`)

Written by `MAXEXP/camera_obj.cpp:8`. Stores `fov`, `near/far`, `targetIndex`.
Each camera record is followed by a separate `OBJECT_TYPE_CAMERA_TARGET`
record.

---

## 6. .ANM — animation format

Written by `anmexp/anmexp.cpp` (motion export). Animations are **separate
files** (one per motion, e.g. `walk.anm`, `attack1.anm`). Wire format:

```c
struct FILE_MOTION_HEADER {        // 28 B
    DWORD dwVersion;               // 0x00000001
    DWORD dwFrameNum;              // total keyframe count
    DWORD dwBoneNum;               // bones animated
    DWORD dwFrameRate;             // typically 30 or 60
    DWORD dwReserved[3];
};
```

Followed by `dwBoneNum × bone-name (TCHAR[MAX_NAME_LEN])`, then for each frame
`dwBoneNum × { MATRIX4 localTM }` (rows of bone-local-to-parent transforms).

**Bones match the skeleton in the .CHR/.CHL by name**, not index. The runtime
`4DyuchiGXRenderer` looks up the bone by name and applies the per-frame TM.
If a bone name in the animation is missing from the model, that bone is
silently skipped (logged to `g_Console`).

---

## 7. Packing — `.CHX`

`.CHX` files are produced by `4DyuchiFilePack` (see `墨香【源码】/4DyuchiFilePack/`).
The format is:

```
[12 bytes]  pack_header: { DWORD version=1; DWORD totalFiles; DWORD fileTableSize }
[ fileTableSize bytes ]  array of { DWORD nameHash; DWORD offset; DWORD size; }
[ concatenated file payloads... ]   // each one is a full .CHL/.CHR/.MON/.ANM
```

The `nameHash` is `crc32` of the lowercase filename. The runtime `I4DyuchiFile`
returns the requested entry by hash lookup; no path traversal, no directory
info. The runtime cache keeps a flat array of decoded `I3DModel*` objects
populated on first access.

This means **.pak-style packed files are 1:1 lossless** — repacking must use
the same `crc32` algorithm, or every `I4DyuchiFile::GetFile` call will fail.

---

## 8. Loading in the runtime

The runtime loader lives in `4DyuchiGXGFunc/SS3DGFunc.lib` (compiled binary,
source not in this repo). From the public headers (`4DyuchiGRX_Common/IGeometry.h`,
`IRenderer.h`):

```c
// High-level: 4DyuchiGXExecutive loads the file, then iterates objects
// to build an I3DModel tree.
IGeometry::CreateIVertexList(...)  // builds vertex buffer for renderer
IGeometry::InsertFaceGroup(FACE_DESC*)  // consumes a face group from §5.3.1
IGeometry::Render(...)  // bind vbuf, set TMs from animation frame, draw
```

The renderer is **DX8-era** (`IDirect3DDevice8`); the modernized
`modern/src/render/dx11/` has its own loaders that ignore this format. There
is **no bridge** between `SS3DGFunc.lib` and the DX11 renderer. Phase 6.x
(client) will need to either:

- (a) keep using the DX8 path for content (no port needed, but blocks DX11 goal)
- (b) write a new `I3DModel`-compatible parser in `modern/` that the DX11
  renderer consumes (no source for the binary format; we have to write it
  from scratch from this document)

---

## 9. Open items / next steps

- [ ] Validate this doc against an actual `.CHL` file using a Python hex
      dump script (the only test we have for the format). Block on
      `SWorking/Map*/Map*.CHL` extraction.
- [ ] Write a `phase10_chl_dump.py` tool that produces a YAML/JSON summary
      of a `.CHL` file (object list, mesh count, face count, material refs).
      This is the only way to **prove** this document matches reality.
- [ ] Decide on Phase 10.2 (FBX / glTF exporter) target. glTF is the
      obvious modern choice; we have FBX SDK licensing concerns (the original
      SDK is freely downloadable from Autodesk but binary-only).
- [ ] Phase 6.1–6.3 client refactor should treat the DX8 `SS3DGFunc` path as
      legacy and plan the migration to a modern-format-aware renderer.

---

## Appendix A — Byte-order quick reference

All multi-byte values are **little-endian** (Windows-native).

| Type | Size | Source |
|---|---|---|
| `BYTE` | 1 B | Win32 |
| `WORD` | 2 B | Win32 (unsigned) |
| `DWORD` | 4 B | Win32 (unsigned) |
| `float` | 4 B | IEEE-754 single |
| `VECTOR3` | 12 B | 3 × float (x, y, z), packed (no trailing padding) |
| `TVERTEX` | 12 B | 3 × float (u, v, w), packed |
| `MATRIX4` | 64 B | 4 × 4 float, row-major |
| `MAX_NAME_LEN` | 128 TCHARs | = 256 B (UTF-16LE) on this build |
| `TCHAR` | 2 B | = wchar_t, UTF-16LE |

## Appendix B — File extension cheat sheet

| Game data | Format | Notes |
|---|---|---|
| Player character body | `.CHL` (Biped mesh) + `.ANM` (motions) | Body + walk/attack/idle anims |
| Monster body | `.MON` (Biped mesh, no walk anim) + `.ANM` (only idle + attack) | Limited motion set |
| NPC | `.CHL` (no Biped — static) + scripted | No animation needed |
| Weapon | `.WP` (mesh) | Attached to right-hand bone |
| Hat | `.HT` (mesh) | Attached to head bone |
| Map static object | `.CHL` (no Biped) + static lightmap UVs | Buildings, trees, rocks |
| Effect (skill) | `.CHL` (particle) | Emitted by skill system at runtime |
