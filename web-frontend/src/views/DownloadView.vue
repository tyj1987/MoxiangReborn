<script setup lang="ts">
import { ref } from 'vue'
import GoldButton from '@/components/GoldButton.vue'

const version = ref('1.0.0')
const sha256  = ref<string>('')  // populated by /download/checksums.txt in Phase 5
</script>

<template>
  <section class="max-w-2xl mx-auto bg-elevated border border-border rounded p-6">
    <h1 class="font-serif text-3xl text-gold mb-4">下载 / Download</h1>
    <p class="text-text-secondary mb-4">下载墨香现代客户端 v{{ version }}</p>
    <div class="space-y-3">
      <GoldButton @click="window.location.href='/download/client'">
        下载 MoxianClient-{{ version }}.zip
      </GoldButton>
      <p class="text-text-muted text-xs">
        SHA-256: <span class="font-mono">{{ sha256 || '(load via /download/checksums.txt)' }}</span>
      </p>
      <p class="text-text-muted text-xs">
        AutoPatcher manifest: <a href="/download/manifest.json" class="text-gold-bright">manifest.json</a>
      </p>
    </div>
  </section>
</template>
