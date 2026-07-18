// cdialoguelist.hpp — modern port of 墨香 cDialogueList (NPC
// dialogue data: a per-msg-id list of DIALOGUE entries loaded
// from a text file).
//
// 1:1 port of legacy `cDialogueList` from
//   `墨香【源码】\[Client]MH\cDialogueList.{h,cpp}` (1.5 KB legacy
//   code; modern port preserves the data model + state machine
//   + parsing semantics and stubs the engine-side CMHFile + Korean
//   DBCS detection with no-ops until Phase 13+ real impl lands).
//
// Modern port scope (this commit):
//   - m_dwDefaultColor + m_dwStressColor (the legacy 1:1
//     colors used by the parser).
//   - m_dialogues[12800]: std::vector<std::vector<DIALOGUE>>
//     storage (replaces the legacy cPtrList<DIALOGUE>[12800]).
//   - 1:1 surface: LoadDialogueListFile / LoadDialogueList /
//     ParsingLine / AddLine / GetDialogue.
//   - DIALOGUE struct stub (1:1 with legacy fields).
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. **cPtrList → std::vector.** Legacy uses
//    `cPtrList<DIALOGUE> m_Dialogue[12800]` (12800 message ids
//    per file). Modern uses `std::vector<std::vector<DIALOGUE>>`
//    of the same size 12800.
// 2. **CMHFile is engine-side.** The file loading is stubbed
//    to no-op (the modern port doesn't have CMHFile yet).
//    LoadDialogueListFile is a no-op; AddLine is the data-side
//    primitive that the engine-binder layer (Phase 14+) uses
//    to populate the dialogues from real data.
// 3. **Korean DBCS detection is stubbed.** The legacy uses
//    IsDBCSLeadByte to detect 2-byte characters. Modern port's
//    ParsingLine is a simplified no-op (the engine-binder layer
//    provides the real parser when the help-dialog content is
//    loaded from a real file).
// 4. **DIALOGUE is opaque stub.** Engine-side struct with
//    dwColor / str[1024] / wLine / wType. Modern uses a
//    placeholder struct with the same field names.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::ui {

// 1:1 with legacy MAX_DIALOGUE_COUNT.
static constexpr std::uint32_t MAX_DIALOGUE_COUNT = 12800;

// DIALOGUE is the engine-side dialogue payload. Modern port uses
// a placeholder struct with the same field names; the engine-
// binder layer (Phase 14+) provides the real type when the
// help-dialog content is loaded.
struct DIALOGUE {
    std::uint32_t dwColor = 0xFFFFFFFFu;
    char          str[1024] = {};
    std::uint16_t wLine = 0;
    std::uint16_t wType = 0;  // emLink_Null = 0, emLink = 1, etc.

    void Init() noexcept {
        dwColor = 0xFFFFFFFFu;
        std::int32_t i = 0;
        for (char& c : str) { c = 0; if (++i > 1024) break; }
        wLine = 0;
        wType = 0;
    }
};

// 1:1 with legacy color macros.
static constexpr std::uint32_t STRESS_COLOR_DEFAULT = 0xFFFFFF00u;  // RGBA yellow
static constexpr std::uint32_t NORMAL_COLOR_DEFAULT = 0xFFFFFFFFu;  // RGBA white

class cDialogueList {
public:
    cDialogueList();
    ~cDialogueList();

    cDialogueList(const cDialogueList&) = delete;
    cDialogueList& operator=(const cDialogueList&) = delete;

    // -------------------------------------------------------------------------
    // LoadDialogueListFile: 1:1 with legacy. Parses a .bin/.txt
    // file and populates the dialogues. Engine-side stubbed
    // (no CMHFile in modern); the modern port is a no-op for
    // file loading. The engine-binder layer (Phase 14+) is
    // responsible for the real loader.
    // -------------------------------------------------------------------------
    void LoadDialogueListFile(const char* /*filePath*/,
                              const char* /*mode*/ = "rt") {
        // No-op in modern; CMHFile is engine-side.
    }

    // LoadDialogueList: 1:1 with legacy. Reads the body of a
    // single #Msg block. Engine-side stubbed.
    void LoadDialogueList(std::uint32_t /*dwId*/, void* /*fp*/) {
        // No-op in modern.
    }

    // ParsingLine: 1:1 with legacy. Parses a single line.
    // The legacy uses Korean DBCS detection (IsDBCSLeadByte)
    // to handle 2-byte characters. Modern port's
    // implementation is a simplified no-op (the engine-binder
    // layer provides the real parser when the help-dialog
    // content is loaded from a real file).
    void ParsingLine(std::uint32_t /*dwId*/, const char* /*buf*/) {
        // No-op in modern.
    }

    // AddLine: append a DIALOGUE entry to m_dialogues[dwId]
    // at position wLine. 1:1 with legacy (the legacy uses
    // cPtrList::AddTail).
    void AddLine(std::uint32_t dwId, const char* str,
                 std::uint32_t color, std::uint16_t wLine,
                 std::uint16_t wType);

    // GetDialogue: 1:1 with legacy. Returns the DIALOGUE at
    // (dwMsgId, wLine), or nullptr if the line is out of range.
    DIALOGUE* GetDialogue(std::uint32_t dwMsgId, std::uint16_t wLine);

    // Color accessors.
    void          SetDefaultColor(std::uint32_t c) noexcept { m_dwDefaultColor = c; }
    std::uint32_t GetDefaultColor() const noexcept          { return m_dwDefaultColor; }
    void          SetStressColor(std::uint32_t c) noexcept { m_dwStressColor = c; }
    std::uint32_t GetStressColor() const noexcept           { return m_dwStressColor; }

    // Test-only: get the storage.
    const std::vector<std::vector<DIALOGUE>>& GetDialoguesForTesting() const noexcept {
        return m_dialogues;
    }

private:
    std::uint32_t m_dwDefaultColor = NORMAL_COLOR_DEFAULT;
    std::uint32_t m_dwStressColor  = STRESS_COLOR_DEFAULT;

    // m_Dialogue[12800] (legacy) → m_dialogues (modern).
    // Each entry is a list of DIALOGUE entries for one msg id.
    std::vector<std::vector<DIALOGUE>> m_dialogues;
};

} // namespace mxh::ui
