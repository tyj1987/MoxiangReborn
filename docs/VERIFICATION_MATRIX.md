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
| G0 | Git/source/resource protection | In progress | bundle, backup refs, dual-copy hash report |
| G1 | Profiles, paths, secrets | In progress | governance scan, profile hash manifests |
| G2 | Clean tree and current docs | In progress | clean status, clean build, no self-junction |
| G4 | Launcher/login/display transition | Not started | human 800×600 → 1024×768 run |
| G5 | Character and loading path | Not started | fresh/existing account video and logs |
| G6 | Map10 world presentation | Not started | legacy/modern goldens, zero placeholders |
| G7 | Core UI live binding | Not started | per-dialog action/error/close evidence |
| G8 | Movement/combat/loot/map change | Not started | two-client human scenario |
| G9 | Effects and audio | Not started | event timelines, video and audio trace |
| G10 | Full assets/maps/UI | Not started | coverage matrix with no unexplained gaps |
| G11 | RC stability | Not started | clean-machine, fault injection, 24-hour soak |

## Invalid evidence

The following are never sufficient for an E5 claim: parser totals, CTest totals, headless state tests, auto-login screenshots, or modern-vs-modern SSIM.
