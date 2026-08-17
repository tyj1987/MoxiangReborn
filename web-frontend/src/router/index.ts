// src/router/index.ts
// Vue Router routes. Plan: PLAN_PORTAL.md §2.2.
// Base is /portal/ (set in vite.config.ts).

import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '@/views/HomeView.vue'
import NewsView from '@/views/NewsView.vue'
import NewsDetailView from '@/views/NewsDetailView.vue'
import RegisterView from '@/views/RegisterView.vue'
import LoginView from '@/views/LoginView.vue'
import AccountView from '@/views/AccountView.vue'
import ShopView from '@/views/ShopView.vue'
import DownloadView from '@/views/DownloadView.vue'
import StatusView from '@/views/StatusView.vue'
import AboutView from '@/views/AboutView.vue'
import NotFoundView from '@/views/NotFoundView.vue'
import { useAuthStore } from '@/stores/auth'

const router = createRouter({
  history: createWebHistory('/portal/'),
  routes: [
    { path: '/', name: 'home', component: HomeView },
    { path: '/news', name: 'news', component: NewsView },
    { path: '/news/:slug', name: 'news-detail', component: NewsDetailView, props: true },
    { path: '/register', name: 'register', component: RegisterView },
    { path: '/login', name: 'login', component: LoginView },
    { path: '/account', name: 'account', component: AccountView, meta: { requiresAuth: true } },
    { path: '/shop', name: 'shop', component: ShopView },
    { path: '/download', name: 'download', component: DownloadView },
    { path: '/status', name: 'status', component: StatusView },
    { path: '/about', name: 'about', component: AboutView },
    { path: '/:pathMatch(.*)*', name: 'not-found', component: NotFoundView },
  ],
  scrollBehavior(_to, _from, saved) {
    return saved ?? { top: 0 }
  },
})

router.beforeEach((to) => {
  if (to.meta.requiresAuth) {
    const auth = useAuthStore()
    if (!auth.isAuthenticated) {
      return { name: 'login', query: { from: to.fullPath } }
    }
  }
  return true
})

export default router
