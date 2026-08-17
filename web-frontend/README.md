# 墨香 Moxiang — Player Portal (Frontend)

Vue 3 + Vite + TypeScript + TailwindCSS 4 SPA. Served by the C++ Portal gateway
at `/portal/` (SPA fallback) and `/portal_dist/assets/` (static assets).

## Stack

- Vue 3.5
- Vue Router 4
- Pinia 2 (auth + status state)
- Axios 1.7+ with JWT bearer interceptor + 401 redirect
- TailwindCSS 4 (`@tailwindcss/vite` plugin)
- TypeScript 5.7 (strict)

## Local Development

```bash
pnpm install
pnpm dev        # http://localhost:5173 (proxies /api -> :8080)
```

## Build

```bash
pnpm build      # type-check + vite build -> dist/
```

The `dist/` output is mounted by the C++ portal at `/portal_dist/`. After
building, copy or symlink `dist/` into `deploy/portal/static/dist/` so the
gateway can serve it.

## Layout

```
src/
├── api/         # axios client + endpoints
├── assets/css/  # main.css with theme tokens
├── components/  # SiteHeader, SiteFooter, ServerStatusBar, GoldButton, OrnateDivider
├── router/      # Vue Router routes
├── stores/      # Pinia stores (auth, status)
├── views/       # 11 page components
├── App.vue
└── main.ts
```

## Theme

古风暗黑金 — see `src/assets/css/main.css` for color tokens:
- `--color-dark` / `--color-elevated` / `--color-surface` — backgrounds
- `--color-gold` / `--color-gold-bright` / `--color-gold-dim` — primary
- `--color-vermillion` / `--color-vermillion-bright` — accents
- `--color-text-primary` / `--color-text-secondary` / `--color-text-muted` — text
- `--font-serif` / `--font-sans` / `--font-mono` — typographic roles
