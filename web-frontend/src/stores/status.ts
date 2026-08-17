// src/stores/status.ts
import { defineStore } from 'pinia'
import { ref } from 'vue'
import { getStatus, type StatusSnapshot } from '@/api/status'

export const useStatusStore = defineStore('status', () => {
  const snapshot = ref<StatusSnapshot | null>(null)
  const loading = ref(false)
  const error = ref<string>('')

  async function refresh() {
    loading.value = true
    error.value = ''
    try {
      const { data } = await getStatus()
      snapshot.value = data
    } catch (e: any) {
      error.value = e?.message ?? 'failed to fetch status'
    } finally {
      loading.value = false
    }
  }

  return { snapshot, loading, error, refresh }
})
