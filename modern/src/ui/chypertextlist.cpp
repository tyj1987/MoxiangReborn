// chypertextlist.cpp — see chypertextlist.hpp for the full port
// description.
//
// Modern-port simplifications (the .hpp has the high-level
// notes; this file documents the per-method trade-offs):
//
// 1. `cHyperTextList()` calls `m_hyperText.reserve(1000)` to
//    match the legacy `m_HyperText.Initialize(1000)` bucket
//    count. std::unordered_map doesn't strictly need a
//    pre-allocation, but reserve() matches the legacy intent
//    and avoids bucket re-hashing on small inputs.
//
// 2. `AddEntry` uses `make_unique<DIALOGUE>()` + `Init()` to
//    match the legacy `pTemp = new DIALOGUE; pTemp->Init();`.
//    The string copy is bounded by `sizeof(entry->str) - 1`
//    to keep the NUL terminator intact (the legacy uses
//    strcpy which is unbounded but trusted the file parser
//    to deliver ≤ 256-byte lines; modern strncpy is the
//    safer 1:1).
//
// 3. `GetHyperText` returns a raw `DIALOGUE*` (not a unique_ptr)
//    to match the legacy signature 1:1. The pointer is
//    owned by the cHyperTextList; callers must not free.
//
// 4. `RemoveAll` calls `m_hyperText.clear()`. unique_ptr
//    destructors free each DIALOGUE automatically. This
//    replaces the legacy `SetPositionHead / GetData / delete`
//    loop + `m_HyperText.RemoveAll()`.

#include "chypertextlist.hpp"

#include <cstring>
#include <utility>

namespace mxh::ui {

cHyperTextList::cHyperTextList() {
    // Legacy ctor: m_HyperText.Initialize(1000) — pre-allocate
    // 1000 hash buckets. Modern std::unordered_map reserves
    // buckets lazily; reserve(1000) hints the same intent.
    m_hyperText.reserve(1000);
}

cHyperTextList::~cHyperTextList() = default;

void cHyperTextList::AddEntry(std::uint32_t dwIdx, const char* str) {
    if (str == nullptr) {
        return;
    }
    auto entry = std::make_unique<DIALOGUE>();
    entry->Init();
    // 1:1 with legacy strcpy(pTemp->str, buff); but bounded
    // by the DIALOGUE::str buffer size to avoid overflow.
    std::strncpy(entry->str, str, sizeof(entry->str) - 1);
    entry->str[sizeof(entry->str) - 1] = '\0';
    // 1:1 with legacy m_HyperText.Add(pTemp, idx) — overwrites
    // any existing entry at the same idx.
    m_hyperText[dwIdx] = std::move(entry);
}

DIALOGUE* cHyperTextList::GetHyperText(std::uint32_t dwIdx) noexcept {
    auto it = m_hyperText.find(dwIdx);
    if (it == m_hyperText.end()) {
        return nullptr;
    }
    return it->second.get();
}

void cHyperTextList::RemoveAll() noexcept {
    m_hyperText.clear();
}

} // namespace mxh::ui
