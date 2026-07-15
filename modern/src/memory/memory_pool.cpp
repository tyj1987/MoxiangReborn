// memory_pool.cpp - Memory pool implementation.
//
// This file implements the memory pool for Moxian-Reborn.
// Provides buffer pool and memory monitoring functionality.

#include "mxh/memory/memory_pool.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <new>

namespace mxh::memory {

// ============================================================================
// BufferPool implementation
// ============================================================================

BufferPool::BufferPool(std::size_t buffer_size, std::size_t initial_count)
    : buffer_size_(buffer_size) {
    if (initial_count > 0) {
        reserve(initial_count);
    }
}

BufferPool::~BufferPool() {
    // Free all allocated buffers
    for (auto* buffer : allocated_buffers_) {
        deallocate_buffer(buffer);
    }
    
    for (auto* buffer : free_buffers_) {
        deallocate_buffer(buffer);
    }
}

BufferPool::Buffer BufferPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Buffer buffer;
    
    if (!free_buffers_.empty()) {
        // Reuse buffer from free list
        buffer.data = free_buffers_.back();
        free_buffers_.pop_back();
    } else {
        // Allocate new buffer
        buffer.data = allocate_buffer();
        if (buffer.data == nullptr) {
            return buffer; // Invalid buffer
        }
    }
    
    buffer.size = 0;
    buffer.capacity = buffer_size_;
    allocated_buffers_.push_back(buffer.data);
    allocated_++;
    
    // Record allocation
    MemoryMonitor::get_instance().record_allocation(buffer_size_);
    
    return buffer;
}

void BufferPool::deallocate(Buffer& buffer) {
    if (!buffer.is_valid()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove from allocated list
    auto it = std::find(allocated_buffers_.begin(), allocated_buffers_.end(), buffer.data);
    if (it != allocated_buffers_.end()) {
        allocated_buffers_.erase(it);
    }
    
    // Add to free list
    free_buffers_.push_back(buffer.data);
    allocated_--;
    
    // Record deallocation
    MemoryMonitor::get_instance().record_deallocation(buffer_size_);
    
    // Reset buffer
    buffer.data = nullptr;
    buffer.size = 0;
    buffer.capacity = 0;
}

void BufferPool::reserve(std::size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (std::size_t i = 0; i < count; ++i) {
        std::uint8_t* buffer = allocate_buffer();
        if (buffer != nullptr) {
            free_buffers_.push_back(buffer);
            capacity_++;
        }
    }
}

std::uint8_t* BufferPool::allocate_buffer() {
    // Allocate aligned buffer for SIMD operations
    std::uint8_t* buffer = static_cast<std::uint8_t*>(
        aligned_alloc(kCacheLineSize, buffer_size_)
    );
    
    if (buffer != nullptr) {
        // Zero-initialize buffer
        std::memset(buffer, 0, buffer_size_);
    }
    
    return buffer;
}

void BufferPool::deallocate_buffer(std::uint8_t* buffer) {
    if (buffer != nullptr) {
        aligned_free(buffer);
    }
}

// ============================================================================
// MemoryMonitor implementation
// ============================================================================

MemoryMonitor& MemoryMonitor::get_instance() {
    static MemoryMonitor instance;
    return instance;
}

void MemoryMonitor::record_allocation(std::size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats_.total_allocated += size;
    stats_.current_usage += size;
    stats_.allocation_count++;
    
    if (stats_.current_usage > stats_.peak_usage) {
        stats_.peak_usage = stats_.current_usage;
    }
    
    // Check for memory warning
    if (warning_callback_ && stats_.current_usage > 1024 * 1024 * 100) { // 100MB
        warning_callback_(stats_.current_usage);
    }
}

void MemoryMonitor::record_deallocation(std::size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats_.total_deallocated += size;
    stats_.current_usage -= size;
    stats_.deallocation_count++;
}

MemoryStats MemoryMonitor::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void MemoryMonitor::reset_stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.reset();
}

void MemoryMonitor::set_warning_callback(std::function<void(std::size_t)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    warning_callback_ = std::move(callback);
}

// ============================================================================
// Utility functions
// ============================================================================

// Thread-local pool instances
static thread_local ObjectPool<std::uint8_t>* tls_buffer_pool = nullptr;

// Get thread-local buffer pool
ObjectPool<std::uint8_t>& get_thread_local_buffer_pool() {
    if (tls_buffer_pool == nullptr) {
        static ObjectPool<std::uint8_t> global_pool(1000);
        tls_buffer_pool = &global_pool;
    }
    return *tls_buffer_pool;
}

// Allocate from thread-local pool
void* thread_local_alloc(std::size_t size) {
    auto& pool = get_thread_local_buffer_pool();
    return pool.allocate(size);
}

// Deallocate to thread-local pool
void thread_local_free(void* ptr, std::size_t size) {
    if (ptr == nullptr) return;
    
    auto& pool = get_thread_local_buffer_pool();
    pool.deallocate(static_cast<std::uint8_t*>(ptr));
}

// ============================================================================
// Memory statistics reporting
// ============================================================================

void print_memory_stats() {
    auto stats = MemoryMonitor::get_instance().get_stats();
    
    std::cout << "=== Memory Statistics ===" << std::endl;
    std::cout << "Total allocated: " << stats.total_allocated / 1024 << " KB" << std::endl;
    std::cout << "Total deallocated: " << stats.total_deallocated / 1024 << " KB" << std::endl;
    std::cout << "Current usage: " << stats.current_usage / 1024 << " KB" << std::endl;
    std::cout << "Peak usage: " << stats.peak_usage / 1024 << " KB" << std::endl;
    std::cout << "Allocation count: " << stats.allocation_count << std::endl;
    std::cout << "Deallocation count: " << stats.deallocation_count << std::endl;
    std::cout << "Active allocations: " << stats.allocation_count - stats.deallocation_count << std::endl;
}

// ============================================================================
// Memory leak detection
// ============================================================================

#ifdef MXH_MEMORY_LEAK_DETECTION

class MemoryLeakDetector {
public:
    static MemoryLeakDetector& get_instance() {
        static MemoryLeakDetector instance;
        return instance;
    }
    
    void record_allocation(void* ptr, std::size_t size, const char* file, int line) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = {size, file, line};
    }
    
    void record_deallocation(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.erase(ptr);
    }
    
    void report_leaks() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (allocations_.empty()) {
            std::cout << "No memory leaks detected." << std::endl;
            return;
        }
        
        std::cout << "=== Memory Leaks Detected ===" << std::endl;
        for (const auto& [ptr, info] : allocations_) {
            std::cout << "  " << ptr << " - " << info.size << " bytes"
                      << " at " << info.file << ":" << info.line << std::endl;
        }
    }
    
private:
    struct AllocationInfo {
        std::size_t size;
        const char* file;
        int line;
    };
    
    std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
};

// Override global new/delete for leak detection
void* operator new(std::size_t size) {
    void* ptr = malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    
    MemoryLeakDetector::get_instance().record_allocation(ptr, size, "unknown", 0);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    MemoryLeakDetector::get_instance().record_deallocation(ptr);
    free(ptr);
}

void* operator new[](std::size_t size) {
    void* ptr = malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    
    MemoryLeakDetector::get_instance().record_allocation(ptr, size, "unknown", 0);
    return ptr;
}

void operator delete[](void* ptr) noexcept {
    MemoryLeakDetector::get_instance().record_deallocation(ptr);
    free(ptr);
}

#endif // MXH_MEMORY_LEAK_DETECTION

} // namespace mxh::memory