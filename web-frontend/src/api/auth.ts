// src/api/auth.ts
import { api } from './client'

export interface AuthMe {
  account: string
  user_idx: number
  points: number
  registerdate: string
  lastlogindate: string
  lastloginip: string
}

export function register(account: string, password: string, confirm: string) {
  return api.post('/api/auth/register', { account, password, confirm })
}

export function login(account: string, password: string) {
  return api.post<{ token: string; user_idx: number; account: string; points: number }>(
    '/api/auth/login',
    { account, password },
  )
}

export function me() {
  return api.get<AuthMe>('/api/auth/me')
}

export function logout() {
  return api.post('/api/auth/logout')
}
