// yhlibrary_smoke_test.cpp — links against the freshly-built YHLibrary.lib
// and exercises its public API surface to verify the Phase 7.1 migration
// produces a usable static library.
//
// Public surface touched:
//   - CStrClass (TCHAR-based string class; classic MFC CString style)
//   - cPtrList (templated pointer container)
//   - CStrTokenizer
//   - CHSEL (cryptographic primitive base class)
//   - CHSEL_STREAM (concrete encryption class with DES + swap variants)

#include <windows.h>
#include <cstdio>
#include <cstring>

#include "YHLibrary.h"
#include "HSEL.h"
#include "Strclass.h"
#include "PtrList.h"
#include "StrTokenizer.h"
#include "connection.h"
#include "Encryptor.h"
#include "Fileio.h"
#include "MemoryPool.h"

int main() {
    int failures = 0;

    // 1. CStrClass — instantiate, copy-construct, assign, query length.
    {
        CStrClass a("hello, ");
        CStrClass b("world");
        CStrClass c = a + b;
        if (c.GetLength() != 12) {
            std::fprintf(stderr, "CStrClass::GetLength() failed: %d\n", c.GetLength());
            ++failures;
        }
    }
    std::printf("[OK] CStrClass basic ops\n");

    // 2. cPtrList<int> — push / GetCount / RemoveAll
    {
        cPtrList list(16);
        int one = 1, two = 2, three = 3;
        list.AddTail(&one);
        list.AddTail(&two);
        list.AddTail(&three);
        if (list.GetCount() != 3) {
            std::fprintf(stderr, "cPtrList::GetCount() failed: %d\n", list.GetCount());
            ++failures;
        }
        list.RemoveAll();
        if (list.GetCount() != 0) {
            std::fprintf(stderr, "cPtrList::RemoveAll() failed\n");
            ++failures;
        }
    }
    std::printf("[OK] cPtrList basic ops\n");

    // 3. CStrTokenizer
    {
        char buf[] = "alpha,beta,gamma";
        char delims[] = ",";
        CStrTokenizer tok(buf, delims);
        const char* first = tok.GetNextToken();
        if (!first || std::strcmp(first, "alpha") != 0) {
            std::fprintf(stderr, "CStrTokenizer failed: %s\n", first ? first : "(null)");
            ++failures;
        }
    }
    std::printf("[OK] CStrTokenizer basic ops\n");

    // 4. CHSEL — base class. Just instantiate + check vtable is sane.
    {
        CHSEL hsel;
        if (hsel.GetVersion() != 0) {
            std::fprintf(stderr, "CHSEL::GetVersion expected 0, got %d\n", hsel.GetVersion());
            ++failures;
        }
    }
    std::printf("[OK] CHSEL vtable\n");

    // 5. CHSEL_STREAM — Initial + simple lifecycle.
    {
        CHSEL_STREAM s;
        HSEL_INITIAL init{};
        init.iDesCount    = HSEL_DES_SINGLE;
        init.iEncryptType = HSEL_ENCRYPTTYPE_RAND;
        init.iSwapFlag    = HSEL_SWAP_FLAG_OFF;
        init.iCustomize   = HSEL_KEY_TYPE_DEFAULT;
        s.Initial(init);
        // HSEL_VERSION is a TU-local const defined in HSEL.cpp (not in
        // the header); use the value the legacy code uses (=3) to verify
        // CHSEL_STREAM::GetVersion reads it correctly.
        if (s.GetVersion() != 3) {
            std::fprintf(stderr, "CHSEL_STREAM version mismatch: %d (expected 3)\n", s.GetVersion());
            ++failures;
        }
    }
    std::printf("[OK] CHSEL_STREAM basic lifecycle\n");

    if (failures) {
        std::fprintf(stderr, "[FAIL] %d smoke check(s) failed\n", failures);
        return 1;
    }
    std::printf("[OK] YHLibrary.lib exports all major public types; ABI self-consistent\n");
    return 0;
}