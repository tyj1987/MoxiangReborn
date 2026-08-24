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
| G4 | Launcher/login/display transition | Partial | human 800×600 → 1024×768 run still required |
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
| `EVID-20260825-client-158` | current `f8ffa596` | test fixtures | `modern/build/tests/unit/client/mxh_client_tests.exe --gtest_brief=1` | 158/158 passed |
| `EVID-20260825-render-282` | current `f8ffa596` | test fixtures + `playdh-current` package parsing | `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_brief=1` | 282/282 passed; synthetic missing-model warnings remain test-only |
| `EVID-20260825-governance` | `f8ffa596` | repository manifests | `python scripts/check-project-governance.py` | passed |
| `EVID-20260825-release-client` | `dc5b567a` | `playdh-current` | `scripts\\build-modern.bat Release mxh_client` | x86 Release build passed; automation/credential CLI rejected under `NDEBUG` |
| `EVID-20260825-render-texture-gate` | `11539716` | `playdh-current` | `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_brief=1` | 282/282 passed; Release world loading now rejects unresolved terrain/STM textures |
| `EVID-20260825-map10-closure` | `999b8965` | `playdh-current` | `modern/build/tests/unit/render/mxh_render_tests.exe --gtest_brief=1` | 283/283 passed; real Map10 HFL/STM and texture dependencies resolve through filesystem/PAK storage |

These are E1–E3 engineering gates only. They do not upgrade G4–G11 to E5 and do not replace a genuine legacy-client human run.
