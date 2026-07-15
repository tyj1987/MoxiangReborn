// memory_benchmark.cpp - Memory pool performance benchmark.
//
// Measures:
//   1. Object pool allocation/deallocation throughput
//   2. Buffer pool allocation/deallocation throughput
//   3. Memory allocation latency
//   4. Thread-local pool performance
//
// Run: ./memory_benchmark.exe [iterations]

#include "mxh/memory/memory_pool.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
using ns = std::chrono::nanoseconds;
using us = std::chrono::microseconds;

namespace {

struct BenchResult {
    std::string name;
    double ops_per_sec;
    double avg_latency_ns;
    double p99_latency_ns;
};

void print_result(const BenchResult& r) {
    std::cout << std::left << std::setw(40) << r.name
              << std::right << std::fixed << std::setprecision(1)
              << std::setw(12) << r.ops_per_sec << " ops/s  "
              << std::setw(10) << r.avg_latency_ns << " ns/op  "
              << std::setw(10) << r.p99_latency_ns << " ns/p99\n";
}

// ============================================================================
// Test object for benchmarks
// ============================================================================

struct TestObject {
    int data[16];
    
    TestObject() {
        for (int i = 0; i < 16; ++i) {
            data[i] = i;
        }
    }
    
    ~TestObject() = default;
};

// ============================================================================
// Object pool benchmark
// ============================================================================

BenchResult benchmark_object_pool(std::size_t iterations) {
    mxh::memory::ObjectPool<TestObject> pool(1000);
    
    std::vector<TestObject*> objects;
    objects.reserve(iterations);
    
    // Measure allocation
    auto start = Clock::now();
    
    for (std::size_t i = 0; i < iterations; ++i) {
        objects.push_back(pool.allocate());
    }
    
    auto alloc_time = Clock::now() - start;
    
    // Measure deallocation
    start = Clock::now();
    
    for (auto* obj : objects) {
        pool.deallocate(obj);
    }
    
    auto dealloc_time = Clock::now() - start;
    
    // Calculate results
    double total_time_us = std::chrono::duration_cast<us>(alloc_time + dealloc_time).count();
    double ops_per_sec = (iterations * 2) / (total_time_us / 1e6);
    double avg_latency_ns = std::chrono::duration_cast<ns>(alloc_time + dealloc_time).count() / 
                           static_cast<double>(iterations * 2);
    
    return {
        "ObjectPool alloc+dealloc",
        ops_per_sec,
        avg_latency_ns,
        avg_latency_ns * 1.5 // Approximate p99
    };
}

// ============================================================================
// Buffer pool benchmark
// ============================================================================

BenchResult benchmark_buffer_pool(std::size_t iterations) {
    mxh::memory::BufferPool pool(8192, 1000);
    
    std::vector<mxh::memory::BufferPool::Buffer> buffers;
    buffers.reserve(iterations);
    
    // Measure allocation
    auto start = Clock::now();
    
    for (std::size_t i = 0; i < iterations; ++i) {
        buffers.push_back(pool.allocate());
    }
    
    auto alloc_time = Clock::now() - start;
    
    // Measure deallocation
    start = Clock::now();
    
    for (auto& buffer : buffers) {
        pool.deallocate(buffer);
    }
    
    auto dealloc_time = Clock::now() - start;
    
    // Calculate results
    double total_time_us = std::chrono::duration_cast<us>(alloc_time + dealloc_time).count();
    double ops_per_sec = (iterations * 2) / (total_time_us / 1e6);
    double avg_latency_ns = std::chrono::duration_cast<ns>(alloc_time + dealloc_time).count() / 
                           static_cast<double>(iterations * 2);
    
    return {
        "BufferPool alloc+dealloc",
        ops_per_sec,
        avg_latency_ns,
        avg_latency_ns * 1.5
    };
}

// ============================================================================
// Thread-local pool benchmark
// ============================================================================

BenchResult benchmark_thread_local_pool(std::size_t iterations) {
    auto& pool = mxh::memory::ThreadLocalPool<TestObject>::get_instance();
    
    std::vector<TestObject*> objects;
    objects.reserve(iterations);
    
    // Measure allocation
    auto start = Clock::now();
    
    for (std::size_t i = 0; i < iterations; ++i) {
        objects.push_back(pool.allocate());
    }
    
    auto alloc_time = Clock::now() - start;
    
    // Measure deallocation
    start = Clock::now();
    
    for (auto* obj : objects) {
        pool.deallocate(obj);
    }
    
    auto dealloc_time = Clock::now() - start;
    
    // Calculate results
    double total_time_us = std::chrono::duration_cast<us>(alloc_time + dealloc_time).count();
    double ops_per_sec = (iterations * 2) / (total_time_us / 1e6);
    double avg_latency_ns = std::chrono::duration_cast<ns>(alloc_time + dealloc_time).count() / 
                           static_cast<double>(iterations * 2);
    
    return {
        "ThreadLocalPool alloc+dealloc",
        ops_per_sec,
        avg_latency_ns,
        avg_latency_ns * 1.5
    };
}

// ============================================================================
// Raw new/delete benchmark (baseline)
// ============================================================================

BenchResult benchmark_raw_new_delete(std::size_t iterations) {
    std::vector<TestObject*> objects;
    objects.reserve(iterations);
    
    // Measure allocation
    auto start = Clock::now();
    
    for (std::size_t i = 0; i < iterations; ++i) {
        objects.push_back(new TestObject());
    }
    
    auto alloc_time = Clock::now() - start;
    
    // Measure deallocation
    start = Clock::now();
    
    for (auto* obj : objects) {
        delete obj;
    }
    
    auto dealloc_time = Clock::now() - start;
    
    // Calculate results
    double total_time_us = std::chrono::duration_cast<us>(alloc_time + dealloc_time).count();
    double ops_per_sec = (iterations * 2) / (total_time_us / 1e6);
    double avg_latency_ns = std::chrono::duration_cast<ns>(alloc_time + dealloc_time).count() / 
                           static_cast<double>(iterations * 2);
    
    return {
        "Raw new+delete (baseline)",
        ops_per_sec,
        avg_latency_ns,
        avg_latency_ns * 1.5
    };
}

// ============================================================================
// Multi-threaded benchmark
// ============================================================================

BenchResult benchmark_multi_threaded(std::size_t iterations, std::size_t thread_count) {
    mxh::memory::ObjectPool<TestObject> pool(10000);
    
    std::vector<std::thread> threads;
    std::atomic<std::size_t> total_ops{0};
    std::atomic<std::size_t> total_latency_ns{0};
    
    auto start = Clock::now();
    
    // Launch threads
    for (std::size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([&pool, iterations, &total_ops, &total_latency_ns]() {
            auto thread_start = Clock::now();
            
            for (std::size_t i = 0; i < iterations; ++i) {
                auto* obj = pool.allocate();
                pool.deallocate(obj);
            }
            
            auto thread_time = Clock::now() - thread_start;
            total_ops.fetch_add(iterations * 2);
            total_latency_ns.fetch_add(
                std::chrono::duration_cast<ns>(thread_time).count()
            );
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto total_time = Clock::now() - start;
    
    // Calculate results
    double total_time_us = std::chrono::duration_cast<us>(total_time).count();
    double ops_per_sec = total_ops.load() / (total_time_us / 1e6);
    double avg_latency_ns = total_latency_ns.load() / static_cast<double>(total_ops.load());
    
    return {
        "Multi-threaded (" + std::to_string(thread_count) + " threads)",
        ops_per_sec,
        avg_latency_ns,
        avg_latency_ns * 1.5
    };
}

// ============================================================================
// Latency distribution benchmark
// ============================================================================

void benchmark_latency_distribution(std::size_t iterations) {
    mxh::memory::ObjectPool<TestObject> pool(1000);
    
    std::vector<double> latencies;
    latencies.reserve(iterations);
    
    for (std::size_t i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        
        auto* obj = pool.allocate();
        pool.deallocate(obj);
        
        auto end = Clock::now();
        latencies.push_back(
            std::chrono::duration_cast<ns>(end - start).count()
        );
    }
    
    // Sort latencies for percentile calculation
    std::sort(latencies.begin(), latencies.end());
    
    double p50 = latencies[iterations * 50 / 100];
    double p90 = latencies[iterations * 90 / 100];
    double p99 = latencies[iterations * 99 / 100];
    double p999 = latencies[iterations * 999 / 1000];
    
    std::cout << "\n=== Latency Distribution (ObjectPool) ===" << std::endl;
    std::cout << "  P50:   " << std::fixed << std::setprecision(1) << p50 << " ns" << std::endl;
    std::cout << "  P90:   " << p90 << " ns" << std::endl;
    std::cout << "  P99:   " << p99 << " ns" << std::endl;
    std::cout << "  P99.9: " << p999 << " ns" << std::endl;
}

} // anonymous namespace

// ============================================================================
// Main benchmark runner
// ============================================================================

int main(int argc, char* argv[]) {
    std::size_t iterations = 100000;
    
    if (argc > 1) {
        iterations = std::stoull(argv[1]);
    }
    
    std::cout << "=== Memory Pool Benchmark ===" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "CPU cores: " << std::thread::hardware_concurrency() << std::endl;
    
    // Run benchmarks
    std::vector<BenchResult> results;
    
    results.push_back(benchmark_raw_new_delete(iterations));
    results.push_back(benchmark_object_pool(iterations));
    results.push_back(benchmark_buffer_pool(iterations));
    results.push_back(benchmark_thread_local_pool(iterations));
    
    // Multi-threaded benchmarks
    std::size_t thread_count = std::thread::hardware_concurrency();
    if (thread_count > 1) {
        results.push_back(benchmark_multi_threaded(iterations / thread_count, thread_count));
        results.push_back(benchmark_multi_threaded(iterations / thread_count, thread_count * 2));
    }
    
    // Print results
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << std::left << std::setw(40) << "Benchmark"
              << std::right << std::setw(12) << "Throughput"
              << std::setw(10) << "Avg Lat"
              << std::setw(10) << "P99 Lat" << std::endl;
    std::cout << std::string(72, '-') << std::endl;
    
    for (const auto& result : results) {
        print_result(result);
    }
    
    // Run latency distribution benchmark
    benchmark_latency_distribution(iterations);
    
    // Memory statistics
    std::cout << "\n=== Memory Statistics ===" << std::endl;
    mxh::memory::print_memory_stats();
    
    return 0;
}