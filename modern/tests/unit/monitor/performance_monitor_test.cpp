// performance_monitor_test.cpp - Unit tests for performance monitoring.
//
// Tests basic performance monitoring functionality:
//   - Connection counting
//   - Message counting
//   - Latency tracking
//   - Statistics calculation

#include "mxh/monitor/performance_monitor.hpp"

#include <chrono>
#include <thread>
#include <gtest/gtest.h>

namespace mxh::monitor::test {

// ============================================================================
// PerformanceMonitor Tests
// ============================================================================

TEST(PerformanceMonitorTest, SingletonInstance) {
    auto& instance1 = PerformanceMonitor::get_instance();
    auto& instance2 = PerformanceMonitor::get_instance();
    
    EXPECT_EQ(&instance1, &instance2);
}

TEST(PerformanceMonitorTest, ConnectionCounting) {
    auto& monitor = PerformanceMonitor::get_instance();
    
    // Get initial stats
    auto initial_stats = monitor.get_stats();
    std::size_t initial_active = initial_stats.active_connections;
    
    // Record connections
    monitor.record_connection();
    monitor.record_connection();
    monitor.record_connection();
    
    auto stats = monitor.get_stats();
    EXPECT_EQ(stats.active_connections, initial_active + 3);
    EXPECT_EQ(stats.total_connections, initial_stats.total_connections + 3);
    
    // Record disconnections
    monitor.record_disconnection();
    monitor.record_disconnection();
    
    stats = monitor.get_stats();
    EXPECT_EQ(stats.active_connections, initial_active + 1);
}

TEST(PerformanceMonitorTest, MessageCounting) {
    auto& monitor = PerformanceMonitor::get_instance();
    
    // Get initial stats
    auto initial_stats = monitor.get_stats();
    std::size_t initial_recv = initial_stats.total_messages_received;
    std::size_t initial_sent = initial_stats.total_messages_sent;
    std::size_t initial_bytes_recv = initial_stats.total_bytes_received;
    std::size_t initial_bytes_sent = initial_stats.total_bytes_sent;
    
    // Record messages
    monitor.record_message_received(100);
    monitor.record_message_received(200);
    monitor.record_message_sent(150);
    
    auto stats = monitor.get_stats();
    EXPECT_EQ(stats.total_messages_received, initial_recv + 2);
    EXPECT_EQ(stats.total_messages_sent, initial_sent + 1);
    EXPECT_EQ(stats.total_bytes_received, initial_bytes_recv + 300);
    EXPECT_EQ(stats.total_bytes_sent, initial_bytes_sent + 150);
}

TEST(PerformanceMonitorTest, LatencyTracking) {
    auto& monitor = PerformanceMonitor::get_instance();
    
    // Record latencies
    for (int i = 0; i < 100; ++i) {
        monitor.record_latency(i * 0.1);
    }
    
    auto stats = monitor.get_stats();
    
    // Average should be around 4.95 ms
    EXPECT_NEAR(stats.avg_latency_ms, 4.95, 0.5);
    
    // P99 should be around 9.9 ms
    EXPECT_NEAR(stats.p99_latency_ms, 9.9, 0.5);
}

TEST(PerformanceMonitorTest, AlertCallback) {
    auto& monitor = PerformanceMonitor::get_instance();
    monitor.reset();  // singleton survives across tests
    
    // Set low threshold
    monitor.set_connection_threshold(1);
    
    // Set alert callback
    std::string last_metric;
    double last_value = 0;
    double last_threshold = 0;
    
    monitor.set_alert_callback([&](const std::string& metric, 
                                   double value, 
                                   double threshold) {
        last_metric = metric;
        last_value = value;
        last_threshold = threshold;
    });
    
    // Record connection that exceeds threshold
    monitor.record_connection();
    
    // Get stats and manually check alerts
    auto stats = monitor.get_stats();
    
    // Verify stats
    EXPECT_EQ(stats.active_connections, 1);
    EXPECT_GT(stats.total_connections, 0);
    
    // Reset threshold
    monitor.set_connection_threshold(10000);
}

TEST(PerformanceMonitorTest, Reset) {
    auto& monitor = PerformanceMonitor::get_instance();
    
    // Record some data
    monitor.record_connection();
    monitor.record_message_received(100);
    monitor.record_latency(10.0);
    
    // Reset
    monitor.reset();
    
    auto stats = monitor.get_stats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.total_messages_received, 0);
    EXPECT_TRUE(stats.avg_latency_ms == 0.0);
}

TEST(PerformanceMonitorTest, StatsToJson) {
    PerformanceStats stats;
    stats.total_connections = 100;
    stats.active_connections = 50;
    stats.peak_connections = 75;
    stats.total_messages_received = 1000;
    stats.total_messages_sent = 800;
    stats.avg_latency_ms = 10.5;
    stats.memory_usage_bytes = 1024 * 1024;
    stats.cpu_usage_percent = 25.0;
    
    std::string json = stats.to_json();
    
    // Check that JSON contains expected fields
    EXPECT_NE(json.find("\"connections\""), std::string::npos);
    EXPECT_NE(json.find("\"messages\""), std::string::npos);
    EXPECT_NE(json.find("\"latency\""), std::string::npos);
    EXPECT_NE(json.find("\"memory\""), std::string::npos);
    EXPECT_NE(json.find("\"cpu\""), std::string::npos);
}

// ============================================================================
// MetricsCollector Tests
// ============================================================================

TEST(MetricsCollectorTest, CollectStats) {
    auto stats = MetricsCollector::collect();
    
    // Stats should be valid
    EXPECT_GE(stats.active_connections, 0);
    EXPECT_GE(stats.total_messages_received, 0);
    EXPECT_GE(stats.memory_usage_bytes, 0);
}

TEST(MetricsCollectorTest, GetConnectionCount) {
    std::size_t count = MetricsCollector::get_connection_count();
    EXPECT_GE(count, 0);
}

TEST(MetricsCollectorTest, GetMemoryUsage) {
    std::size_t usage = MetricsCollector::get_memory_usage();
    EXPECT_GT(usage, 0); // Should be using some memory
}

TEST(MetricsCollectorTest, GetCpuUsage) {
    double cpu = MetricsCollector::get_cpu_usage();
    EXPECT_GE(cpu, 0.0);
    EXPECT_LE(cpu, 100.0);
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST(UtilityFunctionsTest, FormatStats) {
    PerformanceStats stats;
    stats.active_connections = 10;
    stats.peak_connections = 20;
    stats.message_rate = 100.5;
    stats.throughput_mbps = 1.5;
    stats.avg_latency_ms = 5.0;
    stats.p95_latency_ms = 10.0;
    stats.p99_latency_ms = 15.0;
    stats.memory_usage_bytes = 1024 * 1024 * 100;
    stats.cpu_usage_percent = 30.0;
    
    std::string formatted = format_stats(stats);
    
    // Check that formatted string contains key information
    EXPECT_NE(formatted.find("10"), std::string::npos);
    EXPECT_NE(formatted.find("20"), std::string::npos);
    EXPECT_NE(formatted.find("100.5"), std::string::npos);
}

} // namespace mxh::monitor::test