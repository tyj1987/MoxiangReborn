// util_test.cpp — Phase 8.1-8.3: ThreadPool, ObjectPool, Compression tests.

#include "mxh/util/thread_pool.hpp"
#include "mxh/util/object_pool.hpp"
#include "mxh/util/compress.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

// ============================================================================
// ThreadPool tests (Phase 8.1)
// ============================================================================

TEST(ThreadPoolTest, ExecutesTasks) {
    mxh::util::ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.enqueue([&] { counter.fetch_add(1); });
    }

    // Wait for all tasks.
    while (pool.pending_tasks() > 0 || pool.active_tasks() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, EnqueueWithFuture) {
    mxh::util::ThreadPool pool(2);

    auto f1 = pool.enqueue_with_future([] { return 42; });
    auto f2 = pool.enqueue_with_future([] { return std::string("hello"); });

    EXPECT_EQ(f1.get(), 42);
    EXPECT_EQ(f2.get(), "hello");
}

TEST(ThreadPoolTest, ThreadCount) {
    mxh::util::ThreadPool pool(8);
    EXPECT_EQ(pool.thread_count(), 8u);
}

TEST(ThreadPoolTest, ConcurrentAccess) {
    mxh::util::ThreadPool pool(4);
    constexpr int N = 1000;
    std::vector<int> results(N, 0);

    std::vector<std::future<int>> futures;
    for (int i = 0; i < N; ++i) {
        futures.push_back(pool.enqueue_with_future([i] { return i * 2; }));
    }

    for (int i = 0; i < N; ++i) {
        results[i] = futures[i].get();
    }

    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(results[i], i * 2);
    }
}

TEST(ThreadPoolTest, ShutdownIsIdempotent) {
    mxh::util::ThreadPool pool(2);
    pool.shutdown();
    pool.shutdown();  // should not crash
}

TEST(ThreadPoolTest, DefaultThreadCount) {
    mxh::util::ThreadPool pool;  // default = hardware_concurrency
    EXPECT_GT(pool.thread_count(), 0u);
}

// ============================================================================
// ObjectPool tests (Phase 8.2)
// ============================================================================

TEST(ObjectPoolTest, AcquireReturnsObject) {
    mxh::util::ObjectPool<std::vector<int>> pool(4);
    auto obj = pool.acquire();
    ASSERT_NE(obj, nullptr);
    obj->resize(10, 42);
    EXPECT_EQ(obj->size(), 10u);
}

TEST(ObjectPoolTest, ReleaseReturnsToPool) {
    mxh::util::ObjectPool<int> pool(4);
    EXPECT_EQ(pool.available(), 4u);
    {
        auto obj = pool.acquire();
        EXPECT_EQ(pool.available(), 3u);
        *obj = 42;
    }
    EXPECT_EQ(pool.available(), 4u);
}

TEST(ObjectPoolTest, GrowsOnDemand) {
    mxh::util::ObjectPool<int> pool(2);
    auto a = pool.acquire();
    auto b = pool.acquire();
    auto c = pool.acquire();  // exceeds initial pool size
    EXPECT_NE(a, nullptr);
    EXPECT_NE(b, nullptr);
    EXPECT_NE(c, nullptr);
    EXPECT_GE(pool.high_watermark(), 1u);  // at least 1 extra allocation
}

TEST(ObjectPoolTest, ResetOnRelease) {
    mxh::util::ObjectPool<std::vector<int>> pool(1);
    {
        auto obj = pool.acquire();
        obj->resize(100, 99);
        EXPECT_EQ(obj->size(), 100u);
    }
    auto obj2 = pool.acquire();
    EXPECT_EQ(obj2->size(), 0u);  // should be reset to default
}

TEST(ObjectPoolTest, ManyAcquireReleaseCycles) {
    mxh::util::ObjectPool<int> pool(8);
    for (int cycle = 0; cycle < 100; ++cycle) {
        std::vector<decltype(pool)::Ptr> objs;
        for (int i = 0; i < 16; ++i) {
            objs.push_back(pool.acquire());
            *objs.back() = i;
        }
        // All return to pool when objs goes out of scope.
    }
    // Pool should have at least its initial size available.
    EXPECT_GE(pool.available(), 8u);
}

// ============================================================================
// Compression tests (Phase 8.3)
// ============================================================================

TEST(CompressionTest, RLERoundTrip) {
    // Highly repetitive data (good for RLE).
    std::vector<std::uint8_t> input(1000, 0xAB);
    auto compressed = mxh::util::rle_compress(input);
    auto decompressed = mxh::util::rle_decompress(compressed);
    EXPECT_EQ(decompressed, input);
    EXPECT_LT(compressed.size(), input.size());
}

TEST(CompressionTest, RLEMixedData) {
    std::vector<std::uint8_t> input;
    for (int i = 0; i < 100; ++i) input.push_back(0x11);
    for (int i = 0; i < 50; ++i) input.push_back(static_cast<std::uint8_t>(i));
    for (int i = 0; i < 200; ++i) input.push_back(0x00);

    auto compressed = mxh::util::rle_compress(input);
    auto decompressed = mxh::util::rle_decompress(compressed);
    EXPECT_EQ(decompressed, input);
}

TEST(CompressionTest, RLEEmptyInput) {
    std::vector<std::uint8_t> empty;
    auto compressed = mxh::util::rle_compress(empty);
    EXPECT_TRUE(compressed.empty());
    auto decompressed = mxh::util::rle_decompress(compressed);
    EXPECT_TRUE(decompressed.empty());
}

TEST(CompressionTest, PayloadSmallNotCompressed) {
    std::vector<std::uint8_t> payload(64, 0xFF);
    auto result = mxh::util::compress_payload(payload);

    // Small payloads should have original_size = 0 (not compressed).
    EXPECT_FALSE(mxh::util::is_compressed(result));

    auto decompressed = mxh::util::decompress_payload(result);
    EXPECT_EQ(decompressed, payload);
}

TEST(CompressionTest, PayloadLargeRepetitiveCompressed) {
    std::vector<std::uint8_t> payload(2048, 0x42);
    auto result = mxh::util::compress_payload(payload);

    EXPECT_TRUE(mxh::util::is_compressed(result));
    EXPECT_LT(result.size(), payload.size() + 4);  // +4 for header

    auto decompressed = mxh::util::decompress_payload(result);
    EXPECT_EQ(decompressed, payload);
}

TEST(CompressionTest, PayloadIncompressibleSentRaw) {
    // Random-ish data that RLE can't compress.
    std::vector<std::uint8_t> payload;
    for (std::size_t i = 0; i < 256; ++i) {
        payload.push_back(static_cast<std::uint8_t>(i));
    }
    // Repeat to exceed threshold.
    for (std::size_t i = 0; i < 256; ++i) {
        payload.push_back(static_cast<std::uint8_t>(i));
    }

    auto result = mxh::util::compress_payload(payload);
    // Should detect no compression gain and send raw.
    EXPECT_FALSE(mxh::util::is_compressed(result));

    auto decompressed = mxh::util::decompress_payload(result);
    EXPECT_EQ(decompressed, payload);
}

TEST(CompressionTest, ThresholdConstant) {
    EXPECT_EQ(mxh::util::kCompressionThreshold, 128u);
}

TEST(CompressionTest, CompressedHeaderSize) {
    EXPECT_EQ(mxh::util::kCompressedHeaderSize, 4u);
}
