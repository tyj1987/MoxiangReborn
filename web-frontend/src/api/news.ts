// src/api/news.ts
import { api } from './client'

export interface NewsItem {
  id: string
  slug: string
  title: string
  summary: string
  hero_image: string
  published_at: string
}
export interface NewsDetail extends NewsItem {
  body: string
}

export function listNews(page = 1, size = 10) {
  return api.get<{ items: NewsItem[]; page: number; total: number }>('/api/news', {
    params: { page, size },
  })
}

export function getNews(slug: string) {
  return api.get<NewsDetail>(`/api/news/${encodeURIComponent(slug)}`)
}
