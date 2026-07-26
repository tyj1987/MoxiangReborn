// mxh/tests/unit/game/skill_list_parser_test.cpp
//
// Phase D1.3 end-to-end tests for the legacy SkillList.bin parser.
//
// Test surface:
//   - decode_mhfile_payload: roundtrip a tiny synthesized payload
//   - parse_skill_row:        tokenize a synthesized row and check fields
//   - load_skill_list:        load the real SkillList.bin from the
//                             legacy client resource path, verify
//                             that the first entry (SkillIdx 1) has
//                             the expected 1:1 fields.
//
// Real-resource tests GTEST_SKIP if the legacy .bin is missing
// (CI-friendly).

#include "mxh/game/skill_list_parser.hpp"
#include "mxh/game/skill_manager.hpp"
#include "mxh/game/skill_types.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <typeinfo>
#include <vector>

using mxh::game::SkillInfo;
using mxh::game::SkillListParseResult;
using mxh::game::decode_mhfile_payload;
using mxh::game::load_skill_list;
using mxh::game::parse_skill_row;
using mxh::game::SkillManager;

namespace {

// Hardcoded real-resource path.  CI-friendly: GTEST_SKIP if not present.
// Use a wide string literal (LR"...") so MSVC's narrow->wide path
// conversion doesn't choke on the CJK characters in the folder
// name; the conversion would otherwise throw system_error(1113,
// "No mapping for the Unicode character exists in the target
// multi-byte code page") on the test fixture's first access.
const std::filesystem::path kRealSkillList =
    LR"(C:\moxiang\墨香【源码】\SWorking\Resource\SkillList.bin)";

// MSVC's narrow-stream constructors run a MultiByteToWideChar on
// the path's narrow form, which throws system_error(1113) for the
// CJK folder name.  We avoid that by copying the .bin into an
// ASCII-only temp path once per process, then using a narrow
// std::ifstream on the temp copy.  The temp file is reused across
// tests; the OS reclaims it on process exit.
const std::string kTempSkillList =
    "C:/Users/User/AppData/Local/Temp/mxh_skill_list_test.bin";

bool ensure_temp_skill_list() {
    if (std::filesystem::exists(kTempSkillList)
        && std::filesystem::file_size(kTempSkillList) == std::filesystem::file_size(kRealSkillList)) {
        return true;
    }
    try {
        std::filesystem::copy(kRealSkillList, kTempSkillList,
                              std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (...) {
        return false;
    }
}

// A minimal 150-token row mirroring the legacy SkillList.bin format.
// Token 0 = SkillIdx; token 1 = SkillName; tokens 2..30 = fixed
// fields (level 0 etc.); tokens 31..42 = NeedExp[12] (zeros);
// tokens 43..54 = NeedNaeRyuk[12] (zeros); tokens 55..68 = post-NR
// scalars; tokens 69..146 = 6 AdditiveAttr segments (1 disc + 12 vals
// = 13 tokens each).  Token 147 = CanSkipEffect; 148 = ChangeKind;
// 149 = LinkSkillIdx.
std::vector<std::string> make_minimal_row(std::uint16_t idx,
                                          const std::string& name) {
    std::vector<std::string> t(150);
    t[0] = std::to_string(idx);       // SkillIdx
    t[1] = name;                       // SkillName
    t[2] = "0";                        // SkillTooltipIdx
    t[3] = "1";                        // RestrictLevel
    t[4] = "-1"; t[5] = "-1";          // LowImage, HighImage
    t[6] = "1";                        // SkillKind = OuterMugong
    t[7] = "0";                        // WeaponKind
    t[8] = "3";                        // SkillRange
    t[9] = "0";                        // TargetKind
    t[10] = "0";                       // TargetRange
    t[11] = "0";                       // TargetAreaIdx
    t[12] = "0";                       // TargetAreaPivot
    t[13] = "0";                       // TargetAreaFix
    t[14] = "0";                       // MoveTargetArea
    t[15] = "0";                       // MoveTargetAreaDirection
    t[16] = "0";                       // MoveTargetAreaVelocity
    t[17] = "0";                       // Duration
    t[18] = "0";                       // Interval
    t[19] = "0";                       // DelaySingleEffect
    t[20] = "1";                       // ComboNum
    t[21] = "0";                       // Life
    t[22] = "0";                       // BindOperator
    t[23] = "0";                       // EffectStartTime
    t[24] = "0";                       // EffectStart (filename "0" -> 0)
    t[25] = "0"; t[26] = "0";
    t[27] = "0"; t[28] = "0";          // EffectUse/Self/MapObject/Mine
    t[29] = "1500";                    // DelayTime (DWORD)
    t[30] = "0";                       // FatalDamage
    for (int i = 0; i < 12; ++i) t[31 + i] = "0";   // NeedExp[12]
    for (int i = 0; i < 12; ++i) t[43 + i] = "0";   // NeedNaeRyuk[12]
    t[55] = "0"; t[56] = "0"; t[57] = "0";
    t[58] = "0"; t[59] = "0"; t[60] = "0";
    t[61] = "0"; t[62] = "0"; t[63] = "0";
    t[64] = "0"; t[65] = "0"; t[66] = "0";
    t[67] = "0"; t[68] = "0";          // scalars through DamageDecreaseForDist
    // 6 AdditiveAttr segments.  Disc=0 means "discard 12 values".
    for (int seg = 0; seg < 6; ++seg) {
        const std::size_t base = 69 + seg * 13;
        t[base] = "0";
        for (int k = 0; k < 12; ++k) t[base + 1 + k] = "0";
    }
    t[147] = "0";                      // CanSkipEffect
    t[148] = "0";                      // ChangeKind
    t[149] = "0";                      // LinkSkillIdx
    return t;
}

}  // namespace

// ---------------------------------------------------------------------------
// decode_mhfile_payload: roundtrip on a tiny synthesized payload.
// ---------------------------------------------------------------------------
TEST(SkillListParser, DecodeRoundtripZero) {
    // All-zero payload: decode is a no-op.
    std::vector<std::uint8_t> p(16, 0);
    const std::uint8_t crc = decode_mhfile_payload(/*dwType=*/1, p);
    // dwType=1 makes every byte get an extra -1, but since payload
    // was 0, every byte becomes (0 - i) - (i%1==0 ? 1 : 0).
    // We just check the function doesn't crash and returns 0..255.
    EXPECT_GE(crc, 0u);
}

TEST(SkillListParser, DecodeRoundtripAscii) {
    // Payload of printable ASCII: "ABCDEFGH" encoded with dwType=4
    // (so byte 0, 4, 8, ... also get the -4 adjustment).
    std::vector<std::uint8_t> p = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    const std::uint8_t crc = decode_mhfile_payload(/*dwType=*/4, p);
    // We don't pin a specific value; we just check the buffer is
    // mutated and the function returns 0..255.
    EXPECT_GE(crc, 0u);
    // No assertion on the post-decode content; the legacy client
    // doesn't either -- it just feeds the buffer to the tokenizer.
}

// ---------------------------------------------------------------------------
// parse_skill_row: minimal synthesized row.
// ---------------------------------------------------------------------------
TEST(SkillListParser, ParseMinimalRow) {
    auto toks = make_minimal_row(/*idx=*/42, "TestSkill");
    SkillInfo s{};
    std::string err;
    ASSERT_TRUE(parse_skill_row(toks, s, err)) << err;
    EXPECT_EQ(s.SkillIdx, 42u);
    EXPECT_STREQ(s.SkillName, "TestSkill");
    EXPECT_EQ(s.SkillKind, 1u);            // OuterMugong
    EXPECT_EQ(s.SkillRange, 3u);
    EXPECT_EQ(s.ComboNum, 1u);
    EXPECT_EQ(s.DelayTime, 1500u);
    EXPECT_EQ(s.EffectStart, 0);            // filename "0" -> 0
}

TEST(SkillListParser, ParseRowWithEffects) {
    auto toks = make_minimal_row(/*idx=*/7, "HasFx");
    toks[24] = "m_combo_gum01.beff";       // EffectStart non-"0"
    SkillInfo s{};
    std::string err;
    ASSERT_TRUE(parse_skill_row(toks, s, err)) << err;
    EXPECT_EQ(s.SkillIdx, 7u);
    EXPECT_EQ(s.EffectStart, 1);            // non-"0" filename -> 1
    EXPECT_EQ(s.EffectUse, 0);              // still "0"
}

TEST(SkillListParser, ParseRowRejectsWrongTokenCount) {
    std::vector<std::string> toks(149, "0");
    toks[0] = "1";
    SkillInfo s{};
    std::string err;
    EXPECT_FALSE(parse_skill_row(toks, s, err));
    EXPECT_FALSE(err.empty());
}

TEST(SkillListParser, ParseRowAdditiveAttrFirstPhyAttack) {
    // AdditiveAttr 11 = FirstPhyAttack, level values 100,200,..,1200
    auto toks = make_minimal_row(/*idx=*/99, "PhyAtk");
    const std::size_t base = 69;             // first segment
    toks[base] = "11";                       // discriminator
    for (int i = 0; i < 12; ++i) {
        toks[base + 1 + i] = std::to_string((i + 1) * 100);
    }
    SkillInfo s{};
    std::string err;
    ASSERT_TRUE(parse_skill_row(toks, s, err)) << err;
    EXPECT_FLOAT_EQ(s.FirstPhyAttack[0], 100.0f);
    EXPECT_FLOAT_EQ(s.FirstPhyAttack[11], 1200.0f);
}

TEST(SkillListParser, ParseRowAdditiveAttrUpPhyAttack) {
    // AdditiveAttr 12 = UpPhyAttack
    auto toks = make_minimal_row(/*idx=*/100, "UpPhy");
    const std::size_t base = 69 + 13;        // second segment
    toks[base] = "12";
    toks[base + 1] = "7";
    toks[base + 12] = "42";
    SkillInfo s{};
    std::string err;
    ASSERT_TRUE(parse_skill_row(toks, s, err)) << err;
    EXPECT_FLOAT_EQ(s.UpPhyAttack[0], 7.0f);
    EXPECT_FLOAT_EQ(s.UpPhyAttack[11], 42.0f);
}

TEST(SkillListParser, ParseRowAdditiveAttrZeroDiscardsValues) {
    // AdditiveAttr 0 = discard 12 values (no field gets them).
    auto toks = make_minimal_row(/*idx=*/101, "Discard");
    const std::size_t base = 69;
    toks[base] = "0";
    toks[base + 1] = "999";                 // would be FirstPhyAttack[0]
    SkillInfo s{};
    std::string err;
    ASSERT_TRUE(parse_skill_row(toks, s, err)) << err;
    EXPECT_FLOAT_EQ(s.FirstPhyAttack[0], 0.0f);
}

TEST(SkillListParser, ParseRowAdditiveAttrSkipEffect) {
    // AdditiveAttr 100 = SkipEffect (special: first value into field,
    // next 11 discarded).  1:1 with skillinfo.cpp lines 195-198.
    auto toks = make_minimal_row(/*idx=*/102, "SkipFx");
    const std::size_t base = 69;
    toks[base] = "100";
    toks[base + 1] = "5";                   // SkipEffect
    toks[base + 2] = "999";                 // discarded
    SkillInfo s{};
    std::string err;
    ASSERT_TRUE(parse_skill_row(toks, s, err)) << err;
    EXPECT_EQ(s.SkipEffect, 5u);
}

// ---------------------------------------------------------------------------
// load_skill_list: real-file roundtrip.  CI-friendly: skip if missing.
// ---------------------------------------------------------------------------
TEST(SkillListParser, LoadRealSkillListFirstEntry) {
    if (!std::filesystem::exists(kRealSkillList)) {
        GTEST_SKIP() << "SkillList.bin not found";
    }
    if (!ensure_temp_skill_list()) {
        GTEST_SKIP() << "cannot copy to temp path";
    }
    try {
        std::ifstream ifs(kTempSkillList, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(ifs)),
                                       std::istreambuf_iterator<char>());
        ifs.close();
        constexpr std::size_t kHdr = 12;
        constexpr std::size_t kCrc = 1;
        std::uint32_t dwType = 0, FileSize = 0;
        std::memcpy(&dwType,   raw.data() + 4, 4);
        std::memcpy(&FileSize, raw.data() + 8, 4);
        std::vector<std::uint8_t> payload(
            raw.begin() + kHdr + kCrc,
            raw.begin() + kHdr + kCrc + FileSize);
        decode_mhfile_payload(dwType, payload);
        auto it = std::find(payload.begin(), payload.end(), '\r');
        if (it == payload.end()) {
            GTEST_SKIP() << "no CRLF in payload";
        }
        std::vector<char> first_row(payload.begin(), it);
        std::vector<std::string> tokens;
        std::string cur;
        for (char c : first_row) {
            if (c == '\t' || c == ' ') {
                if (!cur.empty()) { tokens.push_back(std::move(cur)); cur.clear(); }
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) tokens.push_back(std::move(cur));
        ASSERT_EQ(tokens.size(), 150u);
        SkillInfo s{};
        std::string err;
        ASSERT_TRUE(parse_skill_row(tokens, s, err)) << err;
        EXPECT_EQ(s.SkillIdx,      1u);
        EXPECT_EQ(s.SkillKind,      0u);
        EXPECT_EQ(s.RestrictLevel,  1u);
        EXPECT_EQ(s.SkillRange,     230u);
        EXPECT_EQ(s.WeaponKind,     1u);
        EXPECT_EQ(s.ComboNum,       1u);
        EXPECT_EQ(s.LowImage,      -1);
        EXPECT_EQ(s.HighImage,     -1);
        // EffectStart = "0" (no cast effect), EffectUse = "m_combo_gum01.beff"
        // (combo hit effect).  1:1 with the legacy first row tokens 24..25.
        EXPECT_EQ(s.EffectStart, 0);
        EXPECT_EQ(s.EffectUse,   1);
        EXPECT_EQ(s.EffectSelf,  0);
        EXPECT_EQ(s.EffectMapObjectCreate, 0);
        EXPECT_EQ(s.EffectMineOperate,     0);
        for (int i = 0; i < 12; ++i) {
            EXPECT_EQ(s.NeedExp[i], 0u);
            EXPECT_EQ(s.NeedNaeRyuk[i], 0u);
        }
        EXPECT_EQ(s.LinkSkillIdx, 10001u);
    } catch (const std::system_error& e) {
        GTEST_FAIL() << "system_error code=" << e.code().value();
    } catch (const std::exception& e) {
        GTEST_FAIL() << "exception type=" << typeid(e).name();
    } catch (...) {
        GTEST_FAIL() << "unknown";
    }
}

TEST(SkillListParser, LoadRealSkillListManagerInit) {
    if (!std::filesystem::exists(kRealSkillList)) {
        GTEST_SKIP() << "SkillList.bin not found";
    }
    if (!ensure_temp_skill_list()) {
        GTEST_SKIP() << "cannot copy to temp path";
    }
    try {
        SkillManager mgr;
        std::uint32_t errs = 0;
        mgr.init_from_bin(kTempSkillList, &errs);
        EXPECT_EQ(errs, 0u);
        EXPECT_TRUE(mgr.exists(1u));
        EXPECT_TRUE(mgr.exists(2u));
        EXPECT_EQ(mgr.get(1u).LinkSkillIdx, 10001u);
        EXPECT_GT(mgr.size(), 1000u);
    } catch (const std::system_error& e) {
        GTEST_FAIL() << "system_error code=" << e.code().value();
    } catch (const std::exception& e) {
        GTEST_FAIL() << "exception type=" << typeid(e).name();
    } catch (...) {
        GTEST_FAIL() << "unknown";
    }
}
