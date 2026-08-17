<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { RouterLink } from 'vue-router'
import OrnateDivider from '@/components/OrnateDivider.vue'
import GoldButton from '@/components/GoldButton.vue'
import { listNews, type NewsItem } from '@/api/news'

const featured = ref<NewsItem[]>([])

onMounted(async () => {
  try {
    const { data } = await listNews(1, 3)
    featured.value = data.items
  } catch {
    featured.value = []
  }
})
</script>

<template>
  <section class="space-y-8">
    <div class="text-center space-y-4 py-12">
      <h1 class="font-serif text-5xl sm:text-6xl text-gold tracking-wider">墨香 Moxiang</h1>
      <p class="text-text-secondary text-lg">经典 2D MMORPG · 1:1 现代复刻</p>
      <OrnateDivider />
      <div class="flex justify-center gap-4 pt-2">
        <RouterLink :to="{ name: 'register' }">
          <GoldButton>注册账号 / Register</GoldButton>
        </RouterLink>
        <RouterLink :to="{ name: 'download' }">
          <GoldButton variant="secondary">下载客户端 / Download</GoldButton>
        </RouterLink>
      </div>
    </div>

    <div v-if="featured.length" class="grid md:grid-cols-3 gap-4">
      <RouterLink
        v-for="item in featured"
        :key="item.id"
        :to="{ name: 'news-detail', params: { slug: item.slug } }"
        class="block bg-elevated border border-border rounded p-4 hover:border-gold transition"
      >
        <h3 class="font-serif text-lg text-gold-bright mb-2">{{ item.title }}</h3>
        <p class="text-text-muted text-sm line-clamp-3">{{ item.summary }}</p>
      </RouterLink>
    </div>
  </section>
</template>
