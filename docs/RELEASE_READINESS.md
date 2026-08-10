# Commercial Release Readiness

> This matrix is evidence-driven. A green local test suite is not a substitute for legacy side-by-side or production dependency acceptance.

## Local gates

| Gate | Command/evidence | Current status |
|---|---|---|
| Debug build | `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-modern.ps1 -Config Debug` | PASS |
| Full CTest | `ctest -C Debug --test-dir modern/build --output-on-failure` | PASS baseline: 11,748/11,748; current discovery 11,749 with new targeted test PASS |
| Commercial smoke | `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/commercial-smoke.ps1 -BuildDir modern/build` | PASS: 33/33 + MSSQL five-step E2E |
| MSSQL modern schema | `MXH_MSSQL_E2E` gated `MssqlRealE2E` plus commercial smoke | PASS: ODBC 18 + LocalDB roundtrip |
| Governance | `python scripts/check-project-governance.py --ignore-root-artifacts` | PASS |
| Protocol/resource locks | Included in full CTest and smoke selections | PASS |
| DX11 headless frame | `RenderDemo.HeadlessFrameAcceptance` | PASS |
| GUI modern login | `mxh_client` against `start_modern.ps1`, client/server logs | PASS through LoginAck, Agent connect and CharacterListAck; real scene/UI pending |

## Required external acceptance

| Area | Required evidence | Status |
|---|---|---|
| Clean deployment | Fresh Windows install of ODBC/database/client/servers/tools with repeatable startup and rollback | Pending clean-machine exercise |
| Gameplay | Five side-by-side behavior scenarios with zero diff | Pending original client/server environment |
| Visual fidelity | Original login/empty-scene screenshots compared with modern frames | Pending legacy rendering environment |

## Repository hygiene

- Modern services default SQLite runtime files to `modern/build/runtime/`.
- Historical root `moxian.db*` files remain listed in `docs/CLEANUP_MANIFEST.md` and require explicit deletion confirmation.
- `modern/build/` is the only supported local build directory.

## Release rule

Do not mark the project 1.0 or commercially accepted until every external row above has reproducible evidence and the user approves the final cleanup/release decision. Physical HSEL hardware is explicitly outside the commercial RC scope; its software ABI and wire compatibility remain locked by tests.

Run the repeatable local prerequisite probe with:

`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-mssql-prereq.ps1`
