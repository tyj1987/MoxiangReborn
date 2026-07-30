// wire_format_coverage_test.cpp
//
// M18 -- wire-format golden coverage audit (C 协议扩展 foundation).
//
// Reads every .bin in modern/tests/unit/server/golden/, extracts the
// category byte (5th byte of the legacy wire frame, index 4), groups
// goldens by category, and reports coverage against the 81 categories
// defined in mxh::proto::Category.
//
// Baseline (M18, 2026-07-30):
//   9 wire-format goldens, 4 distinct categories covered (cat=4, 6, 7, 8).
//   77 of 81 categories have no wire-format golden.
//
// Why a coverage audit before more goldens:
//   1. Catches accidental golden deletion (regression detector).
//   2. Locks the current M-stack coverage count as the floor.
//   3. Prints the gap list to the test log so the next agent knows
//      exactly which cat to target.
//
// Test-only diff. No changes to modern/src/ or to the goldens themselves.

#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

// Decode the 10-byte legacy wire header from a golden file.
// Layout: 2B body_length (LE) | 1B checksum | 1B code | 1B category
//         | 1B protocol | 4B object_id (LE) | payload...
struct GoldenHeader {
    std::uint16_t body_length = 0;
    std::uint8_t  checksum   = 0;
    std::int8_t   code       = 0;
    std::uint8_t  category   = 0;
    std::uint8_t  protocol   = 0;
    std::uint32_t object_id  = 0;
    std::size_t   file_size  = 0;
};

std::optional<GoldenHeader> read_golden_header(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) return std::nullopt;
    std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(f)),
                                  std::istreambuf_iterator<char>{});
    if (buf.size() < 10u) return std::nullopt;
    GoldenHeader h;
    h.body_length = static_cast<std::uint16_t>(buf[0]) |
                    (static_cast<std::uint16_t>(buf[1]) << 8);
    h.checksum    = buf[2];
    h.code        = static_cast<std::int8_t>(buf[3]);
    h.category    = buf[4];
    h.protocol    = buf[5];
    h.object_id   = static_cast<std::uint32_t>(buf[6])  |
                    (static_cast<std::uint32_t>(buf[7])  << 8)  |
                    (static_cast<std::uint32_t>(buf[8])  << 16) |
                    (static_cast<std::uint32_t>(buf[9])  << 24);
    h.file_size   = buf.size();
    return h;
}

// Resolve the golden/ directory using the test source file location so the
// audit works regardless of WORKING_DIRECTORY (ctest vs direct .exe runs).
// This file lives at modern/tests/unit/server/wire_format_coverage_test.cpp,
// so the golden dir is modern/tests/unit/server/golden.
const std::filesystem::path kGoldenDir =
    std::filesystem::path(__FILE__).parent_path() / "golden";

// All 81 category byte values (1..81) that the legacy protocol defines.
// mxh::proto::Category::Max is a terminator (=82), not a real category.
constexpr int kTotalCategories = 81;

// Encrypted goldens (e.g. login_ack_enc.bin) have their body encrypted --
// the category byte is not recoverable from the header. For those files
// we infer the category from the filename. Unencrypted files always have
// the category in cleartext at offset 4.
std::uint8_t infer_category_from_name(const std::string& name) {
    if (name.find("login_ack") != std::string::npos) return 7;
    if (name.find("login_nack") != std::string::npos) return 7;
    if (name.find("login_request") != std::string::npos) return 7;
    if (name.find("dist_connect") != std::string::npos) return 7;
    if (name.find("cat4") != std::string::npos) return 4;
    if (name.find("cat6") != std::string::npos) return 6;
    if (name.find("cat8") != std::string::npos) return 8;
    if (name.find("unknown_category") != std::string::npos) return 8;
    return 0;
}

// Walk modern/tests/unit/server/golden/*.bin and bucket by category.
std::map<std::uint8_t, std::vector<std::string>> golden_by_category() {
    std::map<std::uint8_t, std::vector<std::string>> out;
    if (!std::filesystem::exists(kGoldenDir)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(kGoldenDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".bin") continue;
        auto h = read_golden_header(entry.path());
        if (!h) continue;
        std::uint8_t cat = h->category;
        if (cat < 1u || cat > static_cast<std::uint8_t>(kTotalCategories)) {
            cat = infer_category_from_name(entry.path().filename().string());
            if (cat == 0u) continue;
        }
        out[cat].push_back(entry.path().filename().string());
    }
    return out;
}

// High-priority uncovered categories for the C 协议扩展 arc (M19+).
// Names per mxh::proto::Category enum.
const std::vector<std::uint8_t> kPriorityUncovered = {
    1,   // Server
    2,   // PowerUp
    5,   // Item
    9,   // Mugong
    10,  // AuctionBoard
    11,  // Cheat
    14,  // Party
    22,  // Skill
    28,  // Exchange
    58,  // Wanted
    71,  // Weather
};

TEST(WireFormatCoverage, PrintsCoverageMatrix) {
    const auto by_cat = golden_by_category();
    std::set<std::uint8_t> covered;
    for (const auto& kv : by_cat) covered.insert(kv.first);

    std::ostringstream os;
    os << "Wire-format golden coverage (M18 baseline):\n";
    os << "  Total goldens: " << by_cat.size() << " entries (covering "
       << covered.size() << "/" << kTotalCategories << " categories)\n";
    os << "  COVERED categories:\n";
    for (const auto& kv : by_cat) {
        const char* name = mxh::proto::category_name(
            static_cast<mxh::proto::Category>(kv.first));
        os << "    cat=" << static_cast<int>(kv.first)
           << " (" << (name ? name : "Unknown") << ") -- "
           << kv.second.size() << " golden(s): ";
        for (std::size_t i = 0; i < kv.second.size(); ++i) {
            if (i) os << ", ";
            os << kv.second[i];
        }
        os << "\n";
    }
    os << "  TODO (high-priority uncovered for C 协议扩展 M19+):\n";
    int todo_count = 0;
    for (auto c : kPriorityUncovered) {
        if (covered.count(c)) continue;
        const char* name = mxh::proto::category_name(
            static_cast<mxh::proto::Category>(c));
        os << "    cat=" << static_cast<int>(c)
           << " (" << (name ? name : "Unknown") << ")\n";
        ++todo_count;
    }
    if (todo_count == 0) {
        os << "    (none -- all priority categories covered!)\n";
    }
    os << "  Uncovered: " << (kTotalCategories - static_cast<int>(covered.size()))
       << " of " << kTotalCategories << " categories total.\n";

    // Print to test log for the next agent / CI to see.
    std::cout << "\n" << os.str() << std::endl;
    RecordProperty("coverage_matrix", os.str());
}

TEST(WireFormatCoverage, LocksCurrentCoverageCount) {
    // Floor: M18 baseline. Increment as M19+ add new goldens.
    const auto by_cat = golden_by_category();
    int total_goldens = 0;
    for (const auto& kv : by_cat) total_goldens += static_cast<int>(kv.second.size());
    EXPECT_EQ(total_goldens, 9)
        << "Golden file count regressed. Re-add the missing golden or "
           "bump the floor if intentional.";
    EXPECT_EQ(by_cat.size(), 4u)
        << "Distinct category count regressed. C 协议扩展 should only "
           "grow this number, never shrink.";
    EXPECT_TRUE(by_cat.count(4)) << "cat=4 (Map) golden lost";
    EXPECT_TRUE(by_cat.count(6)) << "cat=6 (Chat) golden lost";
    EXPECT_TRUE(by_cat.count(7)) << "cat=7 (UserConn) golden lost";
    EXPECT_TRUE(by_cat.count(8)) << "cat=8 (Move) golden lost";
}

TEST(WireFormatCoverage, EachGoldenIsValidWireHeader) {
    // Defense in depth: every .bin in golden/ must be a well-formed legacy
    // wire frame: 2B length + N bytes body where N == length. The body may
    // be encrypted (e.g. login_ack_enc.bin) -- in that case the category
    // byte is not at offset 4. We only check length + payload-size here.
    if (!std::filesystem::exists(kGoldenDir)) {
        GTEST_SKIP() << "golden directory missing: " << kGoldenDir.string();
    }
    for (const auto& entry : std::filesystem::directory_iterator(kGoldenDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".bin") continue;
        auto h = read_golden_header(entry.path());
        ASSERT_TRUE(h.has_value())
            << "golden file too small (<10B): " << entry.path().filename();
        EXPECT_EQ(h->body_length + 2u, h->file_size)
            << "golden " << entry.path().filename()
            << " body_length=" << h->body_length
            << " file_size=" << h->file_size << " mismatch";
        // For unencrypted files, the category byte must be a valid protocol
        // category. Encrypted files have encrypted body so the 5th byte is
        // ciphertext and is allowed to fall outside 1..81.
        const bool name_says_enc = entry.path().filename().string().find("_enc")
                                   != std::string::npos;
        if (!name_says_enc) {
            EXPECT_GE(h->category, 1u)
                << "golden " << entry.path().filename()
                << " category=" << static_cast<int>(h->category)
                << " must be in 1..81";
            EXPECT_LE(h->category, static_cast<std::uint8_t>(kTotalCategories))
                << "golden " << entry.path().filename()
                << " category byte out of legacy protocol range";
        }
    }
}

}  // namespace
// (reserved for future test cases -- M19+ wire-format golden coverage)
// (end of M18 coverage audit -- M19+ will append category-specific tests here)
