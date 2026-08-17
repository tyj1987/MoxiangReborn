<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { getNews, type NewsDetail } from '@/api/news'

const props = defineProps<{ slug: string }>()
const item = ref<NewsDetail | null>(null)
const error = ref('')

onMounted(async () => {
  try {
    const { data } = await getNews(props.slug)
    item.value = data
  } catch (e: any) {
    error.value = e?.response?.status === 404 ? '未找到 / Not found' : '加载失败'
  }
})
</script>

<template>
  <article>
    <div v-if="error" class="text-vermillion">{{ error }}</div>
    <div v-else-if="!item" class="text-text-muted">加载中…</div>
    <div v-else>
      <h1 class="font-serif text-3xl text-gold">{{ item.title }}</h1>
      <p class="text-text-muted text-sm mt-2">{{ new Date(item.published_at).toLocaleDateString() }}</p>
      <div class="prose prose-invert mt-6 whitespace-pre-wrap">{{ item.body }}</div>
    </div>
  </article>
</template>
