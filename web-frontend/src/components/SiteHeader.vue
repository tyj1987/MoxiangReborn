<script setup lang="ts">
import { computed } from 'vue'
import { RouterLink } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()
const links = computed(() => [
  { name: 'home',     label: '首页 / Home' },
  { name: 'news',     label: '新闻 / News' },
  { name: 'shop',     label: '商城 / Shop' },
  { name: 'download', label: '下载 / Download' },
  { name: 'status',   label: '状态 / Status' },
  { name: 'about',    label: '关于 / About' },
])
</script>

<template>
  <header class="border-b border-border bg-elevated">
    <div class="container mx-auto px-4 max-w-6xl flex items-center justify-between py-4">
      <RouterLink to="/" class="flex items-center gap-3">
        <span class="font-serif text-2xl text-gold tracking-wider">墨香</span>
        <span class="text-text-muted text-sm hidden sm:inline">Moxian Portal</span>
      </RouterLink>
      <nav class="flex items-center gap-1">
        <RouterLink
          v-for="l in links"
          :key="l.name"
          :to="{ name: l.name }"
          class="px-3 py-1.5 rounded text-sm text-text-secondary hover:text-gold-bright hover:bg-surface transition"
          active-class="text-gold"
        >
          {{ l.label }}
        </RouterLink>
      </nav>
      <div class="flex items-center gap-2">
        <template v-if="auth.isAuthenticated">
          <RouterLink
            :to="{ name: 'account' }"
            class="px-3 py-1.5 rounded text-sm text-gold-bright hover:text-gold"
          >
            {{ auth.account }}
          </RouterLink>
          <button
            class="px-3 py-1.5 rounded text-sm border border-vermillion text-vermillion hover:bg-vermillion hover:text-text-primary transition"
            @click="auth.doLogout()"
          >
            登出 / Logout
          </button>
        </template>
        <template v-else>
          <RouterLink
            :to="{ name: 'login' }"
            class="px-3 py-1.5 rounded text-sm text-text-secondary hover:text-gold-bright"
          >
            登录 / Login
          </RouterLink>
          <RouterLink
            :to="{ name: 'register' }"
            class="px-3 py-1.5 rounded text-sm border border-gold text-gold hover:bg-gold hover:text-dark transition"
          >
            注册 / Register
          </RouterLink>
        </template>
      </div>
    </div>
  </header>
</template>
