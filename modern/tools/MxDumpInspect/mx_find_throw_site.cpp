// mx_find_throw_site: DIA SDK RVA -> file:line lookup.
//
// Uses msdia140.dll (DIA SDK) to load a PDB and resolve an RVA to
// the source file and line number. Used to map the GameIn C++ throw
// site (RVA 0x16A2B4 in mxh_client.exe .text) back to modern/src/.
// Intentionally minimal: COM init, loadDataFromPdb, openSession,
// findLinesByAddr, print. Raw BSTR + SysFreeString, manual
// AddRef/Release, no ATL. Linked against diaguids.lib for the COM
// type-library stubs; the actual msdia140.dll is loaded at runtime
// via CoCreateInstance(CLSID_DiaSource).
//
// Plan ref: EXECUTION_PLAN.md Phase 0 §6.5 — root-cause the C++ throw
// site observed in the 459KB minidumps. The dump inspector (commit
// 22f6ef5c) only goes as far as "RVA 0x16A2B4 / module
// mxh_client.exe"; this tool is the next step to source line.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <dia2.h>
#include <diacreate.h>

#pragma comment(lib, "diaguids.lib")

template <typename T>
struct ComPtr {
    T* p_ = nullptr;
    ComPtr() = default;
    explicit ComPtr(T* p) : p_(p) {}
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    ComPtr(ComPtr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    ComPtr& operator=(ComPtr&& o) noexcept {
        if (this != &o) { reset(); p_ = o.p_; o.p_ = nullptr; }
        return *this;
    }
    ~ComPtr() { reset(); }
    void reset() { if (p_) { p_->Release(); p_ = nullptr; } }
    T** put() { reset(); return &p_; }
    T* get() const { return p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }
};

static void PrintLine(IDiaLineNumber* pLine) {
    DWORD rva = 0, line = 0, col = 0;
    if (pLine->get_relativeVirtualAddress(&rva) != S_OK) rva = 0;
    if (pLine->get_lineNumber(&line) != S_OK) line = 0;
    if (pLine->get_columnNumber(&col) != S_OK) col = 0;
    IDiaSourceFile* pSrcFile = nullptr;
    BSTR bstrName = nullptr;
    if (pLine->get_sourceFile(&pSrcFile) == S_OK && pSrcFile) {
        if (pSrcFile->get_fileName(&bstrName) != S_OK) bstrName = nullptr;
        pSrcFile->Release();
    }
    if (bstrName) {
        wprintf(L"  RVA 0x%08X  line %u  col %u  %s\n", rva, line, col, bstrName);
        SysFreeString(bstrName);
    } else {
        wprintf(L"  RVA 0x%08X  line %u  col %u  (no source file)\n", rva, line, col);
    }
}

static void PrintFunctionIfContains(IDiaSymbol* pFunc, DWORD targetRva) {
    DWORD rva = 0;
    ULONGLONG len64 = 0;
    if (pFunc->get_relativeVirtualAddress(&rva) != S_OK) rva = 0;
    if (pFunc->get_length(&len64) != S_OK) len64 = 0;
    if (rva <= targetRva && targetRva < rva + (DWORD)len64) {
        BSTR bstrName = nullptr;
        if (pFunc->get_name(&bstrName) != S_OK) bstrName = nullptr;
        if (bstrName) {
            wprintf(L"  Func RVA 0x%08X  len 0x%llX  %s\n", rva, len64, bstrName);
            SysFreeString(bstrName);
        } else {
            wprintf(L"  Func RVA 0x%08X  len 0x%llX  (no name)\n", rva, len64);
        }
    }
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        wprintf(L"Usage: mx_find_throw_site <pdb> <rva_hex>\n");
        return 1;
    }
    const wchar_t* pdb = argv[1];
    DWORD rva = 0;
    {
        wchar_t* endp = nullptr;
        rva = wcstoul(argv[2], &endp, 16);
        if (endp == argv[2] || (endp && *endp != L'\0')) {
            wprintf(L"Bad RVA: %s\n", argv[2]);
            return 2;
        }
    }

    HRESULT hrInit = CoInitialize(nullptr);
    bool comOk = SUCCEEDED(hrInit);

    ComPtr<IDiaDataSource> pSrc;
    HRESULT hr = E_FAIL;
    // msdia140.dll is often not registered on developer machines.
    // Use LoadLibrary + DllGetClassObject so we don't require regsvr32.
    HMODULE hDia = LoadLibraryW(L"C:\\BuildTools\\DIA SDK\\bin\\msdia140.dll");
    if (!hDia) {
        wprintf(L"LoadLibraryW(msdia140.dll) failed: GetLastError=%lu\n",
                GetLastError());
        if (comOk) CoUninitialize();
        return 3;
    }
    using PFNDllGetClassObject = HRESULT (WINAPI*)(REFCLSID, REFIID, LPVOID*);
    auto pfnGetClassObject = reinterpret_cast<PFNDllGetClassObject>(
        GetProcAddress(hDia, "DllGetClassObject"));
    if (!pfnGetClassObject) {
        wprintf(L"GetProcAddress(DllGetClassObject) failed\n");
        FreeLibrary(hDia);
        if (comOk) CoUninitialize();
        return 3;
    }
    IClassFactory* pFactory = nullptr;
    hr = pfnGetClassObject(CLSID_DiaSource, IID_IClassFactory,
                           reinterpret_cast<void**>(&pFactory));
    if (FAILED(hr) || !pFactory) {
        wprintf(L"DllGetClassObject failed: 0x%08lX\n", hr);
        if (hDia) FreeLibrary(hDia);
        if (comOk) CoUninitialize();
        return 3;
    }
    hr = pFactory->CreateInstance(nullptr, IID_PPV_ARGS(pSrc.put()));
    pFactory->Release();
    if (FAILED(hr)) {
        wprintf(L"CreateInstance(IDiaDataSource) failed: 0x%08lX\n", hr);
        if (hDia) FreeLibrary(hDia);
        if (comOk) CoUninitialize();
        return 3;
    }
    // hDia stays loaded for the lifetime of pSrc; COM holds the
    // reference implicitly via the running object table.  Released by
    // the process on exit — this tool exits immediately after use.

    hr = pSrc->loadDataFromPdb(pdb);
    if (FAILED(hr)) {
        wprintf(L"loadDataFromPdb(%s) failed: 0x%08lX\n", pdb, hr);
        if (comOk) CoUninitialize();
        return 4;
    }

    ComPtr<IDiaSession> pSession;
    hr = pSrc->openSession(pSession.put());
    if (FAILED(hr)) {
        wprintf(L"openSession failed: 0x%08lX\n", hr);
        if (comOk) CoUninitialize();
        return 5;
    }

    wprintf(L"PDB: %s  RVA: 0x%08X\n\n", pdb, rva);

    wprintf(L"[LineNumber hits]\n");
    ComPtr<IDiaEnumLineNumbers> pLines;
    hr = pSession->findLinesByRVA(rva, 1, pLines.put());
    if (FAILED(hr) || !pLines) {
        wprintf(L"  findLinesByRVA failed hr=0x%08lX (PDB may lack /Zi line info)\n", hr);
    } else {
        IDiaLineNumber* pLine = nullptr;
        ULONG fetched = 0;
        int found = 0;
        while (SUCCEEDED(pLines->Next(1, &pLine, &fetched)) && fetched == 1 && pLine) {
            PrintLine(pLine);
            pLine->Release();
            pLine = nullptr;
            found++;
        }
        if (!found) wprintf(L"  (no lines matched RVA 0x%08X)\n", rva);
    }

    wprintf(L"\n[Containing function]\n");
    ComPtr<IDiaSymbol> pGlobal;
    if (SUCCEEDED(pSession->get_globalScope(pGlobal.put()))) {
        ComPtr<IDiaEnumSymbols> pEnums;
        if (SUCCEEDED(pGlobal->findChildren(SymTagFunction, nullptr, nsNone, pEnums.put()))) {
            IDiaSymbol* pSym = nullptr;
            ULONG fetched = 0;
            while (SUCCEEDED(pEnums->Next(1, &pSym, &fetched)) && fetched == 1 && pSym) {
                PrintFunctionIfContains(pSym, rva);
                pSym->Release();
                pSym = nullptr;
            }
        }
    }

    if (comOk) CoUninitialize();
    return 0;
}
