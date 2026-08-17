// src/api/status.ts
import { api } from './client'

export interface StatusSnapshot {
  login: 'up' | 'down' | 'unknown'
  agent: 'up' | 'down' | 'unknown'
  map: 'up' | 'down' | 'unknown'
  online_count: number | null
  last_check_at: string
  version: string
}

export function getStatus() {
  return api.get<StatusSnapshot>('/api/status')
}
