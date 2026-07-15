// object_pool.hpp — Phase 8.2: Object pool for frequent allocations.
//
// Reduces heap allocation pressure for Message objects, connection buffers,
// and other frequently created/destroyed objects.
//
// Design:
//   - Templated on object type T
//   - Pre-allocates a batch of objects (default: 64)
//   - acquire() returns a unique_ptr with custom deleter that returns to pool
//   - release() returns an object to the pool
//   - Thread-safe (mutex-protected free list)
//   - Grows on demand when pool is exhausted (avoids blocking)
//
// Usage:
//   ObjectPool<Message> pool(128);
//   auto msg = pool.acquire();  // msg is unique_ptr<Message, Deleter>
//   msg->payload.resize(64);
//   // msg automatically returns to pool when it goes out of scope

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace mxh::util {

template <typename T>
class ObjectPool {
public:
    // Custom deleter that returns the object to the pool.
    struct Deleter {
        ObjectPool* pool;
        void operator()(T* obj) {
            if (pool) pool->release(obj);
            else delete obj;
        }
    };

    using Ptr = std::unique_ptr<T, Deleter>;

    explicit ObjectPool(std::size_t initial_size = 64)
        : high_watermark_(0) {
        free_list_.reserve(initial_size);
        for (std::size_t i = 0; i < initial_size; ++i) {
            free_list_.push_back(new T());
        }
    }

    ~ObjectPool() {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto* p : free_list_) delete p;
        free_list_.clear();
    }

    // Non-copyable, non-movable.
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Acquire an object from the pool. If pool is empty, allocates a new one.
    [[nodiscard]] Ptr acquire() {
        T* obj = nullptr;
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (!free_list_.empty()) {
                obj = free_list_.back();
                free_list_.pop_back();
            }
        }
        if (!obj) {
            obj = new T();
            std::lock_guard<std::mutex> lk(mu_);
            ++high_watermark_;
        }
        return Ptr(obj, Deleter{this});
    }

    // Return an object to the pool.
    void release(T* obj) {
        if (!obj) return;
        // Reset the object to default state before returning to pool.
        *obj = T{};
        std::lock_guard<std::mutex> lk(mu_);
        free_list_.push_back(obj);
    }

    [[nodiscard]] std::size_t available() {
        std::lock_guard<std::mutex> lk(mu_);
        return free_list_.size();
    }

    [[nodiscard]] std::size_t high_watermark() const noexcept {
        return high_watermark_.load();
    }

private:
    std::vector<T*> free_list_;
    std::mutex mu_;
    std::atomic<std::size_t> high_watermark_;
};

}  // namespace mxh::util
