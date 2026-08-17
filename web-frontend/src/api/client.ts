// src/api/client.ts
// Axios instance with JWT injection + 401 redirect to /login.

import axios, { AxiosError } from 'axios'
import { useAuthStore } from '@/stores/auth'

export const api = axios.create({
  baseURL: import.meta.env.VITE_API_BASE ?? '',
  timeout: 10_000,
})

api.interceptors.request.use((config) => {
  const auth = useAuthStore()
  if (auth.token) {
    config.headers ??= {}
    config.headers.Authorization = `Bearer ${auth.token}`
  }
  return config
})

api.interceptors.response.use(
  (resp) => resp,
  (err: AxiosError) => {
    if (err.response?.status === 401) {
      const auth = useAuthStore()
      auth.clear()
      const router = (window as any).__router__
      if (router && router.currentRoute.value.name !== 'login') {
        router.push({ name: 'login', query: { from: router.currentRoute.value.fullPath } })
      }
    }
    return Promise.reject(err)
  },
)
