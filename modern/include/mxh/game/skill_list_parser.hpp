// mxh/game/skill_list_parser.hpp - Phase D1.3
//
// 1:1 parser for `Resource/SkillList.bin`, the packed-text skill
// table from the legacy 墨香 client.  1:1 with the legacy parser
// in `墨香【源码】\[CC]Skill\skillinfo.cpp::InitSkillInfo()` and the
// MHFile packed-file format in
// `墨香【源码】\[Client]MH\MHFile.cpp::OpenBin()` / `CheckCRC()`.
//
// File layout (all little-endian):
//   struct Header { uint32_t dwVersion; uint32_t dwType; uint32_t FileSize; };
//   Header  (12 bytes)
//   uint8_t crc1   (1 byte)
//   uint8_t data[FileSize]
//   uint8_t crc2   (1 byte)
//
// Decoding (CMHFile::CheckCRC, lines 188-197 of MHFile.cpp):
//   char crc = (char) m_Header.dwType;
//   for (DWORD i = 0; i < FileSize; ++i) {
//       crc += m_pData[i];
//       m_pData[i] -= (char) i;
//       if (i % m_Header.dwType == 0)
//           m_pData[i] -= (char) m_Header.dwType;
//   }
//   if (m_crc1 != crc) return FALSE;     // commented out in the legacy
//                                         // client, so we don't fail on
//                                         // CRC mismatch either.
//
// After decoding, the data is plain text in EUC-KR (cp949):
//   - one row per skill
//   - tokens separated by space/tab
//   - rows separated by CRLF
//   - first token of each row is SkillIdx
//   - token order matches skillinfo.cpp::InitSkillInfo exactly
//
// Token count per row (after the first 33 fixed fields, before
// the 6 AdditiveAttr segments + tail): see the table in
// skill_list_parser.cpp::kTokensPerRow.

#pragma once

#include "mxh/game/skill_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::game {

// Result of a parse attempt.
struct SkillListParseResult {
    std::vector<SkillInfo>  skills;   // one entry per non-empty row
    std::uint32_t           rows_seen = 0;   // total rows (including parse errors)
    std::uint32_t           parse_errors = 0; // rows that didn't tokenize
    std::uint8_t            decoded_crc = 0;  // for verification
    std::uint8_t            stored_crc = 0;   // for verification
    std::string             error_message;    // empty on success
};

// Decode the MHFile packed-text data section in-place.  The buffer
// passed in is the raw FileSize-byte payload (between the crc1 and
// crc2 bytes).  After this call, the buffer contains the decoded
// EUC-KR text.  Returns the computed CRC byte (== m_crc1 in a valid
// file, but the legacy client doesn't enforce this so we don't
// either; we just report it via SkillListParseResult).
//
// 1:1 with CMHFile::CheckCRC() lines 188-197.
std::uint8_t decode_mhfile_payload(std::uint32_t dwType,
                                   std::vector<std::uint8_t>& payload);

// Read the SkillList.bin file, decode it, and parse every row into
// a SkillInfo.  Throws std::runtime_error on I/O failure (file not
// found, size too small, header missing).  Parse-level errors (bad
// token count, out-of-range field values) are accumulated in the
// returned result's parse_errors counter and do not throw.
SkillListParseResult load_skill_list(const std::string& path);

// Tokenize one decoded line (CRLF already stripped) into the
// SkillInfo.  Returns false and leaves `out` untouched on parse
// failure; sets parse_error_msg on the first malformed row.
bool parse_skill_row(const std::vector<std::string>& tokens,
                     SkillInfo& out,
                     std::string& parse_error_msg);

}  // namespace mxh::game
