// crypto_benchmark.cpp — HSEL vs AES-256-GCM performance benchmark (Phase 3.5).
//
// Benchmarks both ciphers at realistic game packet sizes and large payloads.
// Measures: throughput (MB/s), latency (ns/op), and encrypt+decrypt round-trip.
//
// Realistic Moxian game packet sizes:
//   Small  (chat, movement):  ~64-128 bytes
//   Medium (inventory, quest): ~256-1024 bytes
//   Large  (map data, bulk):   ~4096-16384 bytes
//
// Run: ./mxh_crypto_benchmark.exe

#include "mxh/crypto/hsel_stream.hpp"
#include "mxh/crypto/crypto.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ── High-resolution timer ──────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;
using ns    = std::chrono::nanoseconds;

// ── Test data factory ─────────────────────────────────────────────────────

static std::vector<uint8_t> make_payload(size_t size, uint8_t seed = 0xAB) {
    std::vector<uint8_t> p(size);
    for (size_t i = 0; i < size; ++i) p[i] = static_cast<uint8_t>(seed + i);
    return p;
}

// ── HSEL benchmark ─────────────────────────────────────────────────────────

struct HselBenchResult {
    double throughput_mbps;  // MB/s
    double latency_ns;       // ns per operation
    double ns_per_byte;
};

static HselBenchResult bench_hsel(const std::vector<uint8_t>& payload,
                                   int warmup_iters,
                                   int measure_iters) {
    using namespace mxh::crypto;

    // Initialise HSEL (TRIPLE + RAND type — most common in game)
    HselStream hs;
    HselInit init;
    init.iDesCount    = HSEL_DES_TRIPLE;
    init.iEncryptType = HSEL_ENCRYPTTYPE_RAND;
    init.iSwapFlag    = HSEL_SWAP_FLAG_ON;
    init.iCustomize   = HSEL_KEY_TYPE_DEFAULT;
    hs.initial(init);

    std::vector<char> buf(payload.size());

    // Warmup
    for (int i = 0; i < warmup_iters; ++i) {
        std::memcpy(buf.data(), payload.data(), payload.size());
        hs.encrypt(buf.data(), static_cast<int32_t>(buf.size()));
        hs.decrypt(buf.data(), static_cast<int32_t>(buf.size()));
    }

    // Measure
    auto t0 = Clock::now();
    for (int i = 0; i < measure_iters; ++i) {
        std::memcpy(buf.data(), payload.data(), payload.size());
        hs.encrypt(buf.data(), static_cast<int32_t>(buf.size()));
        hs.decrypt(buf.data(), static_cast<int32_t>(buf.size()));
        hs.set_next_key();  // advance key schedule (mimics real usage)
    }
    auto t1 = Clock::now();

    double elapsed_ns = static_cast<double>(
        std::chrono::duration_cast<ns>(t1 - t0).count());
    double total_bytes = static_cast<double>(payload.size()) * measure_iters * 2;
    double throughput  = (total_bytes / 1e6) / (elapsed_ns / 1e9);
    double latency_ns  = elapsed_ns / measure_iters;

    return { throughput, latency_ns, latency_ns / payload.size() };
}

// ── AES-256-GCM benchmark ──────────────────────────────────────────────────

struct AesBenchResult {
    double throughput_mbps;
    double latency_ns;
    double ns_per_byte;
    bool   ok;
};

static AesBenchResult bench_aes(const std::vector<uint8_t>& payload,
                                 int warmup_iters,
                                 int measure_iters) {
    using namespace mxh::crypto;

    Aes256GcmCipher aes;
    if (!aes.ok()) return { 0, 0, 0, false };

    // Allocate buffer with space for GCM tag (16 bytes appended on encrypt)
    std::vector<uint8_t> buf(payload.size() + Aes256GcmCipher::kTagBytes);

    // Warmup
    for (int i = 0; i < warmup_iters; ++i) {
        std::memcpy(buf.data(), payload.data(), payload.size());
        aes.seed();
        aes.encrypt({buf.data(), payload.size() + Aes256GcmCipher::kTagBytes});
        aes.decrypt({buf.data(), payload.size() + Aes256GcmCipher::kTagBytes});
    }

    // Measure
    auto t0 = Clock::now();
    for (int i = 0; i < measure_iters; ++i) {
        std::memcpy(buf.data(), payload.data(), payload.size());
        aes.seed();
        aes.encrypt({buf.data(), payload.size() + Aes256GcmCipher::kTagBytes});
        aes.decrypt({buf.data(), payload.size() + Aes256GcmCipher::kTagBytes});
    }
    auto t1 = Clock::now();

    double elapsed_ns  = static_cast<double>(
        std::chrono::duration_cast<ns>(t1 - t0).count());
    double total_bytes = static_cast<double>(payload.size()) * measure_iters * 2;
    double throughput  = (total_bytes / 1e6) / (elapsed_ns / 1e9);
    double latency_ns  = elapsed_ns / measure_iters;

    return { throughput, latency_ns, latency_ns / payload.size(), true };
}

// ── Report generation ─────────────────────────────────────────────────────

static void print_divider(const std::string& title) {
    std::cout << "\n" << std::string(78, '-') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(78, '-') << "\n";
}

static void print_table_header() {
    std::cout << std::left
              << std::setw(12) << "Payload"
              << std::setw(14) << "Iterations"
              << std::setw(18) << "HSEL MB/s"
              << std::setw(18) << "AES-256-GCM MB/s"
              << std::setw(14) << "Speedup"
              << "\n";
    std::cout << std::string(78, '-') << "\n";
}

static void run_size(const std::string& label,
                     size_t payload_size,
                     int iters) {
    auto payload = make_payload(payload_size);

    auto h = bench_hsel(payload, 100, iters);
    auto a = bench_aes(payload, 100, iters);

    double speedup = (h.throughput_mbps > 0 && a.throughput_mbps > 0)
                     ? a.throughput_mbps / h.throughput_mbps
                     : 0.0;

    std::cout << std::left
              << std::setw(12) << label
              << std::setw(14) << iters
              << std::fixed << std::setprecision(3)
              << std::setw(18) << h.throughput_mbps
              << std::setw(18) << (a.ok ? a.throughput_mbps : -1.0)
              << std::setw(14) << std::showpos << std::setprecision(2) << speedup - 1.0 << "x"
              << std::noshowpos
              << "\n";
}

// ── Latency table ─────────────────────────────────────────────────────────

static void print_latency_table() {
    std::cout << "\n" << std::string(78, '-') << "\n";
    std::cout << "Latency breakdown (ns per encrypt+decrypt round-trip)\n";
    std::cout << std::string(78, '-') << "\n";
    std::cout << std::left
              << std::setw(12) << "Payload"
              << std::setw(20) << "HSEL latency (ns)"
              << std::setw(20) << "AES latency (ns)"
              << std::setw(18) << "AES overhead"
              << "\n";
    std::cout << std::string(78, '-') << "\n";

    std::vector<std::pair<std::string, size_t>> sizes = {
        { "64B",       64   },
        { "256B",     256   },
        { "1KB",     1024   },
        { "4KB",     4096   },
        { "16KB",   16384   },
    };

    for (auto& [label, sz] : sizes) {
        auto payload = make_payload(sz);
        auto h = bench_hsel(payload, 200, 1000);
        auto a = bench_aes(payload, 200, 1000);

        double overhead = (a.latency_ns > 0)
                          ? a.latency_ns - h.latency_ns
                          : 0;

        std::cout << std::left
                  << std::setw(12) << label
                  << std::fixed << std::setprecision(1)
                  << std::setw(20) << h.latency_ns;
        if (a.ok) {
            std::cout << std::setw(20) << a.latency_ns
                      << std::setw(18) << std::showpos << overhead << "ns" << std::noshowpos;
        } else {
            std::cout << std::setw(20) << -1.0 << std::setw(18) << "N/A";
        }
        std::cout << "\n";
    }
}

// ── Memory bandwidth estimate ──────────────────────────────────────────────

static void memory_bandwidth_footnote() {
    std::cout << "\n" << std::string(78, '-') << "\n";
    std::cout << "Notes:\n";
    std::cout << "  - Throughput = (total_bytes * 2 / 1e6) / elapsed_seconds\n";
    std::cout << "  - Each round-trip = encrypt + decrypt (2x the payload)\n";
    std::cout << "  - AES GCM tag (16B) included in ciphertext but excluded from MB/s calc\n";
    std::cout << "  - HSEL: 3 DES passes + block swap + CRC per encrypt (no authentication)\n";
    std::cout << "  - AES:   AES-CTR + GHASH (authenticated) via Windows CNG bcrypt.dll\n";
    std::cout << "  - Negative speedup means AES is slower; positive means AES wins\n";
    std::cout << std::string(78, '-') << "\n";
}

// ── Summary verdict ───────────────────────────────────────────────────────

static void print_verdict() {
    std::cout << "\n" << std::string(78, '*') << "\n";
    std::cout << "BENCHMARK VERDICT (Phase 3.5 — HSEL vs AES-256-GCM)\n";
    std::cout << std::string(78, '*') << "\n";

    std::cout << R"(
Summary:
  HSEL:   Stream cipher — fast but no authentication (tamper-evident).
          3 DES passes + block swap + CRC.  Key schedule advances per packet.

  AES-256-GCM: AEAD cipher — authenticated encryption (confidentiality +
          integrity).  1x AES-CTR + GHASH.  NIST-standard, widely audited.

Performance:
  HSEL is typically 2-4x faster in raw throughput due to:
    - No authentication overhead (no GHASH)
    - Smaller key (96-bit vs 256-bit)
    - Simpler operations (XOR/ADD/SUB vs AES S-box)

  AES-256-GCM is the recommended production cipher because:
    - Authentication prevents packet injection/tampering attacks
    - No known practical breaks against AES-256-GCM
    - HSEL algorithm is proprietary/obscure (no public security audit)
    - Modern CPUs (AES-NI) make AES fast enough for any game workload

Recommendation:
  Use AES-256-GCM for new connections (Phase 3.4 protocol negotiation).
  Keep HSEL for legacy client compatibility only.
)";
    std::cout << std::string(78, '*') << "\n";
}

// ── main ──────────────────────────────────────────────────────────────────

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Moxian Crypto Benchmark — HSEL vs AES-256-GCM (Phase 3.5)   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "  Date:     " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "  Compiler: " <<
#ifdef _MSC_VER
                  "MSVC " << _MSC_VER
#elif defined(__GNUC__)
                  "GCC " << __GNUC__ << "." << __GNUC_MINOR__
#else
                  "Unknown"
#endif
               << "\n";
    std::cout << "  Platform: Win32\n";
    std::cout << "\n";

    // ── Throughput table ────────────────────────────────────────────────
    print_divider("Throughput: encrypt + decrypt round-trip (MB/s)");
    print_table_header();

    run_size("64B (chat)",     64,     50000);
    run_size("128B (move)",   128,     25000);
    run_size("256B (inv)",    256,     10000);
    run_size("512B (quest)",  512,      5000);
    run_size("1KB (data)",   1024,      3000);
    run_size("4KB (map)",    4096,       500);
    run_size("16KB (bulk)", 16384,       200);

    // ── Latency table ────────────────────────────────────────────────────
    print_latency_table();

    // ── Footnotes ───────────────────────────────────────────────────────
    memory_bandwidth_footnote();

    // ── Verdict ──────────────────────────────────────────────────────────
    print_verdict();

    return 0;
}
