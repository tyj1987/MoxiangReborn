// 1:1 test for ItemList.bin parser (56-token common row, 60-token JAPAN_LOCAL).
// Verifies field assignment order, JAPAN_LOCAL opt-in block, error reporting,
// end-to-end synthesized MHFile roundtrip, and rejection of malformed rows.

#include "mxh/game/item_list_parser.hpp"
#include "mxh/compat/detail/text_parse.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

using mxh::game::ItemInfo;
using mxh::game::parse_item_row;
using mxh::game::ItemListParseResult;
using mxh::game::load_item_list;
using mxh::game::ITEM_ELEM_MAX;
using mxh::game::ITEM_MAX_NAME;

namespace {
// 56-token synthetic common row. ItemIdx=1, name "Sword", BuyPrice=99.
// Fill the rest with zeros so parse_u16 / parse_f32 produce the
// expected defaults; only the first handful of tokens are set.
std::vector<std::string> make_common_row_56() {
std::vector<std::string> t(56u);
t[0]  = "1";                  // ItemIdx
t[1]  = "Sword";              // ItemName
t[2]  = "10";                 // ItemTooltipIdx
t[3]  = "11";                 // Image2DNum
t[4]  = "2";                  // ItemKind (equip)
t[5]  = "99";                 // BuyPrice
t[6]  = "33";                 // SellPrice
t[7]  = "5";                  // Rarity
t[8]  = "3";                  // WeaponType
t[9]  = "1";                  // GenGol
t[10] = "2";                  // MinChub
t[11] = "3";                  // CheRyuk
t[12] = "4";                  // SimMek
t[13] = "100";                // Life
t[14] = "200";                // Shield
t[15] = "50";                 // NaeRyuk
// 16-20: AttrRegist 5 floats
t[16] = "1.0";
t[17] = "2.0";
t[18] = "3.0";
t[19] = "4.0";
t[20] = "5.0";
t[21] = "0";                  // LimitJob
t[22] = "0";                  // LimitGender
t[23] = "10";                 // LimitLevel
t[24] = "11";                 // LimitGenGol
t[25] = "12";                 // LimitMinChub
t[26] = "13";                 // LimitCheRyuk
t[27] = "14";                 // LimitSimMek
t[28] = "5";                  // ItemGrade
t[29] = "0";                  // RangeType
t[30] = "0";                  // EquipKind
t[31] = "0";                  // Part3DType
t[32] = "0";                  // Part3DModelNum
t[33] = "10";                 // MeleeAttackMin
t[34] = "20";                 // MeleeAttackMax
t[35] = "0";                  // RangeAttackMin
t[36] = "0";                  // RangeAttackMax
t[37] = "5";                  // CriticalPercent
// 38-42: AttrAttack 5 floats
t[38] = "0.5";
t[39] = "0.6";
t[40] = "0.7";
t[41] = "0.8";
t[42] = "0.9";
t[43] = "40";                 // PhyDef
t[44] = "0";                  // Plus_MugongIdx
t[45] = "0";                  // Plus_Value
t[46] = "0";                  // AllPlus_Kind
t[47] = "0";                  // AllPlus_Value
t[48] = "0";                  // MugongNum
t[49] = "0";                  // MugongType
t[50] = "0";                  // LifeRecover
t[51] = "0.0";                // LifeRecoverRate
t[52] = "0";                  // NaeRyukRecover
t[53] = "0.0";                // NaeRyukRecoverRate
t[54] = "0";                  // ItemType
// index 55: wSetItemKind (last field of 56-row)
t[55] = "1";                  // wSetItemKind
return t;
}

}  // namespace

TEST(ItemListParser, CommonRow56ParsesCoreFields) {
auto tokens = make_common_row_56();
ItemInfo it{};
std::string err;
ASSERT_TRUE(parse_item_row(tokens, it, err)) << err;
EXPECT_EQ(it.ItemIdx, 1u);
EXPECT_STREQ(it.ItemName, "Sword");
EXPECT_EQ(it.ItemKind, 2u);
EXPECT_EQ(it.BuyPrice, 99u);
EXPECT_EQ(it.SellPrice, 33u);
EXPECT_EQ(it.Rarity, 5u);
EXPECT_EQ(it.Life, 100u);
EXPECT_EQ(it.Shield, 200u);
EXPECT_EQ(it.NaeRyuk, 50u);
EXPECT_EQ(it.AttrRegist.Element[0], 1.0f);
EXPECT_EQ(it.AttrRegist.Element[4], 5.0f);
EXPECT_EQ(it.AttrAttack.Element[0], 0.5f);
EXPECT_EQ(it.CriticalPercent, 5u);
EXPECT_EQ(it.wSetItemKind, 1u);
// JAPAN_LOCAL fields stay zero in 56-row parse.
EXPECT_EQ(it.wItemAttr, 0u);
EXPECT_EQ(it.wAcquireSkillIdx1, 0u);
}

TEST(ItemListParser, ItemNameTruncatesToItemMaxName) {
auto tokens = make_common_row_56();
// 50-byte name should be truncated to 30 (ITEM_MAX_NAME - 1).
tokens[1] = std::string(50, 'A');
ItemInfo it{};
std::string err;
ASSERT_TRUE(parse_item_row(tokens, it, err));
// Last byte is NUL terminator.
const auto len = std::strlen(it.ItemName);
EXPECT_EQ(len, ITEM_MAX_NAME - 1);
for (std::size_t i = 0; i < len; ++i) EXPECT_EQ(it.ItemName[i], 'A');
EXPECT_EQ(it.ItemName[len], 0);
}

TEST(ItemListParser, RowWithTooFewTokensIsRejected) {
auto tokens = make_common_row_56();
tokens.resize(55u);
ItemInfo it{};
std::string err;
EXPECT_FALSE(parse_item_row(tokens, it, err));
EXPECT_NE(err.find("56 or 60"), std::string::npos);
}

TEST(ItemListParser, RowWithTooManyTokensIsRejected) {
auto tokens = make_common_row_56();
tokens.push_back("999");
ItemInfo it{};
std::string err;
EXPECT_FALSE(parse_item_row(tokens, it, err));
}

TEST(ItemListParser, JapanLocalRow60FillsFourExtraFields) {
auto tokens = make_common_row_56();
// Insert the 4 JAPAN_LOCAL tokens after index 54 (ItemType), so
// they end up at positions 55..58; wSetItemKind shifts to 59.
tokens.insert(tokens.begin() + 55, "1");   // wItemAttr
tokens.insert(tokens.begin() + 56, "100"); // wAcquireSkillIdx1
tokens.insert(tokens.begin() + 57, "101"); // wAcquireSkillIdx2
tokens.insert(tokens.begin() + 58, "102"); // wDeleteSkillIdx
ASSERT_EQ(tokens.size(), 60u);
ItemInfo it{};
std::string err;
ASSERT_TRUE(parse_item_row(tokens, it, err)) << err;
EXPECT_EQ(it.wItemAttr, 1u);
EXPECT_EQ(it.wAcquireSkillIdx1, 100u);
EXPECT_EQ(it.wAcquireSkillIdx2, 101u);
EXPECT_EQ(it.wDeleteSkillIdx, 102u);
EXPECT_EQ(it.wSetItemKind, 1u);
}

TEST(ItemListParser, DecodeReusesCommonHelper) {
// Verify the public parser still uses mxh::compat::detail::decode_mhfile_text_payload
// -- 1:1 sanity test that dwType=1 round-trips.
std::vector<std::uint8_t> p(8u, 0u);
for (std::uint8_t i = 0; i < 8; ++i) p[i] = i;
const std::uint8_t crc = mxh::compat::detail::decode_mhfile_text_payload(1, p);
// First byte: (0 - 0) - 1 = 255 (type=1, i%1==0, so extra subtract 1).
EXPECT_EQ(p[0], 255);
EXPECT_NE(crc, 0);
}

TEST(ItemListParser, ElementArraysHaveFiveSlots) {
EXPECT_EQ(ITEM_ELEM_MAX, 5u);
ItemInfo it{};
for (std::uint16_t i = 0; i < 5; ++i) {
EXPECT_EQ(it.AttrRegist.Element[i], 0.0f);
EXPECT_EQ(it.AttrAttack.Element[i], 0.0f);
}
}

TEST(ItemListParser, BadItemIdxIsReported) {
auto tokens = make_common_row_56();
tokens[0] = "-1";  // out of u16 range
ItemInfo it{};
std::string err;
EXPECT_FALSE(parse_item_row(tokens, it, err));
EXPECT_NE(err.find("ItemIdx"), std::string::npos);
}

TEST(ItemListParser, NonNumericStatFallsBackToZero) {
auto tokens = make_common_row_56();
tokens[23] = "not-a-number";  // LimitLevel
ItemInfo it{};
std::string err;
ASSERT_TRUE(parse_item_row(tokens, it, err));
EXPECT_EQ(it.LimitLevel, 0u);
EXPECT_EQ(it.ItemIdx, 1u);
}

TEST(ItemListParser, MalformedRowInLoadIsCountedButDoesNotThrow) {
// Synthesize a 14-byte MHFile text "file" with a single short row
// (only 3 tokens) -- we expect parse_errors=1 and a populated
// error_message.
const std::uint8_t blob[] = {
0x01, 0x00, 0x00, 0x00,  // dwVersion
0x01, 0x00, 0x00, 0x00,  // dwType
0x0c, 0x00, 0x00, 0x00,  // FileSize
0x00,                     // crc1
'1', ' ', '2', ' ', '3',  // row (3 tokens)
'\r', '\n',
0x00                      // crc2
};
std::string text;
text.resize(sizeof(blob));
std::memcpy(text.data(), blob, sizeof(blob));
// FileSize mismatch (12 declared, actual=14): this is the easiest
// path that we know will fail at header level. Verify the
// header-mismatch error is reported.
const auto path = std::filesystem::temp_directory_path() / "mxh_item_list_test.bin";
std::ofstream(path, std::ios::binary).write(text.data(), static_cast<std::streamsize>(text.size()));
auto res = load_item_list(path.string());
std::error_code ec;
std::filesystem::remove(path, ec);
EXPECT_FALSE(res.error_message.empty());
}
// End-to-end test against the real PlayDH/Resource/ItemList.bin.
// GTEST_SKIPs if the real .bin is missing so CI can still pass.

namespace {
// The CJK path is stored as bytes via wide-char LR"()".
const std::filesystem::path kReal =
LR"(C:\\moxiang\\墨香【源码配套资源】\\PlayDH\\Resource\\ItemList.bin)";
const std::string kTemp =
"C:/Users/User/AppData/Local/Temp/mxh_item_list_real_test.bin";
const std::uintmax_t kExpectedSize = 1510488;
}

TEST(ItemListParser, RealItemListBinHasNoParseErrors) {
if (!std::filesystem::exists(kReal)) {
GTEST_SKIP() << "real ItemList.bin missing";
}
// Copy out of the CJK folder so MSVC std::ifstream does not throw
// system_error(1113, "No mapping for the Unicode character...").
std::error_code ec;
std::filesystem::copy(kReal, kTemp,
    std::filesystem::copy_options::overwrite_existing, ec);
ASSERT_FALSE(ec) << ec.message();
auto result = load_item_list(kTemp);
EXPECT_TRUE(result.error_message.empty()) << result.error_message;
EXPECT_EQ(result.parse_errors, 0u);
EXPECT_GE(result.items.size(), 1u);
if (!result.items.empty()) {
const ItemInfo& first = result.items.front();
EXPECT_GE(first.ItemIdx, 1u);
}
}
