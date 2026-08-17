<script setup lang="ts">
import { computed } from 'vue'
import { useStatusStore } from '@/stores/status'

const status = useStatusStore()

function dotClass(state: string | undefined) {
  if (state === 'up')   return 'bg-green-500'
  if (state === 'down') return 'bg-red-500'
  return 'bg-gray-500'
}

const lastCheck = computed(() => {
  if (!status.snapshot?.last_check_at) return '—'
  return new Date(status.snapshot.last_check_at).toLocaleTimeString()
})
</script>

<template>
  <div class="bg-elevated border-b border-border">
    <div class="container mx-auto px-4 max-w-6xl flex items-center justify-between py-2 text-xs">
      <div class="flex items-center gap-4">
        <span class="flex items-center gap-1.5">
          <span :class="['inline-block w-2 h-2 rounded-full', dotClass(status.snapshot?.login)]" />
          <span class="text-text-muted">Login</span>
        </span>
        <span class="flex items-center gap-1.5">
          <span :class="['inline-block w-2 h-2 rounded-full', dotClass(status.snapshot?.agent)]" />
          <span class="text-text-muted">Agent</span>
        </span>
        <span class="flex items-center gap-1.5">
          <span :class="['inline-block w-2 h-2 rounded-full', dotClass(status.snapshot?.map)]" />
          <span class="text-text-muted">Map</span>
        </span>
      </div>
      <span class="text-text-muted">最近检测: {{ lastCheck }}</span>
    </div>
  </div>
</template>
