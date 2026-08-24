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
| `EVID-20260825-current-map10-blocked` | `cd06a441` | `playdh-current` | `start_modern.ps1 -Mode start -ResourceProfileId playdh-current -MapNumber 10` | correctly fails closed: current 22,766-byte `Monster_10.bin` variant is rejected by the AIGroup decoder |
| `EVID-20260825-full-ctest` | `26523a9c` | local Debug build | `ctest -C Debug --test-dir modern/build --output-on-failure` | 12,130 registered; 2 BGM playback tests fail with environment MCI error 277, 19 tests skip for explicit missing/optional fixtures; not an RC gate |

These are E1–E3 engineering gates only. They do not upgrade G4–G11 to E5 and do not replace a genuine legacy-client human run.
