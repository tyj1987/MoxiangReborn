// thread_pool.hpp — Phase 8.1: General-purpose thread pool.
//
// Replaces thread-per-connection in TcpServer with a shared worker pool.
// Uses C++20 std::counting_semaphore for efficient task dispatch.
//
// Design:
//   - Fixed number of worker threads (default: hardware_concurrency)
//   - Lock-free task queue (std::mutex + condition_variable for simplicity)
//   - Tasks are std::function<void()> — can capture any callable
//   - Graceful shutdown: stop() waits for all queued tasks to complete
//
// Usage:
//   ThreadPool pool(4);
//   pool.enqueue([]{ /* work */ });
//   pool.shutdown();  // waits for all tasks

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mxh::util {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads = 0)
        : stop_(false), active_tasks_(0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4;
        }
        workers_.reserve(num_threads);
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    // Non-copyable, non-movable.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Enqueue a task. Returns a future for tasks with return values.
    template <typename F>
    void enqueue(F&& task) {
        {
            std::lock_guard<std::mutex> lk(mu_);
            tasks_.push(std::forward<F>(task));
        }
        cv_.notify_one();
    }

    template <typename F>
    [[nodiscard]] auto enqueue_with_future(F&& task)
        -> std::future<decltype(task())> {
        using R = decltype(task());
        auto promise = std::make_shared<std::promise<R>>();
        auto future = promise->get_future();
        auto wrapper = [p = promise, f = std::forward<F>(task)]() mutable {
            if constexpr (std::is_void_v<R>) {
                f();
                p->set_value();
            } else {
                p->set_value(f());
            }
        };
        enqueue(std::move(wrapper));
        return future;
    }

    // Graceful shutdown: waits for all queued tasks to complete.
    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (stop_) return;
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            if (t.joinable()) t.join();
        }
        workers_.clear();
    }

    [[nodiscard]] std::size_t thread_count() const noexcept {
        return workers_.size();
    }

    [[nodiscard]] std::size_t pending_tasks() {
        std::lock_guard<std::mutex> lk(mu_);
        return tasks_.size();
    }

    [[nodiscard]] std::size_t active_tasks() const noexcept {
        return active_tasks_.load();
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            active_tasks_.fetch_add(1);
            task();
            active_tasks_.fetch_sub(1);
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
    std::atomic<std::size_t> active_tasks_;
};

}  // namespace mxh::util
