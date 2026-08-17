<script setup lang="ts">
const year = new Date().getFullYear()
</script>

<template>
  <footer class="border-t border-border bg-elevated mt-12">
    <div class="container mx-auto px-4 max-w-6xl py-6 text-sm text-text-muted flex flex-col sm:flex-row gap-2 justify-between">
      <span>© {{ year }} 墨香 Moxiang Portal. All rights reserved.</span>
      <span>Modern C++ · 1:1 reverse-engineered MMORPG</span>
    </div>
  </footer>
</template>
