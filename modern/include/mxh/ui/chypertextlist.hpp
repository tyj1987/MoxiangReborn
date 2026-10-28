// chypertextlist.hpp — modern port of 墨香 cHyperTextList
// (a single-level hashmap of DIALOGUE entries indexed by a
// DWORD id; used by the help dialog to resolve hyperlink
// target text from a `cHyperTextList` lookup).
//
// 1:1 port of legacy `cHyperTextList` from
//   `墨香【源码】\[Client]MH\cHyperTextList.{h,cpp}` (≈700 B legacy
//   code; modern port preserves the data model + lookup API
//   and stubs the engine-side CMHFile with a no-op until
//   Phase 13+ real impl lands).
//
// Modern port scope (this commit):
//   - m_hyperText: std::unordered_map<std::uint32_t, std::unique_ptr<DIALOGUE>>.
//   - 1:1 surface: LoadHyperTextFormFile / GetHyperText /
//     AddEntry / RemoveAll / GetCount.
//   - DIALOGUE: re-used from cdialoguelist.hpp (same 1:1 struct).
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. **CYHHashTable → std::unordered_map.** Legacy uses
//    `CYHHashTable<DIALOGUE> m_HyperText` (engine-side hash
//    table). Modern uses
//    `std::unordered_map<std::uint32_t, std::unique_ptr<DIALOGUE>>`.
// 2. **CMHFile is engine-side.** The file loading is stubbed
//    to no-op (the modern port doesn't have CMHFile yet).
//    LoadHyperTextFormFile is a no-op; AddEntry is the data-side
//    primitive that the engine-binder layer (Phase 14+) uses
//    to populate the entries from real data.
// 3. **Manual delete → unique_ptr.** Legacy ctor/dtor manually
//    iterates the hash table and `delete` each DIALOGUE*.
//    Modern uses std::unique_ptr so the dtor is implicit.
// 4. **DIALOGUE is opaque stub.** Reused from cdialoguelist.hpp
//    (the engine-binder layer Phase 14+ provides the real type
//    when the help-dialog content is loaded from a real file).

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "cdialoguelist.hpp"

namespace mxh::ui {

class cHyperTextList {
public:
    cHyperTextList();
    ~cHyperTextList();

    cHyperTextList(const cHyperTextList&) = delete;
    cHyperTextList& operator=(const cHyperTextList&) = delete;

    // -------------------------------------------------------------------------
    // LoadHyperTextFormFile: 1:1 with legacy. Parses a
    // .bin/.txt file and populates the hyperlink entries.
    // Engine-side stubbed (no CMHFile in modern); the modern
    // port is a no-op for file loading. The engine-binder
    // layer (Phase 14+) is responsible for the real loader.
    // -------------------------------------------------------------------------
    void LoadHyperTextFormFile(const char* /*filePath*/,
                               const char* /*mode*/ = "rt") {
        // No-op in modern; CMHFile is engine-side.
    }

    // -------------------------------------------------------------------------
    // AddEntry: append a DIALOGUE entry at index dwIdx. The
    // legacy deep-copies the DIALOGUE into a heap-allocated
    // pointer; modern uses unique_ptr (no manual delete).
    // 1:1 with legacy m_HyperText.Add(pTemp, idx) — overwrites
    // any existing entry at the same idx.
    // -------------------------------------------------------------------------
    void AddEntry(std::uint32_t dwIdx, const char* str);

    // GetHyperText: 1:1 with legacy. Returns the DIALOGUE*
    // at dwIdx, or nullptr if not present. The returned
    // pointer is owned by the cHyperTextList; do not free.
    DIALOGUE* GetHyperText(std::uint32_t dwIdx) noexcept;

    // RemoveAll: clear all entries.
    void RemoveAll() noexcept;

    // Count accessors.
    std::size_t GetCount() const noexcept { return m_hyperText.size(); }

    // Test-only: get the storage.
    const std::unordered_map<std::uint32_t, std::unique_ptr<DIALOGUE>>&
    GetEntriesForTesting() const noexcept { return m_hyperText; }

private:
    // m_HyperText (legacy) → m_hyperText (modern).
    // Storage: std::unordered_map<std::uint32_t, std::unique_ptr<DIALOGUE>>.
    // 1000 is the legacy Initialize(1000) bucket count; modern
    // std::unordered_map reserves buckets lazily; reserve(1000)
    // hints the same intent.
    std::unordered_map<std::uint32_t, std::unique_ptr<DIALOGUE>> m_hyperText;
};

} // namespace mxh::ui
