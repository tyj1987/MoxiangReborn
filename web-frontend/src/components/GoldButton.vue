<script setup lang="ts">
defineProps<{
  variant?: 'primary' | 'secondary' | 'danger'
  type?: 'button' | 'submit'
  disabled?: boolean
}>()
</script>

<template>
  <button
    :type="type ?? 'button'"
    :disabled="disabled"
    :class="[
      'px-5 py-2.5 rounded font-medium tracking-wide transition',
      'border-2 focus:outline-none focus:ring-2 focus:ring-gold',
      'disabled:opacity-50 disabled:cursor-not-allowed',
      variant === 'secondary'
        ? 'border-gold-dim text-gold hover:bg-gold-dim hover:text-dark'
        : variant === 'danger'
        ? 'border-vermillion text-vermillion hover:bg-vermillion hover:text-text-primary'
        : 'border-gold text-gold hover:bg-gold hover:text-dark animate-pulse-gold',
    ]"
  >
    <slot />
  </button>
</template>
