<script setup lang="ts">
// Decorative gold-curl divider used between hero sections.
</script>

<template>
  <div class="flex justify-center my-4">
    <svg width="200" height="20" viewBox="0 0 200 20" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="gold" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop offset="0%"   stop-color="#8a6d3a" />
          <stop offset="50%"  stop-color="#e8c984" />
          <stop offset="100%" stop-color="#8a6d3a" />
        </linearGradient>
      </defs>
      <path d="M0 10 Q50 0 100 10 T200 10" stroke="url(#gold)" stroke-width="1.5" fill="none" />
      <circle cx="100" cy="10" r="3" fill="#c9a76a" />
    </svg>
  </div>
</template>
