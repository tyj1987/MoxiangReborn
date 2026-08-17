// vite.config.ts
// Player portal SPA. The portal HTTP server mounts the bundle at /portal_dist/
// (see modern/src/portal/http_server.cpp). The SPA itself uses /portal/ as the
// Vue Router base (so the address bar shows /portal/login, /portal/news, etc.)
// while static assets live under /portal_dist/assets/.

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import tailwindcss from '@tailwindcss/vite'
import { fileURLToPath, URL } from 'node:url'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [vue(), tailwindcss()],
  base: '/portal/',
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    emptyOutDir: true,
    sourcemap: false,
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  server: {
    port: 5173,
    proxy: {
      // During dev, forward /api to a local portal server on :8080.
      '/api': 'http://127.0.0.1:8080',
      '/portal_dist/assets': {
        target: 'http://127.0.0.1:8080',
        rewrite: (path) => path.replace(/^\/portal_dist\/assets/, '/portal_dist/assets'),
      },
    },
  },
})
