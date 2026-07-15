// performance_monitor.cpp - Performance monitoring implementation.
//
// This file implements the performance monitoring for Moxian-Reborn server.
// Provides real-time metrics collection and alerting.

#include "mxh/monitor/performance_monitor.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace mxh::monitor {

// ============================================================================
// PerformanceStats implementation
// ============================================================================

std::string PerformanceStats::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"timestamp\": " << std::chrono::system_clock::to_time_t(timestamp) << ",\n";
    oss << "  \"connections\": {\n";
    oss << "    \"total\": " << total_connections << ",\n";
    oss << "    \"active\": " << active_connections << ",\n";
    oss << "    \"peak\": " << peak_connections << ",\n";
    oss << "    \"rate\": " << connection_rate << "\n";
    oss << "  },\n";
    oss << "  \"messages\": {\n";
    oss << "    \"received\": " << total_messages_received << ",\n";
    oss << "    \"sent\": " << total_messages_sent << ",\n";
    oss << "    \"bytes_received\": " << total_bytes_received << ",\n";
    oss << "    \"bytes_sent\": " << total_bytes_sent << ",\n";
    oss << "    \"rate\": " << message_rate << ",\n";
    oss << "    \"throughput_mbps\": " << throughput_mbps << "\n";
    oss << "  },\n";
    oss << "  \"latency\": {\n";
    oss << "    \"avg_ms\": " << avg_latency_ms << ",\n";
    oss << "    \"p95_ms\": " << p95_latency_ms << ",\n";
    oss << "    \"p99_ms\": " << p99_latency_ms << "\n";
    oss << "  },\n";
    oss << "  \"memory\": {\n";
    oss << "    \"usage_bytes\": " << memory_usage_bytes << ",\n";
    oss << "    \"peak_bytes\": " << memory_peak_bytes << ",\n";
    oss << "    \"buffer_pool_usage\": " << buffer_pool_usage << "\n";
    oss << "  },\n";
    oss << "  \"cpu\": {\n";
    oss << "    \"usage_percent\": " << cpu_usage_percent << "\n";
    oss << "  }\n";
    oss << "}";
    return oss.str();
}

// ============================================================================
// PerformanceMonitor implementation
// ============================================================================

PerformanceMonitor& PerformanceMonitor::get_instance() {
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor() {
    latency_samples_.reserve(latency_sample_count_);
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

void PerformanceMonitor::start(std::chrono::milliseconds sample_interval) {
    if (running_) return;
    
    sample_interval_ = sample_interval;
    running_ = true;
    
    monitor_thread_ = std::thread(&PerformanceMonitor::monitor_thread_func, this);
    
    std::cout << "[Monitor] Performance monitoring started" << std::endl;
}

void PerformanceMonitor::stop() {
    if (!running_) return;
    
    running_ = false;
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    std::cout << "[Monitor] Performance monitoring stopped" << std::endl;
}

void PerformanceMonitor::record_connection() {
    total_connections_.fetch_add(1);
    auto active = active_connections_.fetch_add(1) + 1;
    
    // Update peak
    auto current_peak = peak_connections_.load();
    while (active > current_peak) {
        if (peak_connections_.compare_exchange_weak(current_peak, active)) {
            break;
        }
    }
}

void PerformanceMonitor::record_disconnection() {
    active_connections_.fetch_sub(1);
}

void PerformanceMonitor::record_message_received(std::size_t size) {
    total_messages_received_.fetch_add(1);
    total_bytes_received_.fetch_add(size);
}

void PerformanceMonitor::record_message_sent(std::size_t size) {
    total_messages_sent_.fetch_add(1);
    total_bytes_sent_.fetch_add(size);
}

void PerformanceMonitor::record_latency(double latency_ms) {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    
    if (latency_samples_.size() >= latency_sample_count_) {
        latency_samples_.erase(latency_samples_.begin());
    }
    
    latency_samples_.push_back(latency_ms);
}

void PerformanceMonitor::record_memory_usage(std::size_t bytes) {
    memory_usage_.store(bytes);
    
    // Update peak
    auto current_peak = memory_peak_.load();
    while (bytes > current_peak) {
        if (memory_peak_.compare_exchange_weak(current_peak, bytes)) {
            break;
        }
    }
}

PerformanceStats PerformanceMonitor::get_stats() const {
    return calculate_stats();
}

std::vector<PerformanceStats> PerformanceMonitor::get_history(std::size_t count) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    std::size_t start = 0;
    if (history_.size() > count) {
        start = history_.size() - count;
    }
    
    return std::vector<PerformanceStats>(history_.begin() + start, history_.end());
}

void PerformanceMonitor::reset() {
    total_connections_.store(0);
    active_connections_.store(0);
    peak_connections_.store(0);
    total_messages_received_.store(0);
    total_messages_sent_.store(0);
    total_bytes_received_.store(0);
    total_bytes_sent_.store(0);
    memory_usage_.store(0);
    memory_peak_.store(0);
    
    std::lock_guard<std::mutex> lock(latency_mutex_);
    latency_samples_.clear();
    
    prev_messages_received_ = 0;
    prev_messages_sent_ = 0;
    prev_bytes_received_ = 0;
    prev_bytes_sent_ = 0;
    prev_connections_ = 0;
}

void PerformanceMonitor::set_alert_callback(AlertCallback callback) {
    alert_callback_ = std::move(callback);
}

void PerformanceMonitor::set_connection_threshold(std::size_t max_connections) {
    connection_threshold_ = max_connections;
}

void PerformanceMonitor::set_latency_threshold(double max_latency_ms) {
    latency_threshold_ms_ = max_latency_ms;
}

void PerformanceMonitor::set_memory_threshold(std::size_t max_bytes) {
    memory_threshold_bytes_ = max_bytes;
}

void PerformanceMonitor::set_cpu_threshold(double max_percent) {
    cpu_threshold_percent_ = max_percent;
}

void PerformanceMonitor::monitor_thread_func() {
    while (running_) {
        std::this_thread::sleep_for(sample_interval_);
        
        auto stats = calculate_stats();
        
        // Store in history
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            
            if (history_.size() >= max_history_size_) {
                history_.erase(history_.begin());
            }
            
            history_.push_back(stats);
        }
        
        // Check alerts
        check_alerts(stats);
        
        // Update previous values for rate calculation
        prev_messages_received_ = stats.total_messages_received;
        prev_messages_sent_ = stats.total_messages_sent;
        prev_bytes_received_ = stats.total_bytes_received;
        prev_bytes_sent_ = stats.total_bytes_sent;
        prev_connections_ = stats.active_connections;
    }
}

PerformanceStats PerformanceMonitor::calculate_stats() const {
    PerformanceStats stats;
    stats.timestamp = std::chrono::system_clock::now();
    
    // Connection statistics
    stats.total_connections = total_connections_.load();
    stats.active_connections = active_connections_.load();
    stats.peak_connections = peak_connections_.load();
    stats.connection_rate = stats.active_connections - prev_connections_;
    
    // Message statistics
    stats.total_messages_received = total_messages_received_.load();
    stats.total_messages_sent = total_messages_sent_.load();
    stats.total_bytes_received = total_bytes_received_.load();
    stats.total_bytes_sent = total_bytes_sent_.load();
    
    // Calculate rates
    auto interval_sec = std::chrono::duration<double>(sample_interval_).count();
    if (interval_sec > 0) {
        stats.message_rate = (stats.total_messages_received - prev_messages_received_) / interval_sec;
        stats.throughput_mbps = ((stats.total_bytes_received - prev_bytes_received_) / (1024.0 * 1024.0)) / interval_sec;
    }
    
    // Latency statistics
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        
        if (!latency_samples_.empty()) {
            auto sorted_samples = latency_samples_;
            std::sort(sorted_samples.begin(), sorted_samples.end());
            
            double sum = 0;
            for (auto sample : sorted_samples) {
                sum += sample;
            }
            
            stats.avg_latency_ms = sum / sorted_samples.size();
            
            std::size_t p95_index = sorted_samples.size() * 95 / 100;
            std::size_t p99_index = sorted_samples.size() * 99 / 100;
            
            stats.p95_latency_ms = sorted_samples[p95_index];
            stats.p99_latency_ms = sorted_samples[p99_index];
        }
    }
    
    // Memory statistics
    stats.memory_usage_bytes = memory_usage_.load();
    stats.memory_peak_bytes = memory_peak_.load();
    stats.buffer_pool_usage = 0; // TODO: Get from buffer pool
    
    // CPU statistics
    stats.cpu_usage_percent = get_cpu_usage();
    
    return stats;
}

void PerformanceMonitor::check_alerts(const PerformanceStats& stats) {
    if (!alert_callback_) return;
    
    if (stats.active_connections > connection_threshold_) {
        alert_callback_("connections", stats.active_connections, connection_threshold_);
    }
    
    if (stats.avg_latency_ms > latency_threshold_ms_) {
        alert_callback_("latency", stats.avg_latency_ms, latency_threshold_ms_);
    }
    
    if (stats.memory_usage_bytes > memory_threshold_bytes_) {
        alert_callback_("memory", stats.memory_usage_bytes, memory_threshold_bytes_);
    }
    
    if (stats.cpu_usage_percent > cpu_threshold_percent_) {
        alert_callback_("cpu", stats.cpu_usage_percent, cpu_threshold_percent_);
    }
}

double PerformanceMonitor::get_cpu_usage() const {
#ifdef _WIN32
    FILETIME idle_time, kernel_time, user_time;
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        static ULARGE_INTEGER last_idle = {}, last_kernel = {}, last_user = {};
        
        ULARGE_INTEGER current_idle, current_kernel, current_user;
        current_idle.LowPart = idle_time.dwLowDateTime;
        current_idle.HighPart = idle_time.dwHighDateTime;
        current_kernel.LowPart = kernel_time.dwLowDateTime;
        current_kernel.HighPart = kernel_time.dwHighDateTime;
        current_user.LowPart = user_time.dwLowDateTime;
        current_user.HighPart = user_time.dwHighDateTime;
        
        ULONGLONG idle_diff = current_idle.QuadPart - last_idle.QuadPart;
        ULONGLONG kernel_diff = current_kernel.QuadPart - last_kernel.QuadPart;
        ULONGLONG user_diff = current_user.QuadPart - last_user.QuadPart;
        
        last_idle = current_idle;
        last_kernel = current_kernel;
        last_user = current_user;
        
        ULONGLONG total = kernel_diff + user_diff;
        if (total > 0) {
            return static_cast<double>(total - idle_diff) / total * 100.0;
        }
    }
    return 0.0;
#else
    // Linux implementation
    static long last_total = 0;
    static long last_idle = 0;
    
    std::ifstream file("/proc/stat");
    std::string line;
    if (std::getline(file, line)) {
        long user, nice, system, idle, iowait, irq, softirq;
        std::sscanf(line.c_str(), "cpu %ld %ld %ld %ld %ld %ld %ld",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq);
        
        long total = user + nice + system + idle + iowait + irq + softirq;
        long idle_time = idle + iowait;
        
        long total_diff = total - last_total;
        long idle_diff = idle_time - last_idle;
        
        last_total = total;
        last_idle = idle_time;
        
        if (total_diff > 0) {
            return static_cast<double>(total_diff - idle_diff) / total_diff * 100.0;
        }
    }
    return 0.0;
#endif
}

// ============================================================================
// MetricsCollector implementation
// ============================================================================

PerformanceStats MetricsCollector::collect() {
    return PerformanceMonitor::get_instance().get_stats();
}

std::size_t MetricsCollector::get_connection_count() {
    return PerformanceMonitor::get_instance().get_stats().active_connections;
}

std::size_t MetricsCollector::get_memory_usage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    // Linux implementation
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::size_t rss;
            std::sscanf(line.c_str(), "VmRSS: %zu", &rss);
            return rss * 1024; // Convert from KB to bytes
        }
    }
    return 0;
#endif
}

double MetricsCollector::get_cpu_usage() {
    return PerformanceMonitor::get_instance().get_stats().cpu_usage_percent;
}

std::size_t MetricsCollector::get_thread_count() {
#ifdef _WIN32
    // Windows implementation
    return std::thread::hardware_concurrency();
#else
    // Linux implementation
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

// ============================================================================
// Utility functions
// ============================================================================

std::string format_stats(const PerformanceStats& stats) {
    std::ostringstream oss;
    oss << "=== Performance Statistics ===" << std::endl;
    oss << "Connections: " << stats.active_connections << " active, " 
       << stats.peak_connections << " peak" << std::endl;
    oss << "Messages: " << stats.message_rate << " msg/s, " 
       << stats.throughput_mbps << " MB/s" << std::endl;
    oss << "Latency: " << stats.avg_latency_ms << " ms avg, " 
       << stats.p95_latency_ms << " ms p95, " 
       << stats.p99_latency_ms << " ms p99" << std::endl;
    oss << "Memory: " << stats.memory_usage_bytes / (1024 * 1024) << " MB" << std::endl;
    oss << "CPU: " << stats.cpu_usage_percent << "%" << std::endl;
    return oss.str();
}

bool export_stats_to_file(const PerformanceStats& stats, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << stats.to_json();
    return true;
}

bool import_stats_from_file(PerformanceStats& stats, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Simple JSON parsing (for demo purposes)
    // In production, use a proper JSON library
    std::string line;
    while (std::getline(file, line)) {
        // Parse JSON fields
        // TODO: Implement proper JSON parsing
    }
    
    return true;
}

} // namespace mxh::monitor