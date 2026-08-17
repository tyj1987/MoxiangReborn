<script setup lang="ts">
import { RouterLink } from 'vue-router'
import GoldButton from '@/components/GoldButton.vue'
</script>

<template>
  <section class="text-center py-16">
    <h1 class="font-serif text-6xl text-gold">404</h1>
    <p class="text-text-secondary mt-2">页面未找到 / Page not found</p>
    <RouterLink to="/" class="inline-block mt-6">
      <GoldButton variant="secondary">返回首页 / Home</GoldButton>
    </RouterLink>
  </section>
</template>
