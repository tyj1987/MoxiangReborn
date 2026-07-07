# Phase 3.5 — HSEL vs AES-256-GCM Performance Benchmark

**Date:** 2026-07-08
**Compiler:** MSVC 1944 (VS2022)
**Platform:** Win32, x86, Debug
**Hardware:** Windows 10, bcrypt.dll (AES-NI capable)

## Objective

Compare throughput and latency of the two available ciphers at realistic Moxian
game packet sizes. Determine whether AES-256-GCM is performant enough to replace
HSEL for new client connections.

## Cipher Specifications

| Property | HSEL (TRIPLE DES) | AES-256-GCM |
|---|---|---|
| Algorithm | 3× DES pass + block swap + CRC | AES-CTR + GHASH (authenticated) |
| Key size | 96-bit effective (3× int32) | 256-bit |
| Authentication | None (CRC only) | 128-bit GCM auth tag |
| Implementation | `mxh::crypto::HselStream` | `mxh::crypto::Aes256GcmCipher` via bcrypt.dll |
| NIST standard | None (proprietary) | SP 800-38D |
| Hardware accel | None | AES-NI via CNG |

## Throughput (encrypt + decrypt round-trip)

| Payload | Size | HSEL (MB/s) | AES-256-GCM (MB/s) | Winner | Margin |
|---|---|---|---|---|---|
| Chat | 64B | 563.8 | 126.1 | HSEL | 4.5× |
| Movement | 128B | 713.3 | 256.0 | HSEL | 2.8× |
| Inventory | 256B | 895.6 | 455.4 | HSEL | 2.0× |
| Quest | 512B | 1044.8 | 953.3 | HSEL | 1.1× |
| Data | 1KB | 1251.1 | 1781.4 | **AES** | 1.4× |
| Map | 4KB | 1504.1 | 5204.6 | **AES** | 3.5× |
| Bulk | 16KB | 1547.0 | 10306.0 | **AES** | 6.7× |

## Latency (ns per encrypt+decrypt round-trip)

| Payload | HSEL (ns) | AES (ns) | Difference |
|---|---|---|---|
| 64B | 258.0 | 1055.2 | +797.2 (AES slower) |
| 256B | 600.0 | 1043.6 | +443.6 (AES slower) |
| 1KB | 1992.1 | 1248.9 | -743.2 (AES faster) |
| 4KB | 5394.2 | 1475.9 | -3918.3 (AES faster) |
| 16KB | 21928.9 | 3181.3 | -18747.6 (AES faster) |

## Analysis

### HSEL: fast for small packets

HSEL dominates at packet sizes below 1KB because:
- No authentication overhead (GHASH is a fixed cost regardless of payload)
- Operations are simple XOR/ADD/SUB on 4-byte blocks
- 3 DES passes are memory-light and cache-friendly

The 64B gap (258ns vs 1055ns) is ~800ns of absolute time — negligible in
network terms (a 1ms ping is 1000× larger).

### AES-256-GCM: dominates for large packets

AES crosses over at ~512B and is dramatically faster at 4KB+ due to:
- **AES-NI hardware acceleration** via Windows CNG bcrypt.dll
  (`BCryptOpenAlgorithmProvider` with `BCRYPT_AES_ALGORITHM`)
- GHASH (polynomial hash) is O(n) over the payload — amortized cost drops
  as payload grows
- At 16KB, AES is 6.7× faster than HSEL

### Practical implication for Moxian

Most game packets fall in the 64B–1KB range where HSEL has a modest edge.
The absolute latency difference (sub-millisecond) is invisible to players.
Authentication, however, prevents:
- Packet injection by third parties
- Session hijacking via ciphertext modification
- Replay attacks

Given that the CNG provider is hardware-accelerated on any modern CPU,
AES-256-GCM is safe to use for all new connections.

## Recommendation

| Use case | Recommended cipher | Reason |
|---|---|---|
| Legacy client (old .exe) | HSEL (existing protocol) | Must match server's HSEL decrypt |
| New client / new server | AES-256-GCM | Authenticated, NIST standard, AES-NI fast |
| Large bulk transfer (map data, etc.) | AES-256-GCM | 3–7× faster with AES-NI |

**Protocol negotiation (Phase 3.4):** server should detect client version and
switch cipher accordingly. New connections use AES; legacy clients use HSEL.

## Files

- `tests/unit/crypto_benchmark.cpp` — standalone benchmark (no gtest dependency)
- `tests/unit/CMakeLists.txt` — build target `mxh_crypto_benchmark`
- Run: `./build/tests/unit/Debug/mxh_crypto_benchmark.exe`

## Exit Status

- [x] Benchmark compiles cleanly (0 warnings)
- [x] Both HSEL and AES tested at 5 payload sizes
- [x] Throughput and latency measured
- [x] Report generated (`docs/phase3.5_benchmark_report.md`)
