//
// 1:1 parser for the legacy `Resource/ItemList.bin` packed-text
// file.  See item_list_parser.hpp for the file format and per-row
// token layout.

#include "mxh/game/item_list_parser.hpp"
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

// MHFile header (3 x uint32_t little-endian). Same struct as the
// SkillList parser, repeated here so this TU compiles standalone.
struct MHFileHeader {
    std::uint32_t dwVersion = 0;
    std::uint32_t dwType    = 0;
    std::uint32_t FileSize  = 0;
};

constexpr std::size_t kHeaderSize = 12;
constexpr std::size_t kCrcSize    = 1;

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
        if (v < 0 || v > 0xFFFFFFFFLL) return false;
        out = static_cast<std::uint32_t>(v);
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

}  // namespace

// ---------------------------------------------------------------------------
// parse_item_row
// ---------------------------------------------------------------------------
bool parse_item_row(const std::vector<std::string>& tokens,
                    ItemInfo& out,
                    std::string& parse_error_msg) {
    // The legacy parser emits 56 (common) or 60 (JAPAN_LOCAL) tokens.
    if (tokens.size() != 56u && tokens.size() != 60u) {
        std::ostringstream oss;
        oss << "expected 56 or 60 tokens, got " << tokens.size();
        parse_error_msg = oss.str();
        return false;
    }
    ItemInfo item{};
    std::size_t p = 0;
    auto take = [&]() -> const std::string& { return tokens[p++]; };

    // 0: ItemIdx
    if (!parse_u16(take(), item.ItemIdx)) {
        parse_error_msg = "bad ItemIdx";
        return false;
    }
    // 1: ItemName (legacy char[31], EUC-KR). Truncate to 30 bytes.
    {
        const std::string& name = take();
        const std::size_t n = std::min<std::size_t>(name.size(), ITEM_MAX_NAME - 1);
        for (std::size_t i = 0; i < n; ++i) item.ItemName[i] = name[i];
    }
    // 2: ItemTooltipIdx
    parse_u16(take(), item.ItemTooltipIdx);
    // 3: Image2DNum
    parse_u16(take(), item.Image2DNum);
    // 4: ItemKind
    parse_u16(take(), item.ItemKind);
    // 5: BuyPrice
    parse_u32(take(), item.BuyPrice);
    // 6: SellPrice
    parse_u32(take(), item.SellPrice);
    // 7: Rarity
    parse_u32(take(), item.Rarity);
    // 8: WeaponType
    parse_u16(take(), item.WeaponType);
    // 9-12: GenGol, MinChub, CheRyuk, SimMek
    parse_u16(take(), item.GenGol);
    parse_u16(take(), item.MinChub);
    parse_u16(take(), item.CheRyuk);
    parse_u16(take(), item.SimMek);
    // 13-14: Life, Shield
    parse_u32(take(), item.Life);
    parse_u32(take(), item.Shield);
    // 15: NaeRyuk
    parse_u16(take(), item.NaeRyuk);
    // 16-20: AttrRegist 5 floats
    for (std::uint16_t i = 0; i < ITEM_ELEM_MAX; ++i) {
        parse_f32(take(), item.AttrRegist.Element[i]);
    }
    // 21-27: Limit* stat block
    parse_u16(take(), item.LimitJob);
    parse_u16(take(), item.LimitGender);
    parse_u16(take(), item.LimitLevel);
    parse_u16(take(), item.LimitGenGol);
    parse_u16(take(), item.LimitMinChub);
    parse_u16(take(), item.LimitCheRyuk);
    parse_u16(take(), item.LimitSimMek);
    // 28-37: equip combat block
    parse_u16(take(), item.ItemGrade);
    parse_u16(take(), item.RangeType);
    parse_u16(take(), item.EquipKind);
    parse_u16(take(), item.Part3DType);
    parse_u16(take(), item.Part3DModelNum);
    parse_u16(take(), item.MeleeAttackMin);
    parse_u16(take(), item.MeleeAttackMax);
    parse_u16(take(), item.RangeAttackMin);
    parse_u16(take(), item.RangeAttackMax);
    parse_u16(take(), item.CriticalPercent);
    // 38-42: AttrAttack 5 floats
    for (std::uint16_t i = 0; i < ITEM_ELEM_MAX; ++i) {
        parse_f32(take(), item.AttrAttack.Element[i]);
    }
    // 43-47: PhyDef + plus mugong block
    parse_u16(take(), item.PhyDef);
    parse_u16(take(), item.Plus_MugongIdx);
    parse_u16(take(), item.Plus_Value);
    parse_u16(take(), item.AllPlus_Kind);
    parse_u16(take(), item.AllPlus_Value);
    // 48-49: mugongbook
    parse_u16(take(), item.MugongNum);
    parse_u16(take(), item.MugongType);
    // 50-53: potion recovery block
    parse_u16(take(), item.LifeRecover);
    parse_f32(take(), item.LifeRecoverRate);
    parse_u16(take(), item.NaeRyukRecover);
    parse_f32(take(), item.NaeRyukRecoverRate);
    // 54: ItemType
    parse_u16(take(), item.ItemType);

    // 55-58 (optional, JAPAN_LOCAL only): wItemAttr, wAcquireSkillIdx1,
    // wAcquireSkillIdx2, wDeleteSkillIdx
    if (tokens.size() == 60u) {
        parse_u16(take(), item.wItemAttr);
        parse_u16(take(), item.wAcquireSkillIdx1);
        parse_u16(take(), item.wAcquireSkillIdx2);
        parse_u16(take(), item.wDeleteSkillIdx);
    }
    // 55 (56-row) or 59 (60-row): wSetItemKind
    parse_u16(take(), item.wSetItemKind);
    out = item;
    return true;
}

// ---------------------------------------------------------------------------
// load_item_list
// ---------------------------------------------------------------------------
ItemListParseResult parse_item_list_bytes(std::span<const std::uint8_t> raw) {
    ItemListParseResult result;
    try {
        if (raw.size() < kHeaderSize + 2 * kCrcSize) {
            result.error_message = "file too small for MHFile header";
            return result;
        }
        MHFileHeader hdr{};
        std::memcpy(&hdr.dwVersion, raw.data() + 0, 4);
        std::memcpy(&hdr.dwType,    raw.data() + 4, 4);
        std::memcpy(&hdr.FileSize,  raw.data() + 8, 4);
        const std::uint8_t stored_crc1 = raw[kHeaderSize];
        const std::uint8_t stored_crc2 = raw[raw.size() - kCrcSize];
        if (hdr.FileSize + kHeaderSize + 2 * kCrcSize != raw.size()) {
            std::ostringstream oss;
            oss << "header.FileSize=" << hdr.FileSize
                << " does not match file size " << raw.size();
            result.error_message = oss.str();
            return result;
        }
        std::vector<std::uint8_t> payload(raw.begin() + kHeaderSize + kCrcSize,
                                            raw.begin() + kHeaderSize + kCrcSize + hdr.FileSize);
        const std::uint8_t computed_crc = mxh::compat::detail::decode_mhfile_text_payload(
            hdr.dwType, payload);
        result.decoded_crc = computed_crc;
        result.stored_crc  = stored_crc1;
        (void)stored_crc2;
        std::string text;
        text.reserve(payload.size());
        for (std::uint8_t b : payload) {
            text.push_back(static_cast<char>(b));
        }
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
                ItemInfo it{};
                std::string err;
                if (!parse_item_row(tokens, it, err)) {
                    ++result.parse_errors;
                    if (result.error_message.empty()) {
                        result.error_message = "row " + std::to_string(result.rows_seen)
                                              + " parse error: " + err;
                    }
                    continue;
                }
                result.items.push_back(std::move(it));
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

ItemListParseResult load_item_list(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ItemListParseResult result;
        result.error_message = "cannot open file: " + path;
        return result;
    }
    const std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(input)),
                                         std::istreambuf_iterator<char>());
    return parse_item_list_bytes(raw);
}

std::vector<std::string> resolve_equipped_character_mods(
    std::span<const std::string> base_mods,
    std::span<const std::string> appearance_mods,
    std::span<const ItemInfo> item_catalog,
    std::span<const std::uint16_t> weared_item_idx) {
    std::vector<std::string> result(base_mods.begin(), base_mods.end());
    for (const auto itemIndex : weared_item_idx) {
        if (!itemIndex) continue;
        const auto item = std::find_if(item_catalog.begin(), item_catalog.end(),
            [&](const ItemInfo& value) { return value.ItemIdx == itemIndex; });
        if (item == item_catalog.end() || item->Part3DModelNum >= appearance_mods.size()) continue;
        const auto part = static_cast<std::size_t>(item->Part3DType);
        if (part < 5u && part < result.size()) result[part] = appearance_mods[item->Part3DModelNum];
        else if (part == 5u) result.push_back(appearance_mods[item->Part3DModelNum]);
    }
    return result;
}

}  // namespace mxh::game
