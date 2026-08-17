# Moxian Portal — ECS Deployment Guide

> Status: M5.13 + M5.14 implemented. Deploys on a single ECS using
> `start_portal.ps1` + `cloudflared` tunnel.

---

## Pipeline

```
[Client]
   │ HTTPS 443
   ▼
[Cloudflare Tunnel] broker.52trz.com / portal.*
   │ 127.0.0.1:<local tunnel port>
   ▼
[ECS localhost:8080] mxh_portal.exe
   │ SQL queries
   ▼
[modern/data/.../moxian.db] sqlite (development) OR
[mssql_odbc / SQL Server 2022] (production)
```

---

## Prerequisites

- Windows 11 / Server 2022 with PowerShell 5.1+
- `mxh_portal.exe` built (see `docs/CLEAN_MACHINE_DEPLOY.md`)
- (Optional) `cloudflared` installed — see `install-cloudflared.ps1`

---

## Step 1 — Build the portal binary

```powershell
cd C:\moxiang
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build-modern.ps1 -Config Debug
```

Output: `modern\build\tools\MoxianPortal\Debug\mxh_portal.exe`.

---

## Step 2 — Generate the Vue bundle

```bash
cd C:\moxiang\web-frontend
pnpm install
pnpm build
```

Output: `web-frontend/dist/`. Copy the contents to `deploy/portal/static/dist/`:

```powershell
Copy-Item -Recurse -Force C:\moxiang\web-frontend\dist\* C:\moxiang\deploy\portal\static\dist\
```

---

## Step 3 — Set the JWT secret

`PORTAL_JWT_SECRET` must be set. `start_portal.ps1` auto-generates a 64-byte
secret if the env var is unset and persists to `deploy/runtime/portal/jwt.secret`
(file mode 0600).

For production, generate the secret explicitly:

```powershell
$bytes = New-Object byte[] 64
(New-Object Random).NextBytes($bytes)
[Convert]::ToBase64String($bytes) | Set-Content -LiteralPath deploy\runtime\portal\jwt.secret -Encoding utf8
$env:PORTAL_JWT_SECRET = (Get-Content -LiteralPath deploy\runtime\portal\jwt.secret -Raw).Trim()
```

If the env var is **empty** and `PORTAL_ALLOW_INSECURE_JWT=1` is **not** set,
the portal exits with code 6 — defending against accidental production
deployments without a secret.

---

## Step 4 — Start the portal

```powershell
cd C:\moxiang
powershell -NoProfile -ExecutionPolicy Bypass -File deploy\portal\start_portal.ps1
```

Verify:
```powershell
curl http://127.0.0.1:8080/api/healthz
curl http://127.0.0.1:8080/api/status
```

---

## Step 5 — Install cloudflared (optional)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File deploy\portal\install-cloudflared.ps1 -TunnelToken <token>
```

The tunnel forwards HTTPS traffic from broker.52trz.com to localhost:8080.

---

## Step 6 — ECS smoke test

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File deploy\portal\smoke-ecs.ps1 -PublicUrl https://broker.52trz.com/portal
```

Expected output:
```
OK   /api/healthz -> 200
OK   /api/status -> 200
OK   / -> 200
All ECS smoke endpoints OK
```

---

## Secrets Management

- Never commit `deploy/runtime/portal/jwt.secret` (in `.gitignore`).
- Rotate the secret every 90 days. Restart the portal after rotation.
- All players re-authenticate after a rotation (their old JWTs are invalid).

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Portal exits 6 at startup | JWT secret missing | Set `PORTAL_JWT_SECRET` or `PORTAL_ALLOW_INSECURE_JWT=1` for dev |
| `/api/status` shows all `"down"` | Game servers not running on 16001/17001/18001 | Start `MoxianLoginServer` / `MoxianAgentServer` / `MoxianMapServer` |
| `/portal/` returns 404 | Vue bundle not built | Run `pnpm --dir web-frontend build` and copy to `deploy/portal/static/dist/` |
| `/portal_dist/assets/*` returns 404 | `dist/` not in `static_root` | Verify `PORTAL_STATIC_ROOT` env var points to `deploy/portal/static` |
| Register returns 409 | Account exists | Use a different account name |
| Login returns 401 | Wrong password or banned | Check `modern_account_status` table |
