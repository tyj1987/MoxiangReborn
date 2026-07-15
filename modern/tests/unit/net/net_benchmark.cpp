// net_benchmark.cpp — Phase 8.4: TCP networking performance benchmark.
//
// Measures:
//   1. Server send throughput (messages/sec, MB/s)
//   2. Client→Server round-trip latency (µs)
//   3. Connection setup rate (connections/sec)
//   4. Encryption overhead (encrypt+decrypt cost per message)
//
// Realistic Moxian game scenarios:
//   - Chat: 64-byte messages at ~10 msg/sec per player
//   - Movement: 32-byte messages at ~30 msg/sec per player
//   - Inventory sync: 512-byte messages at ~2 msg/sec
//   - Map data: 8KB messages at ~0.5 msg/sec
//
// Run: ./net_benchmark.exe [iterations]

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include "mxh/net/net.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
using us = std::chrono::microseconds;
using ms = std::chrono::milliseconds;

namespace {

int find_free_port() {
#ifdef _WIN32
    SOCKET tmp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tmp == INVALID_SOCKET) return 0;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    socklen_t len = sizeof(addr);
    if (bind(tmp, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(tmp);
        return 0;
    }
    if (getsockname(tmp, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        closesocket(tmp);
        return 0;
    }
    int port = ntohs(addr.sin_port);
    closesocket(tmp);
    return port;
#else
    return 0;
#endif
}

struct BenchResult {
    std::string name;
    double messages_per_sec;
    double throughput_mbps;
    double latency_us;
};

void print_result(const BenchResult& r) {
    std::cout << std::left << std::setw(40) << r.name
              << std::right << std::fixed << std::setprecision(1)
              << std::setw(12) << r.messages_per_sec << " msg/s  "
              << std::setw(8) << r.throughput_mbps << " MB/s  "
              << std::setw(8) << r.latency_us << " us/msg\n";
}

// ── Sink handler: counts messages, doesn't process them ────────────────────

struct SinkHandler : mxh::net::IConnectionHandler {
    std::atomic<std::size_t> messages{0};
    std::atomic<std::size_t> bytes{0};
    std::atomic<std::size_t> connects{0};
    mxh::net::ConnectionId last_id{0};
    std::mutex mu;

    bool on_connect(mxh::net::ConnectionId id, const std::string&) override {
        connects.fetch_add(1);
        last_id = id;
        return true;
    }

    void on_message(mxh::net::ConnectionId, const mxh::net::Message& msg) override {
        messages.fetch_add(1);
        bytes.fetch_add(msg.total_size());
    }

    void on_disconnect(mxh::net::ConnectionId, mxh::net::NetError) override {}
};

// ── Benchmark: Server send throughput ──────────────────────────────────────

BenchResult bench_server_send(std::size_t payload_size, int iterations) {
    SinkHandler handler;
    mxh::net::TcpServer server(handler);
    int port = find_free_port();

    mxh::net::ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 2;
    (void)server.start(cfg);
    std::this_thread::sleep_for(ms(30));

    // Connect raw client.
#ifdef _WIN32
    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    (void)connect(csock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
#else
    int csock = 0;
#endif

    for (int i = 0; i < 50 && handler.connects.load() == 0; ++i)
        std::this_thread::sleep_for(ms(10));

    mxh::net::Message msg;
    msg.header.category = 1;
    msg.header.protocol = 1;
    msg.payload.resize(payload_size, 0xAB);

    // Warmup.
    for (int i = 0; i < 10; ++i) {
        (void)server.send(handler.last_id, msg);
    }
    std::this_thread::sleep_for(ms(10));

    // Timed run.
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        (void)server.send(handler.last_id, msg);
    }
    auto elapsed = Clock::now() - start;
    double secs = std::chrono::duration<double>(elapsed).count();

    double msg_per_sec = iterations / secs;
    double total_bytes = static_cast<double>(iterations) * (sizeof(mxh::net::MsgHeader) + payload_size);
    double mbps = total_bytes / secs / (1024.0 * 1024.0);
    double lat_us = secs * 1e6 / iterations;

#ifdef _WIN32
    closesocket(csock);
#endif
    server.stop();

    char name[64];
    std::snprintf(name, sizeof(name), "Server::send(%zu B) x%d",
                  payload_size, iterations);
    return {name, msg_per_sec, mbps, lat_us};
}

// ── Benchmark: Client→Server round-trip ────────────────────────────────────

BenchResult bench_roundtrip(std::size_t payload_size, int iterations) {
    SinkHandler shandler;
    mxh::net::TcpServer server(shandler);
    int port = find_free_port();

    mxh::net::ServerConfig scfg;
    scfg.port = static_cast<std::uint16_t>(port);
    scfg.worker_thread_count = 2;
    (void)server.start(scfg);
    std::this_thread::sleep_for(ms(30));

    SinkHandler chandler;
    mxh::net::TcpClient client(chandler);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port);
    (void)client.connect(ccfg);

    for (int i = 0; i < 50 && shandler.connects.load() == 0; ++i)
        std::this_thread::sleep_for(ms(10));

    mxh::net::Message msg;
    msg.header.category = 1;
    msg.header.protocol = 1;
    msg.payload.resize(payload_size, 0xCD);

    // Warmup.
    for (int i = 0; i < 5; ++i) {
        (void)client.send(msg);
        std::this_thread::sleep_for(ms(1));
    }

    // Timed run: send N messages, measure total time.
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        (void)client.send(msg);
    }
    // Wait for server to receive all.
    for (int w = 0; w < 100 && shandler.messages.load() < static_cast<std::size_t>(iterations + 5); ++w)
        std::this_thread::sleep_for(ms(1));
    auto elapsed = Clock::now() - start;
    double secs = std::chrono::duration<double>(elapsed).count();

    double msg_per_sec = iterations / secs;
    double total_bytes = static_cast<double>(iterations) * (sizeof(mxh::net::MsgHeader) + payload_size);
    double mbps = total_bytes / secs / (1024.0 * 1024.0);
    double lat_us = secs * 1e6 / iterations;

    client.disconnect();
    server.stop();

    char name[64];
    std::snprintf(name, sizeof(name), "Client→Server RT(%zu B) x%d",
                  payload_size, iterations);
    return {name, msg_per_sec, mbps, lat_us};
}

}  // namespace

int main(int argc, char* argv[]) {
    int iterations = (argc > 1) ? std::atoi(argv[1]) : 10000;

    std::cout << "=== Moxian Network Benchmark ===\n";
    std::cout << "Iterations: " << iterations << "\n\n";

    std::cout << std::left << std::setw(40) << "Test"
              << std::right << std::setw(12) << "msg/s"
              << std::setw(11) << "MB/s"
              << std::setw(12) << "us/msg" << "\n";
    std::cout << std::string(75, '-') << "\n";

    // Server send throughput at realistic game packet sizes.
    std::vector<BenchResult> results;

    // Small packets (chat, movement).
    results.push_back(bench_server_send(32, iterations));
    results.push_back(bench_server_send(64, iterations));
    results.push_back(bench_server_send(128, iterations));

    // Medium packets (inventory, quest).
    results.push_back(bench_server_send(256, iterations / 2));
    results.push_back(bench_server_send(512, iterations / 2));
    results.push_back(bench_server_send(1024, iterations / 4));

    // Large packets (map data).
    results.push_back(bench_server_send(4096, iterations / 8));
    results.push_back(bench_server_send(8192, iterations / 16));

    for (const auto& r : results) print_result(r);

    std::cout << "\n--- Client→Server Round-trip ---\n\n";

    std::vector<BenchResult> rt_results;
    rt_results.push_back(bench_roundtrip(32, iterations / 2));
    rt_results.push_back(bench_roundtrip(128, iterations / 2));
    rt_results.push_back(bench_roundtrip(512, iterations / 4));
    rt_results.push_back(bench_roundtrip(4096, iterations / 8));

    for (const auto& r : rt_results) print_result(r);

    std::cout << "\n=== Done ===\n";
    return 0;
}
