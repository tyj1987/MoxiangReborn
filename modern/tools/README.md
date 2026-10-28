# Modern Tools

Buildable C++ tools are in subdirectories with their own `CMakeLists.txt`.
Loose Python tools (audit/repack) live here directly.

| Tool | Type | Purpose |
|------|------|---------|
| `MoxianAgentServer/` | C++ | modern AgentServer entry |
| `MoxianAutoPatcher/` | C++ | client auto-patcher |
| `MoxianClient/` | C++ | modern DX11 client |
| `MoxianClientE2E/` | C++ | headless client E2E harness |
| `MoxianDbTool/` | C++ | database schema + restore utility |
| `MoxianGMTool/` | C++ | GM tool |
| `MoxianLoginServer/` | C++ | modern LoginServer entry |
| `MoxianMapEditor/` | C++ | map editor |
| `MoxianMapServer/` | C++ | modern MapServer entry |
| `MoxianPacker/` | C++ | resource packer |
| `MoxianProtocolDoc/` | C++ | protocol documentation generator |
| `MoxianRenderDemo/` | C++ | headless render acceptance demo |
| `MoxianResourceExplorer/` | C++ | resource inspection CLI |
| `MoxianSideBySide/` | C++ | 5-stage behavior diff harness |
| `gen_protocol_doc.py` | Python | protocol doc fallback (used when C++ tool crashes) |
| `repack_titan_bin.py` | Python | repack MHFile payload utility |
| `audit_resource_coverage.py` | Python | PlayDH resource coverage audit |
| `audit_map_dependencies.py` | Python | HFL used-texture to Map.pak dependency closure audit |
| `resource_reference_gate.py` | Python | Original `.map`/`.stm`/`.hfl` reference check; warn-only until `MXH_RESOURCE_REF_STRICT=1` |
| `MoxianResourceExplorer npc-chx` | C++ command | NpcChxList kind-to-CHX catalog inspection |

## Resource Coverage Audit

`audit_resource_coverage.py` walks a PlayDH resource directory and runs the
modern `MoxianResourceExplorer` against every recognized resource file
(`.bin`, `.pak`, `.bmhm`, `.bsad`). It produces a coverage manifest
documenting which files the modern code can parse and which fail.

This is the M4 resource-coverage gate (see `ROADMAP.md` M4).

### Usage

```bash
python modern/tools/audit_resource_coverage.py \
    "C:\moxiang\墨香【源码配套资源】\PlayDH" \
    --build-dir C:\moxiang\modern\build \
    --output modern/build/coverage_manifest.txt
```

### Notes

- The script auto-creates an ASCII-named junction for the PlayDH root
  (under the ignored run-output directory `modern/out/runs/resource-coverage/`)
  because the explorer mangles non-ASCII path bytes in argv.
- Large files (>50 MB) get a 120s timeout; default 30s.
- Exit code is always 0; check the manifest for actual coverage.
- Per-file status uses the explorer subcommands: `info` (`.bin`),
  `list` (`.pak`), `map` (`.bmhm`), `bsad` (`.bsad`).

For a runtime map gate, audit the HFL texture closure without modifying the
pack:

```bash
python modern/tools/audit_map_dependencies.py 10 modern/data/PlayDH \
    --build-dir modern/build
```

The command exits non-zero and lists every missing converted DDS for a map
whose HFL references assets absent from `Map.pak`. That rewrite from `.tga`
to `.dds` is this audit's own behavior. It is not the original-reference gate.

## Original resource reference gate

`resource_reference_gate.py` extends the MxhResourceReference gate. It keeps
the extension stored in the original file (Map17's used rows stay `.tga`).

`derive` runs on ROG, the machine that has the original client files. It
reads filenames from original `.map`, `.stm`, and referenced `.hfl` files,
and writes only names to
`reference/manifests/original-resource-references.json`. CI does not have
those originals and must not invent `N.hfl` / `N.ttb` from map numbers.

`originalExists` looks at every loose file under `--source` and every
regular `.pak` under that root (nested archives included), plus `--pak`
when that file is not already under `--source`.
Matching follows the client: loose relative path first, then paks in mount
order (PlayDH 92-byte, then original 96-byte, then `pak_filename_less` on
the path relative to `--source`), full name then basename, ASCII
case-insensitive (`pack_file.cpp` `eq_ci`,
`filesystem_file_storage.cpp` `locate`). The client mounts only the top of
its own root; derive walks the whole `--source` tree because the ROG root
is `D:\MX`. Extensions are not rewritten. Render code maps `.tga` to `.dds`
in more than one place (`texture_loader.cpp` `compiledTextureName`,
`terrain_scene.cpp`, `static_scene.cpp`, `entity_scene.cpp`); those are not
the storage lookup, so a `.tga` reference hits only a `.tga` entry.
`originalHit` records the relative loose or pak path that satisfied the
hit (`loose:<path>` or `pak:<path>`, forward slashes, empty when missed).
`derivation.pak.path` is still the `--pak` string that was passed.
`check` ignores `originalHit`.

Re-derive on ROG. Write the manifest outside the repo. Do not commit it
from this tree; the checked-in manifest is replaced only after that run.

```bash
python3 modern/tools/resource_reference_gate.py derive \
    --source D:/MX \
    --pak D:/MX/client/Package/3DData/Map.pak \
    --output D:/MX-audit/original-resource-references.json
```

Obfuscated maps still need the local decoder, which stays outside the repo:
add `--local-decoder <plugin.py outside the repo>` (or set
`MXH_LOCAL_MAP_DECODER`). The command above is the scope and output path.

`check` runs in CI against that manifest. It exits 0 and prints warnings.
The single switch that later makes a non-clean conclusion fail the process
is `MXH_RESOURCE_REF_STRICT=1` (or `--strict`). A derivation that has not
been run is `no conclusion`, never a pass. Height fields 0, 4, 23, 24, 32,
33, 34, 94 and tile tables 0, 3, 4, 23, 24, 32, 33, 34, 94, 198 are not
referenced and are not reported as missing.
