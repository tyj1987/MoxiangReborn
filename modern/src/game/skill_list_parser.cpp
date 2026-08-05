// mxh/src/game/skill_list_parser.cpp - Phase D1.3
//
// 1:1 parser for the legacy `Resource/SkillList.bin` packed-text
// file.  See skill_list_parser.hpp for the file format spec.

#include "mxh/game/skill_list_parser.hpp"
#include "mxh/compat/detail/text_parse.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mxh::game {

namespace {

// MHFile header (3 x uint32_t little-endian).
struct MHFileHeader {
    std::uint32_t dwVersion = 0;
    std::uint32_t dwType    = 0;
    std::uint32_t FileSize  = 0;
};

constexpr std::size_t kHeaderSize = 12;  // 3 * 4
constexpr std::size_t kCrcSize    = 1;   // one byte

// On the legacy map server, FindEffectNum returns 0 if the filename
// is the literal string "0" and 1 for any other non-empty filename.
// Empty filenames are not produced by the parser but treated as 0
// to be defensive.  1:1 with `CommonGameFunc.cpp::FindEffectNum` map
// branch (lines 1257-1264).
int find_effect_num(const std::string& filename) {
    if (filename.empty()) return 0;
    if (filename.size() == 1 && filename[0] == '0') return 0;
    return 1;
}

// Parse a single token as a 16-bit unsigned int.  Returns false on
// underflow/overflow; in that case the value is set to 0.
bool parse_u16(const std::string& tok, std::uint16_t& out) {
    if (tok.empty()) return false;
    try {
        long v = std::stol(tok);
        if (v < 0 || v > 0xFFFFL) return false;
        out = static_cast<std::uint16_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_u32(const std::string& tok, std::uint32_t& out) {
    if (tok.empty()) return false;
    try {
        long long v = std::stoll(tok);
        std::uint64_t uv = (v < 0) ? 0ULL : static_cast<std::uint64_t>(v);
        if (uv > 0xFFFFFFFFULL) return false;
        out = static_cast<std::uint32_t>(uv);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_i32(const std::string& tok, std::int32_t& out) {
    if (tok.empty()) return false;
    try {
        long long v = std::stoll(tok);
        if (v < -0x80000000LL || v > 0x7FFFFFFFLL) return false;
        out = static_cast<std::int32_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_f32(const std::string& tok, float& out) {
    if (tok.empty()) return false;
    try {
        out = std::stof(tok);
        return true;
    } catch (...) {
        return false;
    }
}

// Apply a 12-element AdditiveAttr segment to the SkillInfo.
// Returns false if the discriminator is unknown.  Always consumes
// 13 tokens (discriminator + 12 values); the unused value range
// leaves the corresponding array zero (already initialized).
// 1:1 with skillinfo.cpp lines 129-228.
bool apply_additive_attr(SkillInfo& s, int disc,
                        const std::string* vals) {
    auto parse_arr_u16 = [&](std::uint16_t* dst) {
        for (int i = 0; i < SKILL_MAX_LEVEL; ++i) {
            std::uint16_t v = 0;
            parse_u16(vals[i], v);
            dst[i] = v;
        }
    };
    auto parse_arr_f32 = [&](float* dst) {
        for (int i = 0; i < SKILL_MAX_LEVEL; ++i) {
            float v = 0.0f;
            parse_f32(vals[i], v);
            dst[i] = v;
        }
    };
    auto parse_arr_u32 = [&](std::uint32_t* dst) {
        for (int i = 0; i < SKILL_MAX_LEVEL; ++i) {
            std::uint32_t v = 0;
            parse_u32(vals[i], v);
            dst[i] = v;
        }
    };
    switch (disc) {
        case 0:  /* discard 12 tokens */ return true;
        case 11: parse_arr_f32(s.FirstPhyAttack); return true;
        case 12: parse_arr_f32(s.UpPhyAttack); return true;
        case 13: parse_arr_u16(s.FirstAttAttackMin); return true;
        case 14: parse_arr_u16(s.FirstAttAttackMax); return true;
        case 15: parse_arr_u16(s.ContinueAttAttack); return true;
        case 20: parse_arr_f32(s.CounterPhyAttack); return true;
        case 21: parse_arr_f32(s.CounterAttAttack); return true;
        case 22: parse_arr_f32(s.UpPhyDefence); return true;
        case 23: parse_arr_f32(s.UpAttDefence); return true;
        case 24: parse_arr_u16(s.UpMaxNaeRyuk); return true;
        case 25: parse_arr_u16(s.UpMaxLife); return true;
        case 26: parse_arr_u16(s.ContinueRecoverNaeRyuk); return true;
        case 27: parse_arr_u16(s.FirstRecoverNaeRyuk); return true;
        case 28: parse_arr_u16(s.ContinueRecoverLife); return true;
        case 29: parse_arr_u16(s.FirstRecoverLife); return true;
        case 30: parse_arr_f32(s.CounterDodgeRate); return true;
        case 31: parse_arr_f32(s.CriticalRate); return true;
        case 32: parse_arr_f32(s.StunRate); return true;
        case 33: parse_arr_u16(s.StunTime); return true;
        case 34: parse_arr_u16(s.UpMaxShield); return true;
        case 35: parse_arr_u16(s.AmplifiedPowerPhy); return true;
        case 36: parse_arr_f32(s.VampiricLife); return true;
        case 37: parse_arr_f32(s.VampiricNaeryuk); return true;
        case 38: parse_arr_f32(s.RecoverStateAbnormal); return true;
        case 39: parse_arr_f32(s.DispelAttackFeelRate); return true;
        case 40: parse_arr_f32(s.ChangeSpeedProbability); return true;
        case 41: parse_arr_u16(s.DownMaxLife); return true;
        case 42: parse_arr_u16(s.DownMaxNaeRyuk); return true;
        case 43: parse_arr_u16(s.DownMaxShield); return true;
        case 44: parse_arr_f32(s.DownPhyDefence); return true;
        case 45: parse_arr_f32(s.DownAttDefence); return true;
        case 46: parse_arr_f32(s.DownPhyAttack); return true;
        case 47: parse_arr_u32(s.SkillAdditionalTime); return true;
        case 48: parse_arr_u16(s.AmplifiedPowerAtt); return true;
        case 49: parse_arr_f32(s.AmplifiedPowerAttRate); return true;
        case 50: parse_arr_f32(s.FirstAttAttack); return true;
        case 51: parse_arr_f32(s.ContinueAttAttackRate); return true;
        case 52: parse_arr_f32(s.DamageRate); return true;
        case 53: parse_arr_f32(s.AttackRate); return true;
        case 54: parse_arr_u16(s.ContinueRecoverShield); return true;
        case 55: parse_arr_f32(s.AttackLifeRate); return true;
        case 56: parse_arr_f32(s.AttackShieldRate); return true;
        case 57: parse_arr_f32(s.AttackSuccessRate); return true;
        case 58: parse_arr_f32(s.VampiricReverseLife); return true;
        case 59: parse_arr_f32(s.VampiricReverseNaeryuk); return true;
        case 60: parse_arr_u32(s.AttackPhyLastUp); return true;
        case 61: parse_arr_u32(s.AttackAttLastUp); return true;
        // Cases 100, 101, 200, 201 are special: legacy code reads the
        // first value into SkipEffect/SpecialState/AddDegree/SafeRange
        // and discards the next 11.  We follow the same path: the
        // SkipEffect/SpecialState/AddDegree/SafeRange fields in
        // SkillInfo are set from the FIRST token; the next 11 are
        // ignored (the arrays they would touch in the legacy struct
        // don't exist on our port).
        case 100: {
            std::uint16_t v = 0;
            parse_u16(vals[0], v);
            s.SkipEffect = v;
            return true;
        }
        case 101: {
            std::uint16_t v = 0;
            parse_u16(vals[0], v);
            s.SpecialState = v;
            return true;
        }
        case 200: {
            std::int32_t v = 0;
            parse_i32(vals[0], v);
            s.AddDegree = v;
            return true;
        }
        case 201: {
            std::uint16_t v = 0;
            parse_u16(vals[0], v);
            s.SafeRange = v;
            return true;
        }
        default:
            return false;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// decode_mhfile_payload (thin wrapper around mxh::compat::detail).
// ---------------------------------------------------------------------------
std::uint8_t decode_mhfile_payload(std::uint32_t dwType,
                                   std::vector<std::uint8_t>& payload) {
    return mxh::compat::detail::decode_mhfile_text_payload(dwType, payload);
}

// ---------------------------------------------------------------------------
// parse_skill_row
// ---------------------------------------------------------------------------
bool parse_skill_row(const std::vector<std::string>& tokens,
                     SkillInfo& out,
                     std::string& parse_error_msg) {
    constexpr std::size_t kExpected = 150;
    if (tokens.size() != kExpected) {
        std::ostringstream oss;
        oss << "expected " << kExpected << " tokens, got " << tokens.size();
        parse_error_msg = oss.str();
        return false;
    }
    SkillInfo s{};  // zero-init all fields
    std::size_t p = 0;
    // Helper to advance the token cursor.
    auto take = [&]() -> const std::string& { return tokens[p++]; };

    // 0: SkillIdx
    if (!parse_u16(take(), s.SkillIdx)) {
        parse_error_msg = "bad SkillIdx";
        return false;
    }
    // 1: SkillName (EUC-KR, 16+1 NUL).  Truncate to 16 bytes.
    {
        const std::string& name = take();
        std::size_t n = std::min<std::size_t>(name.size(), SKILL_MAX_NAME - 1);
        for (std::size_t i = 0; i < n; ++i) s.SkillName[i] = name[i];
        // last byte stays 0 (NUL terminator)
    }
    // 2: SkillTooltipIdx
    parse_u16(take(), s.SkillTooltipIdx);
    // 3: RestrictLevel
    parse_u16(take(), s.RestrictLevel);
    // 4-5: LowImage, HighImage
    parse_i32(take(), s.LowImage);
    parse_i32(take(), s.HighImage);
    // 6: SkillKind
    parse_u16(take(), s.SkillKind);
    // 7: WeaponKind
    parse_u16(take(), s.WeaponKind);
    // 8: SkillRange
    parse_u16(take(), s.SkillRange);
    // 9-13: TargetKind..TargetAreaFix
    parse_u16(take(), s.TargetKind);
    parse_u16(take(), s.TargetRange);
    parse_u16(take(), s.TargetAreaIdx);
    parse_u16(take(), s.TargetAreaPivot);
    parse_u16(take(), s.TargetAreaFix);
    // 14-15: MoveTargetArea, MoveTargetAreaDirection
    parse_u16(take(), s.MoveTargetArea);
    parse_u16(take(), s.MoveTargetAreaDirection);
    // 16: MoveTargetAreaVelocity (float)
    parse_f32(take(), s.MoveTargetAreaVelocity);
    // 17: Duration
    parse_u32(take(), s.Duration);
    // 18-19: Interval, DelaySingleEffect
    parse_u16(take(), s.Interval);
    parse_u16(take(), s.DelaySingleEffect);
    // 20-22: ComboNum, Life, BindOperator
    parse_u16(take(), s.ComboNum);
    parse_u16(take(), s.Life);
    parse_u16(take(), s.BindOperator);
    // 23: EffectStartTime
    parse_i32(take(), s.EffectStartTime);
    // 24-28: EffectStart, EffectUse, EffectSelf, EffectMapObjectCreate,
    // EffectMineOperate (all strings -> 0 or 1)
    s.EffectStart            = find_effect_num(take());
    s.EffectUse              = find_effect_num(take());
    s.EffectSelf             = find_effect_num(take());
    s.EffectMapObjectCreate  = find_effect_num(take());
    s.EffectMineOperate      = find_effect_num(take());
    // 29: DelayTime
    parse_u32(take(), s.DelayTime);
    // 30: FatalDamage
    parse_u16(take(), s.FatalDamage);
    // 31..42: NeedExp[12]
    for (int i = 0; i < SKILL_MAX_LEVEL; ++i) {
        parse_u32(take(), s.NeedExp[i]);
    }
    // 43..54: NeedNaeRyuk[12]
    for (int i = 0; i < SKILL_MAX_LEVEL; ++i) {
        parse_u16(take(), s.NeedNaeRyuk[i]);
    }
    // 55-65: Attrib..MineCheckStartTime
    parse_u16(take(), s.Attrib);
    parse_u16(take(), s.NegativeResultTargetType);
    parse_u16(take(), s.TieUpType);
    parse_u16(take(), s.ChangeSpeedType);
    parse_u16(take(), s.ChangeSpeedRate);
    parse_u16(take(), s.Dispel);
    parse_u16(take(), s.PositiveResultTargetType);
    parse_u16(take(), s.Immune);
    parse_u16(take(), s.AIObject);
    parse_u16(take(), s.MineCheckRange);
    parse_u16(take(), s.MineCheckStartTime);
    // 66: CounterDodgeKind
    parse_u16(take(), s.CounterDodgeKind);
    // 67: CounterEffect (string -> 0 or 1)
    s.CounterEffect = find_effect_num(take());
    // 68: DamageDecreaseForDist
    parse_u16(take(), s.DamageDecreaseForDist);
    // 69..146: 6 AdditiveAttr segments (1 disc + 12 values = 13 each)
    for (int seg = 0; seg < 6; ++seg) {
        std::uint16_t disc = 0;
        if (!parse_u16(take(), disc)) {
            parse_error_msg = "bad AdditiveAttr discriminator";
            return false;
        }
        const std::string* vals = &tokens[p];
        p += SKILL_MAX_LEVEL;
        if (!apply_additive_attr(s, static_cast<int>(disc), vals)) {
            // Unknown disc -- still consume 12 tokens, but mark as
            // soft failure.  Legacy client ignored unknown discs.
            // We keep going; the next segment will overwrite.
        }
    }
    // 147: CanSkipEffect (legacy BOOL = int32_t)
    {
        std::int32_t v = 0;
        parse_i32(take(), s.CanSkipEffect);
        (void)v;
    }
    // 148: ChangeKind
    parse_u16(take(), s.ChangeKind);
    // 149: LinkSkillIdx
    parse_u16(take(), s.LinkSkillIdx);
    out = s;
    return true;
}

// ---------------------------------------------------------------------------
// load_skill_list
// ---------------------------------------------------------------------------
SkillListParseResult load_skill_list(const std::string& path) {
    SkillListParseResult result;
    try {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            result.error_message = "cannot open file: " + path;
            return result;
        }
        // Read the whole file.  SkillList.bin is ~770KB, fits in memory.
        std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(ifs)),
                                       std::istreambuf_iterator<char>());
        ifs.close();
        if (raw.size() < kHeaderSize + 2 * kCrcSize) {
            result.error_message = "file too small for MHFile header";
            return result;
        }
        // Parse header.
        MHFileHeader hdr;
        std::memcpy(&hdr.dwVersion, raw.data() + 0,  4);
        std::memcpy(&hdr.dwType,    raw.data() + 4,  4);
        std::memcpy(&hdr.FileSize,  raw.data() + 8,  4);
        const std::uint8_t stored_crc1 = raw[kHeaderSize];
        const std::uint8_t stored_crc2 = raw[raw.size() - kCrcSize];
        if (hdr.FileSize + kHeaderSize + 2 * kCrcSize != raw.size()) {
            std::ostringstream oss;
            oss << "header.FileSize=" << hdr.FileSize
                << " doesn't match file size " << raw.size();
            result.error_message = oss.str();
            return result;
        }
        // Pull out the payload and decode it.
        std::vector<std::uint8_t> payload(raw.begin() + kHeaderSize + kCrcSize,
                                           raw.begin() + kHeaderSize + kCrcSize + hdr.FileSize);
        const std::uint8_t computed_crc = decode_mhfile_payload(hdr.dwType, payload);
        result.decoded_crc = computed_crc;
        result.stored_crc  = stored_crc1;
        (void)stored_crc2;
        // Now parse the decoded text.  We copy the payload into a
        // std::string with explicit per-byte push_back (no locale /
        // codecvt involved) so a 0xDE byte never triggers a Windows
        // MultiByteToWideChar failure.
        std::string text;
        text.reserve(payload.size());
        for (std::uint8_t b : payload) {
            text.push_back(static_cast<char>(b));
        }
        // CRLF split.
        std::vector<std::string> lines;
        {
            std::string cur;
            for (std::size_t i = 0; i < text.size(); ++i) {
                if (i + 1 < text.size()
                    && static_cast<char>(text[i]) == '\r'
                    && static_cast<char>(text[i + 1]) == '\n') {
                    lines.push_back(std::move(cur));
                    cur.clear();
                    ++i;
                } else {
                    cur.push_back(static_cast<char>(text[i]));
                }
            }
            if (!cur.empty()) lines.push_back(std::move(cur));
        }
        // Per-line tokenize + parse.
        for (const std::string& line : lines) {
            bool blank = true;
            for (char c : line) {
                if (!std::isspace(static_cast<unsigned char>(c))) { blank = false; break; }
            }
            if (blank) continue;
            std::vector<std::string> tokens;
            std::string cur;
            for (char c : line) {
                if (c == '\t' || c == ' ') {
                    if (!cur.empty()) {
                        tokens.push_back(std::move(cur));
                        cur.clear();
                    }
                } else {
                    cur.push_back(c);
                }
            }
            if (!cur.empty()) tokens.push_back(std::move(cur));
            ++result.rows_seen;
            try {
                SkillInfo s{};
                std::string err;
                if (!parse_skill_row(tokens, s, err)) {
                    ++result.parse_errors;
                    if (result.error_message.empty()) {
                        result.error_message = "row " + std::to_string(result.rows_seen)
                                             + " parse error: " + err;
                    }
                    continue;
                }
                result.skills.push_back(std::move(s));
            } catch (const std::system_error& e) {
                ++result.parse_errors;
                if (result.error_message.empty()) {
                    result.error_message = "row " + std::to_string(result.rows_seen)
                                         + " system_error code=" + std::to_string(e.code().value());
                }
            } catch (const std::exception& e) {
                ++result.parse_errors;
                if (result.error_message.empty()) {
                    result.error_message = "row " + std::to_string(result.rows_seen)
                                         + " std::exception: "
                                         + std::string(e.what(), 0, 64);
                }
            }
        }
    } catch (const std::system_error& e) {
        result.error_message = std::string("outer system_error code=")
                             + std::to_string(e.code().value());
    } catch (const std::exception& e) {
        result.error_message = std::string("outer std::exception: ")
                             + std::string(e.what(), 0, 64);
    } catch (...) {
        result.error_message = "outer unknown exception";
    }
    return result;
}

}  // namespace mxh::game
