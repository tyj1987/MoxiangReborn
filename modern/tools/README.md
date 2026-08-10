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
  (under `modern/scratch/<date>-resource-coverage/playdh_link_for_audit`)
  because the explorer mangles non-ASCII path bytes in argv.
- Large files (>50 MB) get a 120s timeout; default 30s.
- Exit code is always 0; check the manifest for actual coverage.
- Per-file status uses the explorer subcommands: `info` (`.bin`),
  `list` (`.pak`), `map` (`.bmhm`), `bsad` (`.bsad`).
