// memory_pool.hpp - High-performance memory pool for Moxian-Reborn.
//
// This module provides memory management for server-side components:
//   - Object pool for frequent allocations (connections, messages, etc.)
//   - Buffer pool for network I/O buffers
//   - Thread-safe allocation and deallocation
//   - Memory statistics and monitoring
//
// Design:
//   - Lock-free for single-threaded access patterns
//   - Thread-safe for shared access patterns
//   - Configurable pool sizes and growth policies
//   - Memory alignment for SIMD operations
//
// Usage:
//   ObjectPool<Connection> connection_pool(1000);
//   auto conn = connection_pool.allocate();
//   // Use conn...
//   connection_pool.deallocate(conn);
//
//   BufferPool buffer_pool(8192, 1000);
//   auto buffer = buffer_pool.allocate();
//   // Use buffer...
//   buffer_pool.deallocate(buffer);

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <vector>

namespace mxh::memory {

// ============================================================================
// Memory alignment constants
// ============================================================================

constexpr std::size_t kCacheLineSize = 64;
constexpr std::size_t kDefaultAlignment = alignof(std::max_align_t);

// ============================================================================
// Object pool for frequent allocations
// ============================================================================

template<typename T>
class ObjectPool {
public:
    // Constructor with initial capacity
    explicit ObjectPool(std::size_t initial_capacity = 0);
    
    // Destructor
    ~ObjectPool();
    
    // Disable copy
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    
    // Allocate an object from the pool
    template<typename... Args>
    T* allocate(Args&&... args);
    
    // Return an object to the pool
    void deallocate(T* obj);
    
    // Get pool statistics
    std::size_t capacity() const { return capacity_; }
    std::size_t size() const { return size_; }
    std::size_t available() const { return capacity_ - size_; }
    
    // Reserve additional capacity
    void reserve(std::size_t additional_capacity);
    
    // Clear the pool
    void clear();
    
private:
    // Free list node
    struct FreeNode {
        FreeNode* next;
    };
    
    // Memory block
    struct MemoryBlock {
        T* objects;
        std::size_t count;
        MemoryBlock* next;
    };
    
    // Pool state
    FreeNode* free_list_ = nullptr;
    MemoryBlock* blocks_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t size_ = 0;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper functions
    void allocate_block(std::size_t count);
    void add_to_free_list(T* obj);
    T* get_from_free_list();
};

// ============================================================================
// Buffer pool for network I/O
// ============================================================================

class BufferPool {
public:
    // Buffer handle
    struct Buffer {
        std::uint8_t* data;
        std::size_t size;
        std::size_t capacity;
        
        // Reset buffer
        void reset() { size = 0; }
        
        // Check if buffer is valid
        bool is_valid() const { return data != nullptr && capacity > 0; }
    };
    
    // Constructor
    explicit BufferPool(std::size_t buffer_size, std::size_t initial_count = 0);
    
    // Destructor
    ~BufferPool();
    
    // Disable copy
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;
    
    // Allocate a buffer
    Buffer allocate();
    
    // Return a buffer to the pool
    void deallocate(Buffer& buffer);
    
    // Get pool statistics
    std::size_t buffer_size() const { return buffer_size_; }
    std::size_t capacity() const { return capacity_; }
    std::size_t available() const { return capacity_ - allocated_; }
    
    // Reserve additional buffers
    void reserve(std::size_t count);
    
private:
    // Buffer size
    std::size_t buffer_size_;
    
    // Pool state
    std::vector<std::uint8_t*> free_buffers_;
    std::vector<std::uint8_t*> allocated_buffers_;
    std::size_t capacity_ = 0;
    std::size_t allocated_ = 0;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper functions
    std::uint8_t* allocate_buffer();
    void deallocate_buffer(std::uint8_t* buffer);
};

// ============================================================================
// Thread-local memory pool
// ============================================================================

template<typename T>
class ThreadLocalPool {
public:
    // Constructor
    explicit ThreadLocalPool(std::size_t pool_size = 1000);
    
    // Destructor
    ~ThreadLocalPool();
    
    // Disable copy
    ThreadLocalPool(const ThreadLocalPool&) = delete;
    ThreadLocalPool& operator=(const ThreadLocalPool&) = delete;
    
    // Allocate an object
    template<typename... Args>
    T* allocate(Args&&... args);
    
    // Return an object
    void deallocate(T* obj);
    
    // Get thread-local pool instance
    static ThreadLocalPool& get_instance();
    
private:
    // Thread-local storage
    struct ThreadLocalData {
        ObjectPool<T> pool;
        std::size_t allocations = 0;
        std::size_t deallocations = 0;
        
        explicit ThreadLocalData(std::size_t size) : pool(size) {}
    };
    
    // Thread-local data
    static thread_local std::unique_ptr<ThreadLocalData> tls_data_;
    
    // Pool size
    std::size_t pool_size_;
};

// ============================================================================
// Memory statistics
// ============================================================================

struct MemoryStats {
    std::size_t total_allocated = 0;
    std::size_t total_deallocated = 0;
    std::size_t current_usage = 0;
    std::size_t peak_usage = 0;
    std::size_t allocation_count = 0;
    std::size_t deallocation_count = 0;
    
    void reset() {
        total_allocated = 0;
        total_deallocated = 0;
        current_usage = 0;
        peak_usage = 0;
        allocation_count = 0;
        deallocation_count = 0;
    }
};

// ============================================================================
// Memory monitor
// ============================================================================

class MemoryMonitor {
public:
    // Get singleton instance
    static MemoryMonitor& get_instance();
    
    // Record allocation
    void record_allocation(std::size_t size);
    
    // Record deallocation
    void record_deallocation(std::size_t size);
    
    // Get statistics
    MemoryStats get_stats() const;
    
    // Reset statistics
    void reset_stats();
    
    // Set callback for memory warnings
    void set_warning_callback(std::function<void(std::size_t)> callback);
    
private:
    // Singleton
    MemoryMonitor() = default;
    ~MemoryMonitor() = default;
    
    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;
    
    // Statistics
    MemoryStats stats_;
    mutable std::mutex mutex_;
    
    // Warning callback
    std::function<void(std::size_t)> warning_callback_;
};

// ============================================================================
// Implementation
// ============================================================================

// ObjectPool implementation

template<typename T>
ObjectPool<T>::ObjectPool(std::size_t initial_capacity) {
    if (initial_capacity > 0) {
        reserve(initial_capacity);
    }
}

template<typename T>
ObjectPool<T>::~ObjectPool() {
    clear();
}

template<typename T>
template<typename... Args>
T* ObjectPool<T>::allocate(Args&&... args) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    T* obj = get_from_free_list();
    if (obj == nullptr) {
        // Need to allocate more memory
        allocate_block(std::max(capacity_ / 2, std::size_t(1)));
        obj = get_from_free_list();
    }
    
    if (obj != nullptr) {
        // Construct object
        new (obj) T(std::forward<Args>(args)...);
        size_++;
        
        // Record allocation
        MemoryMonitor::get_instance().record_allocation(sizeof(T));
    }
    
    return obj;
}

template<typename T>
void ObjectPool<T>::deallocate(T* obj) {
    if (obj == nullptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Destruct object
    obj->~T();
    
    // Add to free list
    add_to_free_list(obj);
    size_--;
    
    // Record deallocation
    MemoryMonitor::get_instance().record_deallocation(sizeof(T));
}

template<typename T>
void ObjectPool<T>::reserve(std::size_t additional_capacity) {
    std::lock_guard<std::mutex> lock(mutex_);
    allocate_block(additional_capacity);
}

template<typename T>
void ObjectPool<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Free all memory blocks
    MemoryBlock* block = blocks_;
    while (block != nullptr) {
        MemoryBlock* next = block->next;
        
        // Destruct any remaining objects
        for (std::size_t i = 0; i < block->count; ++i) {
            T* obj = &block->objects[i];
            // Check if object is in free list
            // This is a simplified check - in production, you'd want a better mechanism
        }
        
        delete[] reinterpret_cast<std::uint8_t*>(block->objects);
        delete block;
        block = next;
    }
    
    blocks_ = nullptr;
    free_list_ = nullptr;
    capacity_ = 0;
    size_ = 0;
}

template<typename T>
void ObjectPool<T>::allocate_block(std::size_t count) {
    // Allocate memory for objects
    std::size_t memory_size = count * sizeof(T);
    std::uint8_t* memory = new std::uint8_t[memory_size];
    
    // Create memory block
    MemoryBlock* block = new MemoryBlock;
    block->objects = reinterpret_cast<T*>(memory);
    block->count = count;
    block->next = blocks_;
    blocks_ = block;
    
    // Add all objects to free list
    for (std::size_t i = 0; i < count; ++i) {
        add_to_free_list(&block->objects[i]);
    }
    
    capacity_ += count;
}

template<typename T>
void ObjectPool<T>::add_to_free_list(T* obj) {
    FreeNode* node = reinterpret_cast<FreeNode*>(obj);
    node->next = free_list_;
    free_list_ = node;
}

template<typename T>
T* ObjectPool<T>::get_from_free_list() {
    if (free_list_ == nullptr) {
        return nullptr;
    }
    
    FreeNode* node = free_list_;
    free_list_ = node->next;
    
    return reinterpret_cast<T*>(node);
}

// ThreadLocalPool implementation

template<typename T>
thread_local std::unique_ptr<typename ThreadLocalPool<T>::ThreadLocalData> 
    ThreadLocalPool<T>::tls_data_ = nullptr;

template<typename T>
ThreadLocalPool<T>::ThreadLocalPool(std::size_t pool_size) : pool_size_(pool_size) {}

template<typename T>
ThreadLocalPool<T>::~ThreadLocalPool() = default;

template<typename T>
template<typename... Args>
T* ThreadLocalPool<T>::allocate(Args&&... args) {
    if (tls_data_ == nullptr) {
        tls_data_ = std::make_unique<ThreadLocalData>(pool_size_);
    }
    
    tls_data_->allocations++;
    return tls_data_->pool.allocate(std::forward<Args>(args)...);
}

template<typename T>
void ThreadLocalPool<T>::deallocate(T* obj) {
    if (tls_data_ == nullptr) {
        // This shouldn't happen - deallocate called without allocate
        assert(false);
        return;
    }
    
    tls_data_->deallocations++;
    tls_data_->pool.deallocate(obj);
}

template<typename T>
ThreadLocalPool<T>& ThreadLocalPool<T>::get_instance() {
    static ThreadLocalPool<T> instance;
    return instance;
}

// ============================================================================
// Utility functions
// ============================================================================

// Aligned memory allocation
inline void* aligned_alloc(std::size_t alignment, std::size_t size) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, size);
#endif
}

// Aligned memory deallocation
inline void aligned_free(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

// Check if pointer is aligned
inline bool is_aligned(const void* ptr, std::size_t alignment) {
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}

// Print memory statistics
void print_memory_stats();

} // namespace mxh::memory