// src/stores/auth.ts
import { defineStore } from 'pinia'
import { computed, ref } from 'vue'
import * as authApi from '@/api/auth'

const TOKEN_KEY = 'moxian.token'
const ACCOUNT_KEY = 'moxian.account'

export const useAuthStore = defineStore('auth', () => {
  const token = ref<string>(localStorage.getItem(TOKEN_KEY) ?? '')
  const account = ref<string>(localStorage.getItem(ACCOUNT_KEY) ?? '')
  const user_idx = ref<number>(0)

  const isAuthenticated = computed(() => !!token.value)

  async function doLogin(accountName: string, password: string) {
    const { data } = await authApi.login(accountName, password)
    token.value = data.token
    account.value = data.account
    user_idx.value = data.user_idx
    localStorage.setItem(TOKEN_KEY, data.token)
    localStorage.setItem(ACCOUNT_KEY, data.account)
    return data
  }

  async function doRegister(accountName: string, password: string, confirm: string) {
    return authApi.register(accountName, password, confirm)
  }

  async function fetchMe() {
    if (!token.value) return null
    const { data } = await authApi.me()
    account.value = data.account
    user_idx.value = data.user_idx
    return data
  }

  async function doLogout() {
    try {
      await authApi.logout()
    } finally {
      clear()
    }
  }

  function clear() {
    token.value = ''
    account.value = ''
    user_idx.value = 0
    localStorage.removeItem(TOKEN_KEY)
    localStorage.removeItem(ACCOUNT_KEY)
  }

  return {
    token,
    account,
    user_idx,
    isAuthenticated,
    doLogin,
    doRegister,
    fetchMe,
    doLogout,
    clear,
  }
})
