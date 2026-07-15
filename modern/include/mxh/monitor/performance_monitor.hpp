// performance_monitor.hpp - Performance monitoring for Moxian-Reborn server.
//
// This module provides real-time performance monitoring:
//   - Connection statistics
//   - Message throughput
//   - Memory usage
//   - CPU utilization
//   - Network latency
//
// Design:
//   - Lock-free counters for high-frequency metrics
//   - Periodic sampling for system metrics
//   - Configurable alerting thresholds
//   - Export to JSON for monitoring systems
//
// Usage:
//   PerformanceMonitor& monitor = PerformanceMonitor::get_instance();
//   monitor.record_message(size);
//   monitor.record_connection();
//   auto stats = monitor.get_stats();
//   std::cout << stats.to_json() << std::endl;

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mxh::monitor {

// ============================================================================
// Performance statistics
// ============================================================================

struct PerformanceStats {
    // Connection statistics
    std::size_t total_connections = 0;
    std::size_t active_connections = 0;
    std::size_t peak_connections = 0;
    std::size_t connection_rate = 0; // connections/sec
    
    // Message statistics
    std::size_t total_messages_received = 0;
    std::size_t total_messages_sent = 0;
    std::size_t total_bytes_received = 0;
    std::size_t total_bytes_sent = 0;
    double message_rate = 0; // messages/sec
    double throughput_mbps = 0; // MB/s
    
    // Latency statistics
    double avg_latency_ms = 0;
    double p95_latency_ms = 0;
    double p99_latency_ms = 0;
    
    // Memory statistics
    std::size_t memory_usage_bytes = 0;
    std::size_t memory_peak_bytes = 0;
    std::size_t buffer_pool_usage = 0;
    
    // CPU statistics
    double cpu_usage_percent = 0;
    
    // Timestamp
    std::chrono::system_clock::time_point timestamp;
    
    // Convert to JSON
    std::string to_json() const;
};

// ============================================================================
// Performance monitor
// ============================================================================

class PerformanceMonitor {
public:
    // Get singleton instance
    static PerformanceMonitor& get_instance();
    
    // Start monitoring
    void start(std::chrono::milliseconds sample_interval = std::chrono::milliseconds(1000));
    
    // Stop monitoring
    void stop();
    
    // Record connection events
    void record_connection();
    void record_disconnection();
    
    // Record message events
    void record_message_received(std::size_t size);
    void record_message_sent(std::size_t size);
    
    // Record latency
    void record_latency(double latency_ms);
    
    // Record memory usage
    void record_memory_usage(std::size_t bytes);
    
    // Get current statistics
    PerformanceStats get_stats() const;
    
    // Get statistics history
    std::vector<PerformanceStats> get_history(std::size_t count = 60) const;
    
    // Reset statistics
    void reset();
    
    // Set alert callback
    using AlertCallback = std::function<void(const std::string& metric, 
                                           double value, 
                                           double threshold)>;
    void set_alert_callback(AlertCallback callback);
    
    // Set alert thresholds
    void set_connection_threshold(std::size_t max_connections);
    void set_latency_threshold(double max_latency_ms);
    void set_memory_threshold(std::size_t max_bytes);
    void set_cpu_threshold(double max_percent);
    
private:
    // Singleton
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    // Lock-free counters
    std::atomic<std::size_t> total_connections_{0};
    std::atomic<std::size_t> active_connections_{0};
    std::atomic<std::size_t> peak_connections_{0};
    std::atomic<std::size_t> total_messages_received_{0};
    std::atomic<std::size_t> total_messages_sent_{0};
    std::atomic<std::size_t> total_bytes_received_{0};
    std::atomic<std::size_t> total_bytes_sent_{0};
    std::atomic<std::size_t> memory_usage_{0};
    std::atomic<std::size_t> memory_peak_{0};
    
    // Latency tracking
    mutable std::mutex latency_mutex_;
    std::vector<double> latency_samples_;
    std::size_t latency_sample_count_ = 1000;
    
    // Statistics history
    mutable std::mutex history_mutex_;
    std::vector<PerformanceStats> history_;
    std::size_t max_history_size_ = 3600; // 1 hour at 1-second intervals
    
    // Monitoring thread
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    std::chrono::milliseconds sample_interval_{1000};
    
    // Alert system
    AlertCallback alert_callback_;
    std::size_t connection_threshold_ = 10000;
    double latency_threshold_ms_ = 100.0;
    std::size_t memory_threshold_bytes_ = 1024 * 1024 * 1024; // 1GB
    double cpu_threshold_percent_ = 80.0;
    
    // Previous values for rate calculation
    std::size_t prev_messages_received_ = 0;
    std::size_t prev_messages_sent_ = 0;
    std::size_t prev_bytes_received_ = 0;
    std::size_t prev_bytes_sent_ = 0;
    std::size_t prev_connections_ = 0;
    
    // Monitoring thread function
    void monitor_thread_func();
    
    // Calculate statistics
    PerformanceStats calculate_stats() const;
    
    // Check alerts
    void check_alerts(const PerformanceStats& stats);
    
    // Get CPU usage (platform-specific)
    double get_cpu_usage() const;
};

// ============================================================================
// Performance metrics collector
// ============================================================================

class MetricsCollector {
public:
    // Collect all metrics
    static PerformanceStats collect();
    
    // Collect specific metrics
    static std::size_t get_connection_count();
    static std::size_t get_memory_usage();
    static double get_cpu_usage();
    static std::size_t get_thread_count();
};

// ============================================================================
// Utility functions
// ============================================================================

// Format performance stats to string
std::string format_stats(const PerformanceStats& stats);

// Export stats to file
bool export_stats_to_file(const PerformanceStats& stats, const std::string& filename);

// Import stats from file
bool import_stats_from_file(PerformanceStats& stats, const std::string& filename);

} // namespace mxh::monitor