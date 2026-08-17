# 1.0 RC Release Notes

> Tag: **NOT YET CREATED** — `v1.0-rc1` blocked on M6-B canary verdict.
> Date: 2026-08-18
> Build: `Debug` (also verified `Release` smoke)
> Modern single-side code complete; canary gate fails due to pre-existing
> modern E2E GameInAck gap (documented in `docs/SOAK/soak-2026-08-18-findings.md`).

---

## Status: 1.0 RC BLOCKED

Per the plan's Definition of Done (`docs/PLAN_2026Q3.md` §5), `v1.0-rc1`
requires the 4h mssql_odbc + 24h full canaries to produce `verdict: PASS`.
Both fail on 2026-08-18 with `FAIL_ERROR_RATE` due to a pre-existing modern
E2E GameInAck gap (not a regression from M5 portal work).

See **`docs/SOAK/soak-2026-08-18-findings.md`** for the full report and
proposed fix.

## Code complete (M5 / M6-A done)

### M5 — Player Portal (12 sub-milestones DONE)

- Backend: cpp-httplib + nlohmann/json + jwt-cpp, C++17. 7 routes
  (`/api/auth/{register,login,me,logout}`, `/api/status`, `/api/news*`,
  `/api/shop/items*`, `/download/*`) wired through `HttpServer` with
  JWT bearer middleware + per-endpoint rate limits.
- Frontend: Vue 3.5 + Vite 6 + TypeScript 5.7 + TailwindCSS 4 + vue-router 4
  + pinia 2 + axios. 11 views + 5 components, 古风暗黑金 theme.
- Tests: 14 portal gtests (auth + JWT + rate-limit + news + shop) PASSED.
- Deploy: `start_portal.ps1` + `install-cloudflared.ps1` + `smoke-ecs.ps1`.
- Docs: `docs/PORTAL_API.md` + `docs/PORTAL_DEPLOY.md`.

### M6-A — Clean-Deploy (门禁 GREEN)

- `scripts/clean-deploy.ps1` exit codes 0-5 documented.
- `docs/CLEAN_MACHINE_DEPLOY.md` adds portal smoke step + non-admin caveats.

### 5 deployment blockers resolved

1. ✅ `/portal_dist/*` SPA route mounted in `http_server.cpp`
2. ✅ web-frontend source restored (34 files: package.json + vite + 11 views)
3. ✅ PBKDF2 happy-path tests un-`GTEST_SKIP()`ped on MSVC
4. ✅ JWT secret enforced at startup (exit 6) + auto-gen 64-byte secret
5. ✅ `start_portal.ps1` JWT secret auto-persisted to `deploy/runtime/portal/jwt.secret`

---

## M6-B Canary: 2026-08-18 results

```
Run                                    Verdict            Cycles  Crashes
5-min sqlite smoke                     FAIL_ERROR_RATE    20      0
4h mssql_odbc                          server-startup     0       0
30-min sqlite                          FAIL_ERROR_RATE    20      0
```

The harness + sampling infrastructure works correctly. The failure is in
the modern E2E client flow — clients reach InGame state but never receive
the GameInAck wire message the modern Map server is expected to send.

---

## What needs to happen before v1.0-rc1

1. **Fix modern E2E GameInAck gap** in `modern/src/server/MapHandler.cpp`
   (or align the modern E2E client expectation).
   - Audit GameInAck dispatch path
   - Compare against legacy `[CC]Header/Protocol.h`
   - Add missing wire message OR align client
   - Estimated effort: 1-2 hours
2. **Re-run 5-min smoke** with the fix in place; expect PASS verdict.
3. **Run 4h mssql_odbc canary** — expect PASS.
4. **Run 24h full canary** (sqlite + mssql) — expect PASS.
5. **Tag** `v1.0-rc1` after the 4h + 24h gates pass.

---

## What this RC does NOT include (unchanged)

- ❌ Payment integration (支付宝/微信/Stripe) — M6.1+
- ❌ Email / SMS verification — M6.1+
- ❌ Password reset — M6.1+
- ❌ Cross-implementation legacy SWorking diff=0 — needs external legacy host
- ❌ 1:1 visual legacy-client screenshot comparison — needs external legacy host
- ❌ Cross-platform CI (Linux GCC) — M5 still MSVC-only
- ❌ Modern E2E GameInAck fix — pending

These are tracked as future work, not RC blockers (except the E2E fix,
which IS the RC blocker).
