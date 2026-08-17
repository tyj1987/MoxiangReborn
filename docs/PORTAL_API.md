# Moxian Portal — HTTP API Reference

> Status: M5.3-5.14 implemented. Backend in C++ (cpp-httplib + nlohmann/json + jwt-cpp),
> frontend in Vue 3 + Vite. See `docs/PLAN_PORTAL.md` for the design doc.

All `/api/*` endpoints return JSON. All `/download/*` endpoints return JSON
or, when applicable, `application/zip`. All `/portal_dist/*` and `/static/*`
endpoints serve static files.

---

## Authentication

### `POST /api/auth/register`

Create a new account. Public (rate-limited: 5 / minute / IP).

| Field | Type | Required | Notes |
|---|---|---|---|
| `account`  | string | yes | 3-16 chars, ASCII letters / digits / underscore |
| `password` | string | yes | 8-16 chars, at least 1 letter + 1 digit |
| `confirm`  | string | yes | must equal `password` |

```bash
curl -X POST http://127.0.0.1:8080/api/auth/register \
     -H 'Content-Type: application/json' \
     -d '{"account":"alice","password":"secret123","confirm":"secret123"}'
```

Responses:
- `201` — `{"user_idx": 1, "account": "alice"}`
- `400` — invalid account / weak password / mismatch
- `409` — account already exists
- `429` — rate-limited

### `POST /api/auth/login`

Verify credentials and issue a JWT (HS256, 24h). Public (rate-limited: 10 / minute / IP).

```bash
curl -X POST http://127.0.0.1:8080/api/auth/login \
     -H 'Content-Type: application/json' \
     -d '{"account":"alice","password":"secret123"}'
```

Responses:
- `200` — `{"token": "<jwt>", "user_idx": 1, "account": "alice", "points": 0}`
- `401` — bad credentials / banned
- `429` — rate-limited
- `500` — JWT secret not configured (server misconfig)

### `GET /api/auth/me`

Return the caller's profile. Protected (JWT bearer).

```bash
curl http://127.0.0.1:8080/api/auth/me \
     -H 'Authorization: Bearer <jwt>'
```

Responses:
- `200` — `{"account": "...", "user_idx": 1, "points": 0, "registerdate": "...", "lastlogindate": "...", "lastloginip": "..."}`
- `401` — bad / missing JWT
- `404` — account not found

### `POST /api/auth/logout`

Stateless no-op; cleared client-side.

```bash
curl -X POST http://127.0.0.1:8080/api/auth/logout -H 'Authorization: Bearer <jwt>'
```

Responses:
- `204` — always succeeds

---

## Server Status

### `GET /api/status`

Return up/down snapshot for the three game servers (Login 16001 / Agent 17001 / Map 18001).
Updated every 5s by a background pinger thread.

```bash
curl http://127.0.0.1:8080/api/status
```

Response:
```json
{
  "login": "up",
  "agent": "up",
  "map": "up",
  "online_count": null,
  "last_check_at": "2026-08-18T12:00:00Z",
  "version": "1.0.0"
}
```

---

## News

### `GET /api/news`

List all news articles (newest first), page 1 / size 20.

```bash
curl http://127.0.0.1:8080/api/news
```

Response:
```json
{
  "items": [
    {"id": "welcome", "slug": "welcome", "title": "墨香归来", "summary": "...", "hero_image": "...", "published_at": "..."}
  ],
  "page": 1,
  "total": 3
}
```

### `GET /api/news/<slug>`

Return a single article by slug. 404 if missing.

```bash
curl http://127.0.0.1:8080/api/news/welcome
```

### `GET /api/news/page/<n>`

Paginated listing (10 per page).

```bash
curl http://127.0.0.1:8080/api/news/page/2
```

---

## Shop

### `GET /api/shop/items`

List shop items (read-only, no purchase). Default 24 per page.

```bash
curl http://127.0.0.1:8080/api/shop/items
```

Response:
```json
{
  "items": [
    {"idx": 1, "name": "墨染青丝", "category": "hair", "price_points": 50, "image_url": "...", "description": "..."}
  ],
  "page": 1,
  "total": 24
}
```

### `GET /api/shop/items/<category>`

Filter by category. `category` must be one of `hair`, `weapon`, `armor`, `consumable`.

```bash
curl http://127.0.0.1:8080/api/shop/items/weapon
```

---

## Downloads

### `GET /download/client`

Return a JSON body pointing to the latest client zip:

```bash
curl http://127.0.0.1:8080/download/client
```

Response:
```json
{ "url": "/static/downloads/MoxianClientSetup-1.0.0.zip", "method": "GET" }
```

The frontend follows this URL to download the actual zip.

### `GET /download/manifest.json`

AutoPatcher manifest — `{"version": "...", "files": [{"path": "...", "sha256": "...", "size": 0, "kind": "..."}]}`.

```bash
curl http://127.0.0.1:8080/download/manifest.json
```

### `GET /download/checksums.txt`

Plain-text list of SHA-256 hashes for the client package.

```bash
curl http://127.0.0.1:8080/download/checksums.txt
```

---

## Static

### `GET /static/<path>`

Static file serving from `deploy/portal/static/`. Path traversal (`..`, `\`) is rejected.

### `GET /portal_dist/<path>`

Vue SPA build artifacts (mounted at `static_root/dist/<path>`). Cached for 24h.

### `GET /portal/`

SPA fallback — returns `static_root/index.html`. Vue Router handles the rest.

---

## Error Format

All error responses follow this shape:

```json
{ "error": "human-readable message" }
```

When rate-limited, `Retry-After` header is set in seconds:

```json
{ "error": "rate limit exceeded", "retry_after_seconds": 30 }
```
