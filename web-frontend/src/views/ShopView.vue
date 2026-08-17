<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { listShop, type ShopItem } from '@/api/shop'

const items = ref<ShopItem[]>([])
const category = ref<string>('')
const loading = ref(false)

async function refresh() {
  loading.value = true
  try {
    const { data } = await listShop(category.value || undefined, 1, 24)
    items.value = data.items
  } catch {
    items.value = []
  } finally {
    loading.value = false
  }
}

onMounted(refresh)
</script>

<template>
  <section>
    <div class="flex items-center justify-between mb-4">
      <h1 class="font-serif text-3xl text-gold">商城 / Shop</h1>
      <select
        v-model="category"
        @change="refresh"
        class="bg-elevated border border-border rounded px-3 py-1.5 text-sm"
      >
        <option value="">全部 / All</option>
        <option value="hair">发型 / Hair</option>
        <option value="weapon">武器 / Weapon</option>
        <option value="armor">防具 / Armor</option>
        <option value="consumable">消耗品 / Consumable</option>
      </select>
    </div>
    <div v-if="loading" class="text-text-muted">加载中…</div>
    <div v-else class="grid sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-4">
      <div
        v-for="it in items"
        :key="it.idx"
        class="bg-elevated border border-border rounded p-4 hover:border-gold transition"
      >
        <img v-if="it.image_url" :src="it.image_url" :alt="it.name" class="w-full h-32 object-cover bg-surface rounded mb-2" />
        <h3 class="text-gold-bright text-sm">{{ it.name }}</h3>
        <p class="text-text-muted text-xs mt-1 line-clamp-2">{{ it.description }}</p>
        <p class="text-vermillion text-sm mt-2">{{ it.price_points }} 点券</p>
      </div>
      <div v-if="!items.length" class="text-text-muted col-span-full">暂无商品 / No items.</div>
    </div>
  </section>
</template>
