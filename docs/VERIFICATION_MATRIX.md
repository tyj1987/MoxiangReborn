# Verification matrix

This is the single current evidence index. A row is complete only when it has a commit, resource-profile manifest, command, artifact ID and the required evidence level.

## Evidence levels

| Level | Meaning |
|---|---|
| E0 | File/package bytes and SHA-256 recorded |
| E1 | Format decoded with complete field coverage |
| E2 | Runtime object instantiated with no missing dependency |
| E3 | Runtime renders or plays the asset |
| E4 | Correct player action triggers the asset and state change |
| E5 | Genuine legacy comparison plus human review passes |

## Current gates

| Gate | Scope | Status | Required evidence |
|---|---|---|---|
| G0 | Git/source/resource protection | PASS (42d14756) | governance bundle + backup refs + dual-copy hash report |
| G1 | Profiles, paths, secrets | PASS (42d14756) | explicit profiles, path scan, secret fallback removed |
| G2 | Clean tree and current docs | PASS (42d14756, f8ffa596) | clean status, successful Debug build, historical docs removed, remaining machine-path defaults eliminated |
| G3 | Client state/display/loading foundation | PASS (77e598cb,339351a0, b2993ae4) | client state tests, typed loading coordinator, atomic settings, explicit runtime profile validation |
| G4 | Launcher/login/display transition | Partial | transactional 800×600 → saved post-login mode is implemented; genuine human run, updater GUI and failure matrix still required |
| G5 | Character and loading path | Partial | appearance fields and typed loading path; 3D preview/video still required |
| G6 | Map10 world presentation | Not started | legacy/modern goldens, zero placeholders |
| G7 | Core UI live binding | Not started | per-dialog action/error/close evidence |
| G8 | Movement/combat/loot/map change | Not started | two-client human scenario |
| G9 | Effects and audio | Not started | event timelines, video and audio trace |
| G10 | Full assets/maps/UI | Not started | coverage matrix with no unexplained gaps |
| G11 | RC stability | Not started | clean-machine, fault injection, 24-hour soak |

## Invalid evidence

The following are never sufficient for an E5 claim: parser totals, CTest totals, headless state tests, auto-login screenshots, or modern-vs-modern SSIM.

## Recent reproducible evidence

| Evidence ID | Commit | Profile | Command | Result |
|---|---|---|---|---|
| `EVID-20260825-client-158` | `cd06a441` (includes `5936e521`, `6b1ef0f0`) | test fixtures | `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_brief=1` | 158/158 passed |
| `EVID-20260825-render-282` | `cd06a441` | test fixtures + `playdh-current` package parsing | `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_brief=1` | 282/282 passed; synthetic missing-model warnings remain test-only |
| `EVID-20260825-governance` | `cd06a441` | repository manifests | `python scripts/check-project-governance.py` | passed |
| `EVID-20260825-release-client` | `dc5b567a` | `playdh-current` | `scripts\\build-modern.bat Release mxh_client` | x86 Release build passed; automation/credential CLI rejected under `NDEBUG` |
| `EVID-20260825-render-texture-gate` | `11539716` | `playdh-current` | `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_brief=1` | 282/282 passed; Release world loading now rejects unresolved terrain/STM textures |
| `EVID-20260825-map10-closure` | `999b8965` | `playdh-current` | `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_brief=1` | 283/283 passed; real Map10 HFL/STM and texture dependencies resolve through filesystem/PAK storage |
| `EVID-20260825-canonical-resource-tests` | `1de46337` | `playdh-current` | `ctest -C Debug --test-dir modern/build -R "PackFile|ReadMhBin_" --output-on-failure` | 100% passed; 3 tests skipped only for explicitly absent optional files |
| `EVID-20260825-reference-map10-start` | `cd06a441` | `sworking-2008-reference` (development-only) | `start_modern.ps1 -Mode start -ResourceProfileId sworking-2008-reference -MapNumber 10 -AllowDevFallbacks` | MapServer loaded 114 groups / 228 spawns; reference profile is not release-enabled |
| `EVID-20260825-current-map10-blocked-v2` | `5e75e43c` | `playdh-current` (`server-size-prefixed-opaque-v1`) | `start_modern.ps1 -Mode start -ResourceProfileId playdh-current -MapNumber 10 -Backend sqlite` | correctly fails closed with an explicit unsupported-encoding diagnostic for the current 22,766-byte `Monster_10.bin`; no reference-profile substitution occurs and no server process remains running |
| `EVID-20260825-full-ctest-v2` | `38fe583c` | local Debug build | `ctest -C Debug --test-dir modern/build --output-on-failure` | 12,134 registered; 12,134 passed; 19 tests skip for explicit missing/optional fixtures. The size-prefixed opaque-server-profile detector and classic MHFileEx regression tests are included; this is still not an RC gate because the skipped runtime/resource gates and human path remain open. |
| `EVID-20260825-settings-durable` | `7f262a5a` | local Windows temp profile | `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=ClientSettings.*` | settings round-trip passed after `FlushFileBuffers` and `MoveFileExW(MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING)`; display/audio settings publication is durable and atomic. |
| `EVID-20260825-ui-resolution-propagation` | `13f838a5` | client state fixture | `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=CEngine.UiResolutionModeStartsAtLoginAndCanBePromotedAfterDisplayCommit` | passed; login starts at 800×600 and the committed post-login mode is propagated to CharSelect, CharMake and GameIn UI loading instead of remaining hard-coded to Low800x600. |
| `EVID-20260825-client-160` | `13f838a5` | test fixtures + PlayDH UI resources | `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_brief=1` | 160/160 passed, including login recovery, state transfer, UI runtime, GameIn interactions and post-login resolution mode propagation. |
| `EVID-20260825-mouse-capture` | `1d9c4983` | client window input path | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_brief=1` | x86 Debug client build and 160/160 client tests passed; button gestures now capture/release the HWND so drag rotation and release remain deterministic at client/pillarbox edges. |
| `EVID-20260825-target-picking` | `36cbe1be` | in-game input fixture | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=InGamePlayable.* --gtest_brief=1` | 6/6 playable-input tests passed; a left click now selects the live monster under the cursor, while empty-world clicks retain nearest-target fallback. |
| `EVID-20260825-mapchange-failure-state` | `571f880e` | client state fixture | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=CMapChange.* --gtest_brief=1` | 3/3 MapChange tests passed; progress, cancellation, zero-step rejection and explicit failure text are now observable by the host state coordinator. |
| `EVID-20260825-camera-wheel-zoom` | `27f54850` | in-game input + DX11 terrain camera | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=InGamePlayable.MouseWheelZoomIsBounded --gtest_brief=1` | camera distance is now driven by `WM_MOUSEWHEEL`, clamped to 3–12 world units, and propagated to the terrain camera. |
| `EVID-20260825-map-bounds` | `dc6c63ee` | Map HFL dimensions + movement fixture | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=InGameMovement.* --gtest_brief=1` | movement now accepts map-specific width/height bounds from the loaded HFL instead of relying only on the fixed 50,000-unit fallback; 7/7 movement tests passed. |
| `EVID-20260825-static-collision-query` | `35bf5600` | STM collision descriptors + in-game movement fixture | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/mxh_compat_tests.exe --gtest_filter=StmStaticModel.* --gtest_brief=1` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=InGamePlayable.StaticCollisionQueryRejectsCandidateStep --gtest_brief=1` | STM collision-model bounding descriptors are parsed and exposed through an explicit query; blocked candidate movement is rejected. Exact triangle collision remains pending the legacy `.col` sidecar integration. |
| `EVID-20260825-placeholder-release-gate` | `ad314a54` | `playdh-current` client render policy | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_filter=EntityScene* --gtest_brief=1` + `python scripts/check-project-governance.py` | 6/6 EntityScene regression tests and governance check passed; RenderBox stand-ins are disabled by default and only enabled by explicit `--debug-ui-bounds`. Missing models remain counted and fail the Map10 visual gate rather than being presented as game assets. |
| `EVID-20260825-server-profile-fail-closed` | `2969aad9` | `playdh-current` and `sworking-2008-reference` | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/mxh_compat_tests.exe --gtest_filter=MhFileEx.*` + `modern/build/tools/MoxianMapServer/mxh_map_server_KOR.exe --map 10 --resource-root modern\\data\\PlayDH --db modern\\build\\runtime\\plan_probe.db` | 10/10 MHFileEx profile tests passed; current 22,766-byte opaque Monster_10.bin is rejected through the explicit profile API, while the reference profile remains selectable only by explicit ID. MapServer exits with a clear unsupported-encoding diagnostic instead of positional-decoding garbage. |
| `EVID-20260825-profile-propagation` | `e41fe1a6` | `playdh-current` | `pwsh -NoProfile -File deploy\\scripts\\start_modern.ps1 -Mode start -DryRun -ResourceProfileId playdh-current -MapNumber 10` | Dry-run shows the selected profile and `--resource-profile playdh-current` on MapServer; profile metadata now identifies server-size-prefixed-opaque-v1 separately from text encoding Big5. |
| `EVID-20260825-loading-transaction` | `9e277c01` | client loading fixture + runtime load path | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_filter=GameLoadingCoordinator.*:CGameLoading.*:CMapChange.* --gtest_brief=1` | 8/8 loading/state tests passed; world loading reports ordered stages and leaves the previous scene render/capture state intact when descriptor, terrain, STM, sky or entity activation fails. Late progress callbacks cannot regress the visible stage count. |
| `EVID-20260825-client-profile-gate` | `cea7a845` | client profile selection | `cmd /c scripts\\build-modern.bat` + `modern/build/tools/MoxianClient/mxh_client.exe --resource-profile invalid` + `modern/build/tools/MoxianClient/mxh_client.exe --resource-profile sworking-2008-reference --resource-root modern\\data\\PlayDH` | Unknown profiles and root/profile mismatches are rejected before resource activation; human launcher now propagates the selected profile. Reference profile remains development-only in Release. |
| `EVID-20260825-playdh-manifest-clean` | `bdf9178c` | `playdh-current` canonical tree | `pwsh -NoProfile -File scripts\\generate-resource-manifest.ps1 -Root modern\\data\\PlayDH -Output reference\\manifests\\playdh-current.sha256.json -ProfileId playdh-current` + `python scripts/check-project-governance.py` | Removed four exact SQLite runtime artifacts from the canonical resource root into recoverable quarantine; regenerated manifest now covers 5,135 files / 1,681,141,995 bytes and contains no `moxian_*.db`, WAL or SHM entries. Governance passed. |

| `EVID-20260825-launcher-shell` | `d2d5a525` | `playdh-current` | `cmd /c scripts\\build-modern.bat` + `python scripts/check-project-governance.py` | New Win32 launcher builds in the clean Debug graph. It persists only non-secret launch settings, passes explicit profile/display parameters to the client, never accepts credentials, and blocks check/repair until a signed manifest transport is configured. |
| `EVID-20260825-display-dpi-transition` | `1d8c6e06` | client display path | `cmd /c scripts\\build-modern.bat` + `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_brief=1` | x86 Debug build and 167/167 client tests passed; post-login client-area sizing now uses the current monitor DPI when available and retains rollback behavior on failure. |
| `EVID-20260825-launcher-settings-json` | `472c656d` | launcher settings | `cmd /c scripts\\build-modern.bat` + `python scripts/check-project-governance.py` | Launcher settings now use `%LOCALAPPDATA%\\Moxian\\settings.json`, validate the profile and dimensions on load, and atomically replace the file on save; no credential field or command-line credential path exists. |
| `EVID-20260825-launcher-client-resolution` | `f3ea2006` | local launcher/client layout | `cmd /c scripts\\build-modern.bat` + `python scripts/check-project-governance.py` | Launcher now probes only the explicit release name `MoxianClient.exe` and modern build name `mxh_client.exe`, with a clear failure if neither exists; no arbitrary executable search is permitted. |

These are E1–E3 engineering gates only. They do not upgrade G4–G11 to E5 and do not replace a genuine legacy-client human run.
