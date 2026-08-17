<script setup lang="ts">
// Root layout — header + main router outlet + footer.
import { onMounted } from 'vue'
import SiteHeader from '@/components/SiteHeader.vue'
import SiteFooter from '@/components/SiteFooter.vue'
import ServerStatusBar from '@/components/ServerStatusBar.vue'
import { useStatusStore } from '@/stores/status'

const status = useStatusStore()
onMounted(() => status.refresh())
</script>

<template>
  <ServerStatusBar />
  <SiteHeader />
  <main class="flex-1 container mx-auto px-4 py-6 max-w-6xl">
    <router-view v-slot="{ Component }">
      <transition name="fade" mode="out-in">
        <component :is="Component" />
      </transition>
    </router-view>
  </main>
  <SiteFooter />
</template>

<style scoped>
.fade-enter-active,
.fade-leave-active {
  transition: opacity 200ms ease-out;
}
.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
