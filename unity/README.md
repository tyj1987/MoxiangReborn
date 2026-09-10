# Moxiang Unity remaster

This directory hosts the Windows x64 Unity client migration. The approved
baseline is Unity 6000.6.0f1, URP, Direct3D 11 and the existing modern servers.
Original PlayDH bytes and legacy protocol headers remain read-only references.

The migration is in progress. A template project, passing parser tests or a
loaded native library do not establish playable or release acceptance.

Native C++ code owns protocol/session state. Unity owns presentation and input.
Derived visual resources use `unity-remaster-v1`, linked to the audited
`playdh-current` gameplay data. Placeholders never qualify for release.

From the repository root:

```powershell
scripts/build-unity-core.cmd
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stage-unity-core.ps1
scripts/build-unity-assets.cmd
```

Open `unity/MoxiangClient` with the pinned Editor. Native DLLs are generated
outputs and are not committed; close the target Editor before replacing one.
In the Editor, run `Moxiang.Editor.RemasterSetup.Configure` once, then
`Moxiang.Editor.RemasterSetup.BuildDevelopment`. The development Player is written
to `modern/out/unity-remaster/player`. Package dependencies and `.meta` GUIDs are
versioned. `ConnectionSceneBuilder.Create` is a one-time scene authoring helper;
it refuses to overwrite the already committed scene.

The bundled AMD/NVIDIA modules are required for the current Unity Player's
native library packaging, even on the development Intel GPU. They are built-in
Editor modules, not separately purchased services.

Real isolated integration checks (requires existing modern server binaries):

```powershell
python modern/tools/unity_three_server_smoke.py --player modern/out/unity-remaster/player/MoxiangClient.exe
python modern/tools/unity_three_server_smoke.py --player modern/out/unity-remaster/player/MoxiangClient.exe --editor-test
python modern/tools/unity_three_server_smoke.py --player modern/out/unity-remaster/player/MoxiangClient.exe --create-character
python modern/tools/unity_three_server_smoke.py --player modern/out/unity-remaster/player/MoxiangClient.exe --editor-test --create-character
```

These create fresh SQLite fixture databases and temporary accounts under
`modern/out/unity-remaster/three-server`, bind only loopback, and stop only the
processes they started. Passwords are passed through stdin/process environment,
never stored in project settings. By default the fixture seeds a character;
`--create-character` starts with an empty account and verifies the created row,
start area and initial equipment. The Player invokes the actual creation button
callback; the Editor additionally reconnects 100 times. Client-facing HSEL and the current plaintext internal
Agent→Map loopback link are recorded separately; neither certifies deployment
security. The Editor test covers 100 reconnects; the Player captures its own
rendered frame. Both remain automated evidence, not human gameplay acceptance.

See `docs/UNITY_REMASTER_STATUS.md` and `docs/UNITY_GAMEPLAY_COVERAGE.md` for gates.

Map10 terrain now uses `Map10.mxhterrain`: its ScriptedImporter generates the
64 meshes, materials and Prefab from the heightfield and explicit palette. The
13 `.mxhdds` files contain byte-identical DDS payloads selected from Map.pak;
the custom extension routes them to the legacy DDS importer. They retain their
original five mip levels, which Unity's default DDS importer rejects as incomplete.
This restores source materials, not the modern high-detail remaster sample.

Reproduce palette extraction into a new directory (never overwrite sources):

```powershell
python modern/tools/unity_terrain_palette.py unity/baselines/playdh-current.json.gz modern/data/PlayDH unity/baselines/map10-terrain-palette.json modern/out/unity-remaster/palette-reproduction
```

`Moxiang.Editor.TerrainPaletteSetup.Apply` explicitly binds the imported terrain
Prefab into the saved connection validation scene. The original geometry-only
mesh stays available as a disabled renderer for inspection. No visual mesh is
used as an authoritative movement/collision substitute.
