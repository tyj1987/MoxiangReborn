# Unity remaster — implementation status

## Approved scope

Branch: `codex/unity-client-remaster`, source baseline
`89a11b3653549bab7471f53f8f0aafb72e98ce2a`.
Windows x64 Unity 6000.6.0f1, URP 17.6.0, D3D11. Modern high-detail wuxia
presentation, reuse and selective remake of existing assets. Preserve original
gameplay, values, map topology, input semantics and protocol/security signatures.
The modern Login/Agent/Map servers remain authoritative. No Unity database access.

Original resources remain immutable. `unity-remaster-v1` is an explicitly
authorized derived visual profile linked to `playdh-current` gameplay. This
exception permits visual remakes, not changed collision, skill timing or balance.

## Gates (2026-09-11)

| Gate | Status | Remaining acceptance |
|---|---|---|
| A: baseline | Partial | full gameplay/UI dependency inventory and source selections |
| B: Unity/native core | Partial | domain reload, fault coverage and complete command/state extraction; initial real flow verified |
| C: visual sample | Not accepted | actual Map10/character/material/animation sample and human review |
| D: Map10 playable | Not accepted | two-player input, gameplay, map change, SQLite/MSSQL |
| E: full game | Not accepted | all original target systems, maps and assets |
| F: release candidate | Not accepted | clean machine, signed updates/rollback, performance, 24-hour soak |

The inherited gameplay register is `docs/UNITY_GAMEPLAY_COVERAGE.md`. Trade,
MurimNet client integration and paid-shop fulfillment are explicitly not complete.

Existing `PLAYABLE_STATUS.md` and `VERIFICATION_MATRIX.md` remain evidence for the
DX11 client; their passes do not automatically transfer to Unity. Original E5
continues to mean legacy comparison plus human review. Remaster art acceptance
is separate and must not be labelled pixel-identical original rendering.

## Resource baseline

Map10 source-material restoration (2026-09-11): the 13 used HFL palette slots
resolve explicitly from authoring `.tga` names to unique same-name `.dds` entries
in Map.pak, consistent with the existing terrain renderer's mapping. The reviewed
selection is `unity/baselines/map10-terrain-palette.json`. All source hashes match.
Each source has a partial five-level DDS mip chain; Unity's default importer
rejects it. `MxhDdsImporter` keeps the original DXT blocks and mip count without
recompression. `MxhTerrainImporter` generates 64 meshes/materials/Prefab subassets
from `.mxhterrain`, preserving tile rotation, global normals and existing winding.
These generated meshes are not duplicated in version control. No collision or
high-detail remaster quality acceptance is inferred from source-material display.

Verification: Unity 13 passed / 1 real-server test ignored without its fixture;
full C++ 12,454 registered / zero failures / 6 skips, 92.10 seconds; source-byte
check 5,372 passed. Independent Player run `14edba8c09164acb8a110e865df70644`
reached real three-server GameIn and visibly rendered the textured map. Evidence:
`modern/out/unity-remaster/terrain-importer-tests.xml`, `terrain-ctest.log`, and
`three-server/14edba8c09164acb8a110e865df70644/player.png`.

The new audit reads all seven PAK containers and loose files without extraction
into the source tree. It records 19,682 logical assets, 19,741 physical sources,
14,376 PAK entries and 5,365 loose files. All PAK entry counts match their headers.
53 assets have differing source bytes; 6 have identical duplicates. All 59 need
an explicit source selection. There is no loose-file-first policy.

All 81 loose HFLs match the generator's 16x16 grid, map-derived extents and exact
generated height sequence. 55 packed HFLs remain unverified provenance. This
supersedes the historical claim that 10/21/101 loose HFLs are original. A further
18 map descriptors have no matching TTB in the audit's logical inventory.
These findings are provenance/coverage evidence, not proof of render semantics.

Inventory digest:
`c3f54ffad61641e7de0475178914ab5fabd76975487af6fb8d14851b79544e3a`.
The full reproducible baseline is `unity/baselines/playdh-current.json.gz`.
The manifest checksum protects metadata against accidental changes; it is not a
digital signature. Production signing remains a separate release gate.

Commands (repository root):

```powershell
python modern/tools/unity_resource_audit.py audit modern/data/PlayDH --output modern/out/unity-remaster/baseline/resource-manifest.json --profile-id unity-remaster-v1 --source-commit 89a11b3653549bab7471f53f8f0aafb72e98ce2a
python modern/tools/unity_resource_audit.py verify-source unity/baselines/playdh-current.json.gz modern/data/PlayDH
python modern/tools/unity_resource_audit.py validate unity/baselines/playdh-current.json.gz --mode release
```

The source-byte verification checks 5,372 files (all loose files and complete PAK
containers). The initial release provenance check intentionally fails with 107
blockers. Passing that check alone would not establish full dependency closure,
visual acceptance or game release readiness.

## Verification and environment

- Bounded timed-movement activation (2026-09-11): native ABI bumped to
  `0x00010003` with new `mxh_unity_submit_extended_command` entry and a 140-byte
  `mxh_unity_extended_command` carrier. `MoxianMapServer
  --experimental-timed-movement` flips the gate; without the flag, the legacy
  4-byte dispatch is preserved verbatim. 14 new server-side tests + 5 new Unity
  EditMode tests added; no existing test removed. See
  `docs/UNITY_TIMED_MOVEMENT.md` ("Step 1 activated") and
  `docs/UNITY_MOVEMENT_STATUS.md` ("Bounded timed-movement activation").
  OFF reference run `759f279828f049ceb0d85ec5b96bb9ab` remains authoritative
  for legacy 4-byte; real Player run under
  `MXH_TIMED_MOVEMENT=1 --experimental-timed-movement` is the next gate.
- Final x86 build passed; full CTest registered 12,453 tests, zero failures,
  6 explicit skips, 74.94 seconds. The skips are Map10 release/human smoke,
  deployed resource digest, and four MSSQL tests. Evidence:
  `modern/out/unity-remaster/final-ctest.log`. They are not acceptance passes.
- Before code extraction: `cmake --build modern/build --config Debug` passed.
- Baseline CTest: 12,446 registered tests, zero failures, 6 explicit environment
  skips, 139.17 seconds. Log: `modern/out/unity-baseline-ctest.log`.
- Unity project created from installed `com.unity.template.urp-blank`; package
  versions are recorded by Unity in `Packages/packages-lock.json`.
- Unity CLI 1.0.0-beta.8 and local Personal license verified active. Sandbox
  access to Hub state/IPC required escalation; no account or license was changed.
- Pipeline 0.6.0-exp.1 is an Editor automation dependency, not a gameplay service.
- Development machine: i9-14900K, Intel Arc B580, driver 32.0.101.8992. WMI VRAM
  is a 32-bit field and is not accepted as the card's memory capacity.
- The 16 GB RAM / 4 GB VRAM minimum-target machine is not verified by tests on
  this development machine. Performance, full-map and soak gates remain open.

### Initial implementation evidence

- Native x64 core extracted with fixed-size C ABI and shared ClientWire helpers;
  legacy x86 client builds after extraction. Initial targeted legacy regression:
  93 tests, no failures, one pre-existing Map10 release smoke skip.
- Unity EditMode final run `694e8e79d40d4a95aa60f29cca68757c`: 10 tests passed,
  including native DLL create/snapshot/destroy 100 times and 100 real three-server
  login/list/select/GameIn/disconnect cycles. All servers stayed alive. This is
  connection lifecycle evidence, not a measured memory-growth or persistence test.
- Native exporter contract passed: deterministic bytes, source/sidecar SHA-256,
  no overwriting existing output, truncated/non-finite input rejection.
- Map10 inspection derives from explicitly selected Map.pak entry 265, source
  SHA-256 `a774fc78a439afe0b435b88a562ede471af013d731b7f10c1c1f2a402e2aa454`.
  Existing C++ HFL parser produced a 513×513 height grid, 256×256 tiles and 37
  palette names. Unity mesh has 263,169 vertices. Source provenance, alpha,
  collision, materials and visual quality are not accepted.
- First standalone Player run `b043197cbbd34161a27309e32d78ca21` reached frame
  120, displayed the real terrain and connection panel, passed 100 native
  lifecycle cycles and exited 0. It did not connect to servers or prove gameplay.
  Evidence: `modern/out/unity-remaster/player-smoke/report.json` and `player.png`.
- Actual Player reported Arc B580 VRAM 12,118 MB; this is development hardware,
  not the proposed 4 GB target. Initial Player exposed missing AMD/NVIDIA module
  native libraries; built-in module dependencies were corrected and rerun.
- Built-in `com.unity.modules.amd/nvidia@1.0.0` resolved through Package Manager;
  the subsequent Player no longer reports the missing native module errors.
  Unused URP depth-of-field/Panini shader stripping warnings remain to inspect.
- Real server Player run `bdd9a56c868e48e0a51381bab7d23a62` passed Login →
  character list → select → GameIn, player 111 / Map10. Three existing modern
  executables used a newly migrated isolated SQLite fixture. Client-facing
  links used HSEL; the existing Agent→Map legacy link was loopback-only plaintext.
  No database credentials or passwords are embedded in the Unity project.
  Evidence: `modern/out/unity-remaster/three-server/<run-id>/`.
  This does not establish human playability, new-character creation, combat,
  persistence equivalence or MSSQL acceptance.
- Final Player run `56423dc796c6440aaf4087c5589f1298` passed the same real
  three-server flow using the final 132/992-byte ABI and native library SHA-256
  `97E8D2BBB46669D7381633BACB7FF2B295282D9C5A2F6CB687C088E2B574DA03`.
  The actual rendered frame and report are `player.png` and `report.json` in
  that run directory. This is still the geometry inspection scene.
- Initial headless Pipeline captures were blank despite valid geometry. The
  standalone Player capture is the first accepted visibility evidence; it remains
  an inspection scene, not the high-detail art sample.

### Review and release boundaries

The independent review identified terminal event loss on queue overflow, stale
event capacity across reconnect, permissive character-list parsing, GameIn
identity/map association, destroy races, cleanup and Agent timeout propagation.
These core fixes now pass 23 standalone x64 tests and the final Editor tests.
The GameIn decoder preserves 32-bit life and 64-bit experience; corrected legacy
experience/money offsets are +22/+36 within HERO_TOTALINFO. C ABI game/snapshot
sizes are 132/992 bytes, with matching managed assertions and high-value fixtures.

Real-server validation exposed two inherited defects: MapServer populated the
account field with character ID, and TcpServer could use a handler-owned cipher
after disconnect released it. Map now propagates the authenticated Agent account
field. TcpServer synchronizes cipher use/invalidation before disconnect callbacks;
callbacks run outside the cipher lock. Tests cover callback reentry and a blocked
encryption call concurrent with cipher destruction. The failed pre-fix Editor
run `600839cf9778444abbcfd02dfb23eae3` recorded Agent exit `0xC0000005`; the fixed
100-cycle run above passed. No original protocol header was changed.
HSEL compatibility does not establish authenticated transport or MITM resistance.
The existing server progression state (`server/player_state.hpp`) still stores
level/total experience in 32-bit fields, and MapHandler's database loader narrows
experience. The new bridge preserves the full 64-bit wire value; this does not
resolve server-wide high-experience persistence/progression parity. That inherited
gap requires a separate end-to-end gameplay change before full-game acceptance.
The inherited security stubs and credential transport policy remain release
blockers. `RemasterBuildGuard` refuses non-development builds while these gates,
resource provenance and full gameplay acceptance are incomplete.

### Character creation validation (2026-09-11)

The native command ABI is now `0x00010001`, with a 128-byte command. Event and
snapshot sizes remain 320/992 bytes. Creation validates UTF-8, legacy encoded
name length without truncation/best-fit conversion, semantic appearance options,
slot capacity and session/map generations. It confirms creation only after the
server returns a consistent refreshed list; rejection preserves the old list.
Original protocol headers and gameplay data were not modified.

The independent native implementation passed 29 x64 tests. The main x86 build
and full CTest completed with zero failures (12,454 registered, six skipped,
89.05 seconds). Skips include MSSQL integration and release/resource checks;
they are not acceptance evidence. Log: `modern/out/unity-remaster/create-ctest.log`.

Real isolated SQLite three-server runs started with no character:
- Editor `414e13942d634476b3e50e076fa549ea`: creation, GameIn and 100 reconnects.
- Player `ce8c15e258af4b3cb02d9ecb876a093f`: actual creation button callback,
  refreshed slot and GameIn; rendered frame inspected in `player.png`.

Both persisted character 100000 / UnityNew / start area 17 and equipment
slots 1/2/3 with item IDs 11000/23000/27000. Evidence is under
`modern/out/unity-remaster/three-server/<run-id>/`. Staged native DLL SHA-256:
`84D1D3343E7BF2BE66E8E84270BD906963C1F5D9B43042F19E35EB0AA7053C9A`.
These are automated development checks, not human acceptance. Appearance
preview, CJK input/font coverage, movement, combat and full Map10 gameplay remain
unaccepted. The rendered terrain is source material inspection, not the modern
high-detail quality sample. MSSQL/PVE and full-game delivery remain open.

### Movement bridge and network recovery (2026-09-11)

API 0x00010002 now bridges Move/Stop, predicted local position, own server
Correction and remote movement events, using shared DX11/native wire helpers.
Real two-session testing uncovered and fixed duplicate Map-to-Agent fan-out,
misrouted owner corrections, shared HSEL directional key schedules and encrypted
send ordering. These fixes preserve public crypto signatures and protocol headers.
Client/core/server binaries must be updated or rolled back together.

The x64 core/crypto suite passed 40 tests. Full x86 CTest had 12,456 registered,
zero failed and six skipped. Editor run `e76269ba960c4c1682af3ef2ed18aba4` and
standalone Player run `759f279828f049ceb0d85ec5b96bb9ab` passed actual three-server
move/stop/correction with two native sessions. Editor also completed 100 reconnects.
This does not prove two-human gameplay, full movement rules or persistence.

Read-only source review also located real Map10 fixed-attribute collision data
at `Resource/Map/10.ttb` (1024x1024 WORD attributes, 50-unit cells); six source
copies match. It remains to integrate strict loading, original time/state rules
and segment collision. See `docs/UNITY_MOVEMENT_STATUS.md` for exact contracts,
source evidence, known gaps and run paths. Full Map10/game acceptance remains open.

## Delivery rules

Latest collision increment: MapServer now requires strict original fixed-tile
data and validates movement against its segment scan (plus OneTarget endpoint).
Malformed, unknown-owner and client authority-only movement packets are rejected.
Source Map10 SHA-256 is unchanged. Player SQLite/HSEL movement regression run
`1b0dafffeb7647678a19c69c675348b1` passed. See `UNITY_MOVEMENT_STATUS.md` for
tests and remaining timed movement, state, persistence and wall-probe gaps.
This supersedes the earlier statement that all segment collision is pending.

Further verification: 124,852 independent reference-path comparisons pass.
Audited Map10 blocked-cell movement now passes in real Editor run
`f5d80dbe7bbb4b61bb651797623e6f7e` and Player run
`d7193e33e2ce402daee4fda301e714fc`. The target is below the jump threshold,
the owner is corrected, and the observer receives no rejected movement.
Versioned Player reports prevent older movement-only binaries from passing.

Keep the original x86 build and DX11 executable for differential testing. Native
Unity builds use their own x64 output directory. Do not automatically fix duplicate
UI headers by copying one side: newer declarations exist on both sides.
Record each gate with source revision, resource digest, exact command and run ID.
Default deployment order remains local acceptance, then PVE, with SQLite and MSSQL
both verified. No release tag, production rollout or completion claim is implied
by this status document.

## 2026-09-12 Unity compile and EditMode recovery

The Map10 scene-builder collider typing error was fixed in `25c92cb1`. After
terminating the stale pre-fix Editor process, Unity 6000.6.0f1 EditMode tests
completed successfully: 27 total, 25 passed, 0 failed, 2 skipped. Report:
`unity_editmode_results_latest.xml`. The skipped cases remain environment or
runtime-dependent and do not constitute Player combat acceptance.

## 2026-09-12 Map10 combat input increment

The Unity remaster now exposes a server-authoritative skill command through the
fixed-width native ABI (`MXH_UNITY_COMMAND_SKILL=8`). The command validates
skill and target IDs, finite target coordinates, session generation and map
generation before constructing the existing `Skill StartSyn` payload. Commits
`f502941e`, `4cd1986b`, `0ac91c06`, `a309cda6`, `a4e6b898`, `6ac77fbe` and
`05200461` add the Unity bridge, stable target identity, scene wiring and target
invalidation rules without changing locked protocol headers or server formulas.

The x64 Unity core regression suite remains 69/69 passed. Unity Editor/Player
combat evidence is still pending because the local Unity licensing IPC client
is unavailable; entity snapshot synchronization, visible hit presentation and
two-human combat acceptance remain open.
