<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import GoldButton from '@/components/GoldButton.vue'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()
const router = useRouter()
const account = ref('')
const password = ref('')
const confirm  = ref('')
const error = ref('')
const loading = ref(false)

async function submit() {
  error.value = ''
  if (password.value !== confirm.value) {
    error.value = '两次输入的密码不一致 / Passwords do not match'
    return
  }
  loading.value = true
  try {
    await auth.doRegister(account.value, password.value, confirm.value)
    await auth.doLogin(account.value, password.value)
    router.push({ name: 'account' })
  } catch (e: any) {
    error.value = e?.response?.data?.error ?? '注册失败 / Registration failed'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <section class="max-w-md mx-auto bg-elevated border border-border rounded p-6">
    <h1 class="font-serif text-2xl text-gold mb-4">注册 / Register</h1>
    <form @submit.prevent="submit" class="space-y-3">
      <input
        v-model="account"
        placeholder="账号 / Account (3-16 chars)"
        class="w-full bg-dark border border-border rounded px-3 py-2 text-text-primary focus:border-gold outline-none"
        required minlength="3" maxlength="16"
      />
      <input
        v-model="password"
        type="password"
        placeholder="密码 / Password (8-16 chars)"
        class="w-full bg-dark border border-border rounded px-3 py-2 text-text-primary focus:border-gold outline-none"
        required minlength="8" maxlength="16"
      />
      <input
        v-model="confirm"
        type="password"
        placeholder="确认密码 / Confirm"
        class="w-full bg-dark border border-border rounded px-3 py-2 text-text-primary focus:border-gold outline-none"
        required minlength="8" maxlength="16"
      />
      <p v-if="error" class="text-vermillion text-sm">{{ error }}</p>
      <GoldButton type="submit" :disabled="loading">注册 / Register</GoldButton>
    </form>
  </section>
</template>
