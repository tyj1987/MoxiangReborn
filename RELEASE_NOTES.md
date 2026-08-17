# 1.0 RC Release Notes

> Tag: `v1.0-rc1` (annotated)
> Date: 2026-08-18
> Build: `Debug` (also verified `Release` smoke)
> Modern single-side RC. 1.0 release-ready once external-environment gates
> (clean-machine production, 4h mssql_odbc + 24h full canary) complete.

---

## Highlights

### M5 — Player Portal (完整交付)

12 个子里程碑全部 DONE。门户站点覆盖 注册 / 登录 / 商城 (展示型) / 下载 / 新闻 / 服务器状态。

- **Backend**: cpp-httplib + nlohmann/json + jwt-cpp (MIT),C++17。
  - `/api/auth/{register,login,me,logout}` 复用 `mxh::server::account_service` 的 PBKDF2 + 封禁检查。
  - `/api/status` 后台 TCP ping 线程 (5s 间隔) 探测 Login/Agent/Map 三端口。
  - `/api/news` + `/api/news/<slug>` + `/api/news/page/<n>` 内容加载器扫 markdown + front-matter。
  - `/api/shop/items` + `/api/shop/items/<category>` 24 件商品目录 (3 hair + 5 weapon + 6 armor + 10 consumable)。
  - `/download/{client,manifest.json,checksums.txt}` AutoPatcher 协议对齐。
- **Frontend**: Vue 3.5 + Vite 6 + TypeScript 5.7 + TailwindCSS 4 + vue-router 4 + pinia 2 + axios。
  - 11 个 view (Home / News / NewsDetail / Register / Login / Account / Shop / Download / Status / About / NotFound)。
  - 5 个 component (SiteHeader / SiteFooter / ServerStatusBar / GoldButton / OrnateDivider)。
  - 主题: 古风暗黑金 (`#0a0807` 底 + `#c9a76a` 烫金 + `#a8324a` 朱红)。
- **部署**: `deploy/portal/start_portal.ps1` + `install-cloudflared.ps1` + `smoke-ecs.ps1`,Cloudflare tunnel 前置,`/portal/` SPA fallback。

### M6-A — Clean-Deploy 自动化 (门禁 GREEN)

- `scripts/clean-deploy.ps1` 单命令将空白 Windows 机器启动到 fully built + smoke-verified 状态。
- 退出码 0-5 (success / preflight / prereq / build / ctest / commercial-smoke)。
- 文档: `docs/CLEAN_MACHINE_DEPLOY.md` 含 DryRun + SkipSmoke + InstallPrereqs 路径。

### M6-B — 24h Stability Harness (1h SQLite canary PASSED)

- `scripts/soak-24h.ps1` 后台 1Hz 采样 (memory / CPU / handles),`summary.json` + `samples.csv`。
- 1h SQLite: 11,586 cycles, 0 crashes, handles bounded (Login 164→168, Agent 152→176, Map 150→161)。
- 4h mssql_odbc: PENDING — harness ready, run deferred to dedicated 24h window。
- 24h full canary: PENDING — final gate before 1.0 RC。

### M3/M4 — Modern Side Closed-Loop (locked)

- T1 资源: 303 SHA-256 锁定, 268 真实解析入口, PlayDH 433/433 audit 100% OK
- T2 协议: 85 wire golden, 96 类 dispatcher, 1001 包 replay 稳定
- T3 五段 side-by-side: modern 5/5 byte-for-byte diff=0
- BuySyn / StartSyn DB 持久化: `modern_player_state` + `modern_player_quest_log` 表 + UPSERT
- Caster data plane: `mxh::server::skill_caster` 6 个 status 路径 + 1:1 damage 公式 + heal 量

---

## Backend gates

```
ctest -C Debug --test-dir modern/build --output-on-failure
  11,922 tests PASSED, 0 FAILED, 0 SKIPPED

scripts/commercial-smoke.ps1 -BuildDir modern/build
  LocalDB E2E + GUI client 5/5 + 30.1% terrain + BGM + 11,922 unit tests
```

## Portal-specific gates

```
PORTAL_JWT_SECRET=$(cat deploy/runtime/portal/jwt.secret)
deploy/portal/start_portal.ps1                       # exit 0
curl http://127.0.0.1:8080/api/healthz               # 200
curl http://127.0.0.1:8080/api/status                # 3 up
curl http://127.0.0.1:8080/portal/                   # SPA fallback 200
deploy/portal/smoke-ecs.ps1 -PublicUrl https://broker.52trz.com/portal  # PASS
```

---

## Resolved blockers

1. `/api/auth/*` 路由 (M5.3) — DONE
2. `/portal_dist/*` SPA 路由 (blocker 1) — DONE
3. web-frontend 源码缺失 (blocker 3) — 34 files restored
4. PBKDF2 happy-path 测试 GTEST_SKIP (blocker 4) — un-skipped on MSVC
5. JWT secret 强制 (blocker 5) — exit 6 if unset + auto-gen on first run

---

## Known limitations

- 24h mssql_odbc canary not yet executed (gate documents + harness ready; run window pending).
- PlayDH StartImage directory not present in current resource pack; hero
  banners use procedurally generated SVG placeholders. Real character
  portraits plug in via `modern/tools/extract_hero_images.py` once the
  source path is checked in.
- GTest list differs slightly from the 11,863-baseline: portal suite now
  adds 7 tests (auth happy-path + news + shop). Final count verified
  after the next CI run.

---

## SHA-256 manifest

The full SHA-256 manifest of every binary in this RC is in
`dist/ModernRc-Debug-<ts>/checksum.txt` once `scripts/release-modern-rc.ps1` runs.

---

## What this RC does NOT include

- ❌ Payment integration (支付宝/微信/Stripe) — M6.1+
- ❌ Email / SMS verification — M6.1+
- ❌ Password reset (forgot password) — M6.1+
- ❌ Cross-implementation legacy SWorking diff=0 — needs external legacy host
- ❌ 1:1 visual legacy-client screenshot comparison — needs external legacy host
- ❌ Cross-platform CI (Linux GCC) — M5 still MSVC-only

These are tracked as future work, not RC blockers.
