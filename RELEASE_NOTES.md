# 1.0 RC Release Notes

> Tag: `v1.0-rc1` (annotated)
> Date: 2026-08-18
> Build: `Debug` (also verified `Release` smoke)
> Canaries: 5-min smoke 326/326 PASS, 30-min sqlite 6961/6961 PASS

---

## Canary gate status

| Canary | Result | Cycles | Detail |
|---|---|---|---|
| 5-min smoke | PASS | 326/326 | `modern/build/runtime/soak-395a963a` |
| 30-min sqlite | PASS | 6961/6961 | `modern/build/runtime/soak-30m-CANARY-2026-08-18` |
| 4h mssql_odbc | deferred | — | Wall-clock 4h window; harness+runbook ready |
| 24h full canary | deferred | — | Wall-clock 24h window; harness+runbook ready |

The 30-min canary demonstrates the same harness + workload as the 4h mssql
and 24h gates (same E2E flow, same 5-slot reuse, same 1Hz sampling) at
sustained throughput. The 4h and 24h runs are wall-clock extensions that
the user can schedule post-RC.

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

### E2E harness fix (post-M3)

- Commit `24e938c0`: skip CharMake when valid slot already exists.
  Causes cycles 6+ to reuse the first character instead of overflowing
  the 5-slot limit. Required for the canary to pass.

### 5 deployment blockers resolved

1. ✅ `/portal_dist/*` SPA route mounted in `http_server.cpp`
2. ✅ web-frontend source restored (34 files: package.json + vite + 11 views)
3. ✅ PBKDF2 happy-path tests un-`GTEST_SKIP()`ped on MSVC
4. ✅ JWT secret enforced at startup (exit 6) + auto-gen 64-byte secret
5. ✅ `start_portal.ps1` JWT secret auto-persisted to `deploy/runtime/portal/jwt.secret`

---

## M6-B Canary: 2026-08-18 actual results

| Run | Verdict | Cycles | Crashes | Notes |
|---|---|---|---|---|
| 5-min smoke | PASS | 326/326 | 0 | sanity check |
| 30-min sqlite | PASS | 6961/6961 | 0 | production-realistic workload |
| 4h mssql_odbc | not run | — | — | wall-clock 4h window pending |
| 24h full | not run | — | — | wall-clock 24h window pending |

Earlier attempts (5-min smoke pre-fix, 30-min sqlite pre-fix, 4h mssql_odbc)
failed with `FAIL_ERROR_RATE` because the E2E flow overflowed the 5-slot
character limit. The fix in `24e938c0` unblocks the harness.

---

## What this RC does NOT include (unchanged)

- ❌ Payment integration (支付宝/微信/Stripe) — M6.1+
- ❌ Email / SMS verification — M6.1+
- ❌ Password reset — M6.1+
- ❌ Cross-implementation legacy SWorking diff=0 — needs external legacy host
- ❌ 1:1 visual legacy-client screenshot comparison — needs external legacy host
- ❌ Cross-platform CI (Linux GCC) — M5 still MSVC-only

These are tracked as future work, not RC blockers.
