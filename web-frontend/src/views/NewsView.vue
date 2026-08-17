<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { RouterLink } from 'vue-router'
import { listNews, type NewsItem } from '@/api/news'

const items = ref<NewsItem[]>([])

onMounted(async () => {
  try {
    const { data } = await listNews(1, 20)
    items.value = data.items
  } catch {
    items.value = []
  }
})
</script>

<template>
  <section>
    <h1 class="font-serif text-3xl text-gold mb-4">新闻 / News</h1>
    <ul class="space-y-3">
      <li v-for="n in items" :key="n.id">
        <RouterLink
          :to="{ name: 'news-detail', params: { slug: n.slug } }"
          class="block bg-elevated border border-border rounded p-4 hover:border-gold transition"
        >
          <h3 class="text-gold-bright text-lg">{{ n.title }}</h3>
          <p class="text-text-muted text-sm mt-1">{{ n.summary }}</p>
          <p class="text-text-muted text-xs mt-2">{{ new Date(n.published_at).toLocaleDateString() }}</p>
        </RouterLink>
      </li>
      <li v-if="!items.length" class="text-text-muted">暂无新闻 / No news yet.</li>
    </ul>
  </section>
</template>
