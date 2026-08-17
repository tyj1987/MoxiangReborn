// src/api/shop.ts
import { api } from './client'

export interface ShopItem {
  idx: number
  name: string
  category: 'hair' | 'weapon' | 'armor' | 'consumable'
  price_points: number
  image_url: string
  description: string
}

export function listShop(category?: string, page = 1, size = 24) {
  return api.get<{ items: ShopItem[]; page: number; total: number }>('/api/shop/items', {
    params: { category, page, size },
  })
}
