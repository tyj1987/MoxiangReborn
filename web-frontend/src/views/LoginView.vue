<script setup lang="ts">
import { ref } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import GoldButton from '@/components/GoldButton.vue'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()
const router = useRouter()
const route = useRoute()
const account = ref('')
const password = ref('')
const error = ref('')
const loading = ref(false)

async function submit() {
  error.value = ''
  loading.value = true
  try {
    await auth.doLogin(account.value, password.value)
    const dest = (route.query.from as string) || '/account'
    router.push(dest)
  } catch (e: any) {
    error.value = e?.response?.data?.error ?? '登录失败 / Login failed'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <section class="max-w-md mx-auto bg-elevated border border-border rounded p-6">
    <h1 class="font-serif text-2xl text-gold mb-4">登录 / Login</h1>
    <form @submit.prevent="submit" class="space-y-3">
      <input
        v-model="account"
        placeholder="账号 / Account"
        class="w-full bg-dark border border-border rounded px-3 py-2 text-text-primary focus:border-gold outline-none"
        required
      />
      <input
        v-model="password"
        type="password"
        placeholder="密码 / Password"
        class="w-full bg-dark border border-border rounded px-3 py-2 text-text-primary focus:border-gold outline-none"
        required
      />
      <p v-if="error" class="text-vermillion text-sm">{{ error }}</p>
      <GoldButton type="submit" :disabled="loading">登录 / Login</GoldButton>
    </form>
  </section>
</template>
