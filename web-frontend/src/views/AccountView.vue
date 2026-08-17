<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { me, type AuthMe } from '@/api/auth'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()
const profile = ref<AuthMe | null>(null)
const error = ref('')

onMounted(async () => {
  try {
    const { data } = await me()
    profile.value = data
  } catch (e: any) {
    error.value = e?.response?.data?.error ?? '加载失败'
  }
})
</script>

<template>
  <section class="max-w-lg mx-auto bg-elevated border border-border rounded p-6">
    <h1 class="font-serif text-2xl text-gold mb-4">账号 / Account</h1>
    <p v-if="error" class="text-vermillion">{{ error }}</p>
    <div v-else-if="!profile" class="text-text-muted">加载中…</div>
    <dl v-else class="grid grid-cols-2 gap-3 text-sm">
      <dt class="text-text-muted">账号 / Account</dt><dd class="text-gold-bright">{{ profile.account }}</dd>
      <dt class="text-text-muted">用户 ID / User idx</dt><dd>{{ profile.user_idx }}</dd>
      <dt class="text-text-muted">点券 / Points</dt><dd>{{ profile.points }}</dd>
      <dt class="text-text-muted">注册 / Registered</dt><dd>{{ profile.registerdate || '—' }}</dd>
      <dt class="text-text-muted">最后登录 / Last login</dt><dd>{{ profile.lastlogindate || '—' }}</dd>
      <dt class="text-text-muted">登录 IP</dt><dd>{{ profile.lastloginip || '—' }}</dd>
    </dl>
    <button
      class="mt-6 px-4 py-2 rounded border border-vermillion text-vermillion hover:bg-vermillion hover:text-text-primary transition"
      @click="auth.doLogout()"
    >
      登出 / Logout
    </button>
  </section>
</template>
