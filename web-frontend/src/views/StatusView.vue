<script setup lang="ts">
import { onMounted } from 'vue'
import { useStatusStore } from '@/stores/status'

const status = useStatusStore()
onMounted(() => status.refresh())
</script>

<template>
  <section>
    <h1 class="font-serif text-3xl text-gold mb-4">服务器状态 / Status</h1>
    <div v-if="status.loading" class="text-text-muted">加载中…</div>
    <div v-else-if="!status.snapshot" class="text-vermillion">无法获取状态 / Status unavailable</div>
    <div v-else class="grid sm:grid-cols-3 gap-4">
      <div class="bg-elevated border border-border rounded p-4">
        <h3 class="text-text-muted text-sm">Login</h3>
        <p :class="status.snapshot.login === 'up' ? 'text-green-500' : 'text-red-500'" class="text-2xl font-serif">
          {{ status.snapshot.login }}
        </p>
      </div>
      <div class="bg-elevated border border-border rounded p-4">
        <h3 class="text-text-muted text-sm">Agent</h3>
        <p :class="status.snapshot.agent === 'up' ? 'text-green-500' : 'text-red-500'" class="text-2xl font-serif">
          {{ status.snapshot.agent }}
        </p>
      </div>
      <div class="bg-elevated border border-border rounded p-4">
        <h3 class="text-text-muted text-sm">Map</h3>
        <p :class="status.snapshot.map === 'up' ? 'text-green-500' : 'text-red-500'" class="text-2xl font-serif">
          {{ status.snapshot.map }}
        </p>
      </div>
      <div class="bg-elevated border border-border rounded p-4 sm:col-span-3">
        <h3 class="text-text-muted text-sm">在线 / Online</h3>
        <p class="text-gold-bright text-2xl font-serif">
          {{ status.snapshot.online_count ?? '—' }}
        </p>
        <p class="text-text-muted text-xs mt-1">最近检测: {{ new Date(status.snapshot.last_check_at).toLocaleString() }}</p>
      </div>
    </div>
  </section>
</template>
