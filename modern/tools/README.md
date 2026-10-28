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

`originalExists` looks only under `--client-root`. Loose files are keyed by
their path relative to that root (`Resource/Map/x.hfl`, not
`client/Resource/Map/x.hfl`). Every regular `.pak` under that root is
mounted. Files under `server/`, `src/`, or `database/` do not count.
Matching follows the client: loose relative path first, then paks in mount
order (PlayDH 92-byte, then original 96-byte, then `pak_filename_less` on
the path relative to `--client-root`), full name then basename, ASCII
case-insensitive (`pack_file.cpp` `eq_ci`,
`filesystem_file_storage.cpp` `locate`). Extensions are not rewritten.
Render code maps `.tga` to `.dds` in more than one place
(`texture_loader.cpp` `compiledTextureName`, `terrain_scene.cpp`,
`static_scene.cpp`, `entity_scene.cpp`); those are not the storage lookup,
so a `.tga` reference hits only a `.tga` entry.
`originalHit` records the client-relative loose or pak path
(`loose:<path>` or `pak:<path>`, forward slashes, empty when missed).
`check` ignores `originalHit`.

After that index is built, `derive` calls `locate` for every
`KNOWN_NO_ORIGINAL` name and for the same stem with `.dds`. The `.dds`
name is the original with its extension replaced (`test4` gains `.dds`).
Those names are not in `references`, so the reference walk never checks
them, and storage lookup does not rewrite `.tga` to `.dds` by itself.
A non-empty hit exits non-zero, prints `name -> hit` (the relative
`loose:` or `pak:` label only), and does not write the manifest. When
every lookup misses, the manifest is unchanged. The guard uses the same
index as `originalExists`: `--client-root`, or `--source` when
`--client-root` is omitted. A root that is not a directory leaves the
index empty, and the guard does not fail.

Which maps are parsed is unchanged: casefolded basenames of loose files
under `--source`, plus basenames of entries in `--pak`. A `.map` that
exists only in some other pak is not parsed. `parsedMapCount` and
`decodedMaps` therefore stay on that older set.

`derive` still writes `derivation.localDecoder` (`version`,
`sourcePakSha256`, `decodedMaps` with name and sha256, `keySourceCounts`)
so `check --pak` can verify it. `trim` leaves that node alone and removes
decoder filesystem paths and any `.py` path. `pak.path` stays. Do not
commit the checked-in manifest from this change. The committed manifest
has 4026 `originalExists: false` rows out of 4674 references.

On ROG only `D:\build-mxr` is writable. `--local-decoder` is the existing
external decoder path used for check153. It is not in this repo.

```bash
python3 modern/tools/resource_reference_gate.py derive \
    --source D:/MX/client/Package/3DData \
    --client-root D:/MX/client \
    --pak D:/MX/client/Package/3DData/Map.pak \
    --local-decoder <existing external decoder path used for check153> \
    --output D:/build-mxr/tyj165/derive-a.json

python3 modern/tools/resource_reference_gate.py derive \
    --source D:/MX/client/Package/3DData \
    --client-root D:/MX/client \
    --pak D:/MX/client/Package/3DData/Map.pak \
    --local-decoder <existing external decoder path used for check153> \
    --output D:/build-mxr/tyj165/derive-b.json

python3 modern/tools/resource_reference_gate.py trim \
    --input D:/build-mxr/tyj165/derive-a.json \
    --output D:/build-mxr/tyj165/derive-a.trimmed.json

python3 modern/tools/resource_reference_gate.py trim \
    --input D:/build-mxr/tyj165/derive-b.json \
    --output D:/build-mxr/tyj165/derive-b.trimmed.json
```

`Get-FileHash -Algorithm SHA256` on the two trimmed files must be equal.
Acceptance is that hash match, plus this compare printing
`OTHER_FIELD_DIFFS 0` and `DECODED_MAPS_EQUAL`. The compare counts every
field except `originalExists` and `originalHit`. It also prints
`FALSE_BY_EXT`, `FALSE_BY_VIA`, and the nine `tile_17_mou_a_*.tga` values.
Names and counts only. The checked-in baseline has 4026 false rows out of
4674 references.

```bash
python3 modern/tools/resource_reference_gate.py compare-manifests \
    reference/manifests/original-resource-references.json \
    D:/build-mxr/tyj165/derive-a.trimmed.json
```

`check` runs in CI against that manifest. It exits 0 and prints warnings.
The single switch that later makes a non-clean conclusion fail the process
is `MXH_RESOURCE_REF_STRICT=1` (or `--strict`). A derivation that has not
been run is `no conclusion`, never a pass. Height fields 0, 4, 23, 24, 32,
33, 34, 94 and tile tables 0, 3, 4, 23, 24, 32, 33, 34, 94, 198 are not
referenced and are not reported as missing.
