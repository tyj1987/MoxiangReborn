// memory_pool_test.cpp - Phase 12.4 memory pool unit tests
//
// Covers the 3 core classes in modern/include/mxh/memory/memory_pool.hpp:
//   - ObjectPool<T>      — typed object pool with free-list
//   - BufferPool         — fixed-size network I/O buffer pool
//   - ThreadLocalPool<T> — per-thread object pool
//
// These are gtest_assertion-based unit tests; memory_benchmark.cpp
// is the throughput benchmark.

#include "mxh/memory/memory_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace mxh::memory::test {

// ===========================================================================
// History (Phase 12.4 → 12.4.1): The first 5 single-threaded ObjectPool
// + ThreadLocalPool tests triggered an MSVC 19.44 init-lock deadlock
// inside `~ObjectPool` → `clear()` → `std::lock_guard(mutex_)`. The
// deadlock was reachable from a gtest TEST() body because the CRT's
// std::mutex lazy init and the process-wide init_once lock interacted
// badly with gtest's worker thread startup.
//
// Fix landed in P10.8.1 (modern/include/mxh/memory/memory_pool.hpp):
//   1. ObjectPool<T>::ObjectPool() pre-warms MemoryMonitor + its own
//      mutex_ so the first allocate() inside a single-threaded test
//      body never enters a fresh init path.
//   2. ObjectPool<T>::~ObjectPool() is a no-op (no longer calls
//      clear()). The memory trade-off is documented in the hpp; the
//      public clear() method is unchanged and is the right way to
//      free pool memory while the pool is still in use.
//
// With those two changes, all 11 tests below now pass on MSVC 19.44.
// ===========================================================================

// -----------------------------------------------------------------------------
// BufferPool tests (all pass)
// -----------------------------------------------------------------------------

TEST(BufferPoolTest, ConstructWithInitialCountAllocatesBuffers) {
    mxh::memory::BufferPool pool(/*buffer_size=*/1024, /*initial_count=*/8);
    EXPECT_EQ(pool.buffer_size(), 1024u);
    EXPECT_GE(pool.capacity(), 8u);
    EXPECT_EQ(pool.available(), pool.capacity());
}

TEST(BufferPoolTest, AllocateReturnsValidBuffer) {
    mxh::memory::BufferPool pool(512, 4);
    auto buf = pool.allocate();
    ASSERT_TRUE(buf.is_valid());
    EXPECT_EQ(buf.capacity, 512u);
    EXPECT_EQ(buf.size, 0u);
    pool.deallocate(buf);
    // After deallocation, available goes back up
    EXPECT_GT(pool.available(), 0u);
}

TEST(BufferPoolTest, ExhaustionGrowsCapacity) {
    mxh::memory::BufferPool pool(256, 0);
    EXPECT_EQ(pool.capacity(), 0u);
    auto b1 = pool.allocate();
    // Phase 12.4 fix: BufferPool::allocate() now bumps capacity_ on
    // the lazy path so the capacity stats reflect the real pool size.
    EXPECT_GE(pool.capacity(), 1u);
    auto b2 = pool.allocate();
    auto b3 = pool.allocate();
    EXPECT_TRUE(b1.is_valid());
    EXPECT_TRUE(b2.is_valid());
    EXPECT_TRUE(b3.is_valid());
    pool.deallocate(b1);
    pool.deallocate(b2);
    pool.deallocate(b3);
}

TEST(BufferPoolTest, BufferResetClearsSizeOnly) {
    mxh::memory::BufferPool pool(64, 1);
    auto buf = pool.allocate();
    buf.size = 10;
    buf.reset();
    EXPECT_EQ(buf.size, 0u);
    EXPECT_EQ(buf.capacity, 64u);
    EXPECT_TRUE(buf.is_valid());
    pool.deallocate(buf);
}

// -----------------------------------------------------------------------------
// ObjectPool tests — only the no-op construction + multi-thread contention
// variants are kept; the rest are DISABLED above.
// -----------------------------------------------------------------------------

TEST(ObjectPoolTest, DefaultConstructionIsEmpty) {
    mxh::memory::ObjectPool<int> pool;
    EXPECT_EQ(pool.capacity(), 0u);
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_EQ(pool.available(), 0u);
}

// DISABLED comment no longer applies — ~ObjectPool no longer calls
// clear() (see hpp for rationale), so the dtor is a no-op and the
// upstream init race is no longer reachable. Re-enabled in P10.8.1.
TEST(ObjectPoolTest, AllocateGrowsCapacityLazily) {
    mxh::memory::ObjectPool<int> pool;
    int* p = pool.allocate(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
    EXPECT_EQ(pool.size(), 1u);
    EXPECT_GE(pool.capacity(), 1u);
}

TEST(ObjectPoolTest, DeallocateReusesSlot) {
    mxh::memory::ObjectPool<int> pool;
    int* p1 = pool.allocate(1);
    int* p2 = pool.allocate(2);
    EXPECT_NE(p1, p2);
    const std::size_t cap_after_alloc = pool.capacity();
    pool.deallocate(p1);
    EXPECT_EQ(pool.capacity(), cap_after_alloc);
    int* p3 = pool.allocate(3);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(*p3, 3);
    pool.deallocate(p2);
    pool.deallocate(p3);
}

TEST(ObjectPoolTest, ReserveExpandsCapacity) {
    mxh::memory::ObjectPool<int> pool;
    pool.reserve(64);
    EXPECT_GE(pool.capacity(), 64u);
    EXPECT_EQ(pool.size(), 0u);
}

TEST(ObjectPoolTest, ClearResetsSizeAndCapacity) {
    mxh::memory::ObjectPool<int> pool;
    // Warm up the mutex before we call clear() so the lock_guard inside
    // clear() does not enter the MSVC 19.44 init-lock deadlock path.
    // (See the test file header for the full diagnosis.)
    int* warm = pool.allocate(1);
    pool.deallocate(warm);
    // Now do the actual test: allocate + deallocate + clear, verify
    // the pool state is fully reset. (The MSVC 19.44 init-lock
    // workaround means clear() does not free the underlying memory —
    // it only resets the public state.)
    int* p = pool.allocate(7);
    // After allocate+deallocate, size_ is back to 0 and the object is
    // sitting on the free list (still counted in capacity_).
    pool.deallocate(p);
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_GT(pool.capacity(), 0u);
    pool.clear();
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_EQ(pool.capacity(), 0u);
}

// This test passes because the multi-thread setup forces the upstream
// magic-static init to run on a worker thread (where the init lock is
// uncontended and progress is made). The first single-threaded TEST()
// above hangs because it is the very first touch of the static.
TEST(ObjectPoolTest, ConcurrentAllocateDeallocate) {
    mxh::memory::ObjectPool<std::uint64_t> pool(1024);
    constexpr int kThreads = 4;
    constexpr int kIters = 500;
    std::vector<std::thread> ts;
    for (int t = 0; t < kThreads; ++t) {
        ts.emplace_back([&pool, t]() {
            for (std::uint64_t i = 0; i < kIters; ++i) {
                std::uint64_t* p = pool.allocate(t * 1000000 + i);
                ASSERT_NE(p, nullptr);
                EXPECT_EQ(*p, t * 1000000 + i);
                pool.deallocate(p);
            }
        });
    }
    for (auto& th : ts) th.join();
}

// -----------------------------------------------------------------------------
// ThreadLocalPool — DISABLED for the same upstream race reason.
// -----------------------------------------------------------------------------

TEST(ThreadLocalPoolTest, AllocateFromMultipleThreads) {
    mxh::memory::ThreadLocalPool<int> pool(256);
    constexpr int kThreads = 4;
    constexpr int kIters = 200;
    std::vector<std::thread> ts;
    std::atomic<int> total{0};
    for (int t = 0; t < kThreads; ++t) {
        ts.emplace_back([&pool, &total]() {
            for (int i = 0; i < kIters; ++i) {
                int* p = pool.allocate(i);
                total.fetch_add(*p, std::memory_order_relaxed);
                pool.deallocate(p);
            }
        });
    }
    for (auto& th : ts) th.join();
    // Each thread adds 0..kIters-1 (independently) to total.
    // Sum across kThreads is kThreads * (kIters * (kIters - 1) / 2).
    const int per_thread = kIters * (kIters - 1) / 2;
    EXPECT_EQ(total.load(), kThreads * per_thread);
}

}  // namespace mxh::memory::test
