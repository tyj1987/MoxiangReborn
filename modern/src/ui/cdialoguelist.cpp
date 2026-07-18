// cdialoguelist.cpp — modern port implementation.
//
// 1:1 port of legacy `cDialogueList` from
//   `墨香【源码】\[Client]MH\cDialogueList.cpp` (1.5 KB legacy).
//
// Modern-port notes
// =================
//
// 1. **cPtrList → std::vector.** Legacy uses
//    `cPtrList<DIALOGUE> m_Dialogue[12800]`. Modern uses
//    `std::vector<std::vector<DIALOGUE>>` of the same size 12800.
//    Iteration order is preserved (head-to-tail).
//
// 2. **CMHFile is engine-side.** The file loading is stubbed
//    to no-op. LoadDialogueListFile / LoadDialogueList are
//    no-ops; AddLine is the data-side primitive that the
//    engine-binder layer (Phase 14+) uses to populate the
//    dialogues from real data.
//
// 3. **Korean DBCS detection is stubbed.** The legacy uses
//    IsDBCSLeadByte + CFONT_OBJ->GetTextExtentEx to detect
//    2-byte characters + wrap on width. Modern port's
//    ParsingLine is a simplified no-op (the engine-binder
//    layer provides the real parser when the help-dialog
//    content is loaded from a real file).
//
// 4. **DIALOGUE is opaque stub.** Engine-side struct; modern
//    uses a placeholder struct with the same field names.

#include "cdialoguelist.hpp"

#include <cstring>

namespace mxh::ui {

cDialogueList::cDialogueList() {
    m_dwDefaultColor = NORMAL_COLOR_DEFAULT;
    m_dwStressColor  = STRESS_COLOR_DEFAULT;
    m_dialogues.resize(MAX_DIALOGUE_COUNT);
}

cDialogueList::~cDialogueList() {
    // 1:1 with legacy: DIALOGUE entries are heap-allocated
    // (legacy) or owned by the vector (modern); the vector
    // auto-cleans. Modern port doesn't need explicit delete.
}

void cDialogueList::AddLine(std::uint32_t dwId, const char* str,
                            std::uint32_t color, std::uint16_t wLine,
                            std::uint16_t wType) {
    if (dwId >= m_dialogues.size()) return;
    DIALOGUE p{};
    p.Init();
    p.dwColor = color;
    if (str) {
        std::strncpy(p.str, str, sizeof(p.str) - 1);
        p.str[sizeof(p.str) - 1] = 0;
    }
    p.wLine = wLine;
    p.wType = wType;
    m_dialogues[dwId].push_back(p);
}

DIALOGUE* cDialogueList::GetDialogue(std::uint32_t dwMsgId, std::uint16_t wLine) {
    if (dwMsgId >= m_dialogues.size()) return nullptr;
    const auto& list = m_dialogues[dwMsgId];
    if (wLine >= list.size()) return nullptr;
    // 1:1 quirk: legacy returns a non-const pointer (the
    // cPtrList::GetAt returns a non-const void*). Modern
    // returns a non-const DIALOGUE* by const_casting the
    // vector storage.
    return const_cast<DIALOGUE*>(&list[wLine]);
}

} // namespace mxh::ui
