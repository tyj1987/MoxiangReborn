// stub.cpp - Minimal COM DLL that implements IMHTConsole and I4DyuchiCONSOLE interfaces
// This allows the game servers to start without the original console DLL.
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

// Logging helper
static void LogMsg(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);
    buf[sizeof(buf) - 1] = 0;
    FILE* f = fopen("C:\\Windows\\Temp\\ConsoleStub.log", "a");
    if (f) {
        fprintf(f, "[%lu] %s\n", GetTickCount(), buf);
        fclose(f);
    }
}

// GUIDs from the game source
// CLSID_ULTRA_TCONSOLE: {4845A47E-F4E1-45cd-8561-C7B947BBA936}
static const GUID CLSID_MyConsole =
    {0x4845a47e, 0xf4e1, 0x45cd, {0x85, 0x61, 0xc7, 0xb9, 0x47, 0xbb, 0xa9, 0x36}};

// IID_ITConsole: {152D6C7D-D376-47c1-957A-4BA3730ACFD2}
static const GUID IID_MyITConsole =
    {0x152d6c7d, 0xd376, 0x47c1, {0x95, 0x7a, 0x4b, 0xa3, 0x73, 0x0a, 0xcf, 0xd2}};

// MENU_CUSTOM_INFO structure (from TConsole_Def.h) — NO pack pragma
// (original TConsole_Def.h does NOT use #pragma pack)
struct MENU_CUSTOM_INFO {
    char szMenuName[16];
    void (*cbMenuCommand)();
};

// MHTCONSOLE_DESC structure (from TConsole_Def.h) — NO pack pragma
struct MHTCONSOLE_DESC {
    char* szConsoleName;
    DWORD dwRefreshRate;
    WORD wLogFileType;
    char* szLogFileName;
    DWORD dwFlushFileBufferSize;
    WORD wMaxLineNum;
    DWORD dwListStyle;
    DWORD dwDrawTimeOut;
    int Width;
    int Height;
    int nCustomMunuNum;
    MENU_CUSTOM_INFO* pCustomMenu;
    void* pFont; // LOGFONT*
    void (*cbReturnFunc)(char*);
};

// Stub COM object implementing IMHTConsole (IDispatch-like with IUnknown base)
class CConsoleStub : public IUnknown {
public:
    // IMHTConsole vtable methods (in vtable order after IUnknown)
    virtual HRESULT __stdcall CreateConsole(MHTCONSOLE_DESC* pDesc, HWND* hWndOut) {
        LogMsg("CreateConsole: pDesc=%p hWndOut=%p", pDesc, hWndOut);
        if (hWndOut) *hWndOut = NULL;
        return S_OK;
    }
    virtual void __stdcall OutputDisplay(char* szString, int strLen) { LogMsg("OutputDisplay called"); }
    virtual void __stdcall OutputDisplayEx(char* szString, int strLen, DWORD color) { LogMsg("OutputDisplayEx called"); }
    virtual void __stdcall OutputFile(char* szString, int strLen) { /* stub */ }
    virtual void __stdcall SetDispRefreshRate(DWORD dwTickRate) { /* stub */ }
    virtual DWORD __stdcall GetDispRefreshRate() { return 1000; }
    virtual void __stdcall ForceRefreshDisplay() { /* stub */ }
    virtual void __stdcall ForceFlushFile() { /* stub */ }
    virtual void __stdcall FlushFileAll() { /* stub */ }
    virtual void __stdcall MessageLoop() {
        LogMsg("MessageLoop entered - server main loop starting");
        // This is the server's main loop!
        // Korean comments in ServerSystem.cpp say:
        // "This method doesn't return until the X button of the I4DyuchiCONSOLE dialog is pressed.
        //  If this method returns, the program will exit."
        // We need to keep the server running indefinitely.
        // Use Sleep-based loop instead of message loop since we have no window.
        while (true) {
            Sleep(1000); // Sleep 1 second at a time
        }
    }
    virtual LRESULT __stdcall CallDefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
        return DefWindowProc(NULL, message, wParam, lParam);
    }

    // IUnknown
    ULONG m_refCount;
    CConsoleStub() : m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
        if (!ppvObject) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_MyITConsole)) {
            *ppvObject = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() {
        return InterlockedIncrement(&m_refCount);
    }
    ULONG STDMETHODCALLTYPE Release() {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) delete this;
        return count;
    }
};

// Class Factory
class CConsoleFactory : public IClassFactory {
public:
    ULONG m_refCount;
    CConsoleFactory() : m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        if (!ppv) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&m_refCount); }
    ULONG STDMETHODCALLTYPE Release() {
        ULONG c = InterlockedDecrement(&m_refCount);
        if (c == 0) delete this;
        return c;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) {
        if (!ppv) return E_POINTER;
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;

        CConsoleStub* pObj = new CConsoleStub();
        HRESULT hr = pObj->QueryInterface(riid, ppv);
        if (FAILED(hr)) { delete pObj; }
        return hr;
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL) { return S_OK; }
};

// Exported COM functions
extern "C" {

__declspec(dllexport) STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    LogMsg("DllGetClassObject called");
    if (!ppv) {
        LogMsg("  ppv is NULL, returning E_POINTER");
        return E_POINTER;
    }
    if (!IsEqualCLSID(rclsid, CLSID_MyConsole)) {
        LogMsg("  CLSID mismatch!");
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    LogMsg("  Creating CConsoleFactory");
    CConsoleFactory* pFactory = new CConsoleFactory();
    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    if (FAILED(hr)) {
        LogMsg("  QueryInterface failed: 0x%08X", hr);
        delete pFactory;
    } else {
        LogMsg("  Factory created successfully");
    }
    return hr;
}

__declspec(dllexport) STDAPI DllCanUnloadNow() {
    return S_OK;
}

__declspec(dllexport) STDAPI DllRegisterServer() {
    // Register under both CLSIDs for compatibility
    const char* dllPath = "C:\\Windows\\System32\\ConsoleStub.dll";

    // CLSID_ULTRA_TCONSOLE
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
        "CLSID\\{4845A47E-F4E1-45cd-8561-C7B947BBA936}\\InprocServer32",
        0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE*)dllPath, (DWORD)strlen(dllPath)+1);
        RegSetValueExA(hKey, "ThreadingModel", 0, REG_SZ, (BYTE*)"Both", 5);
        RegCloseKey(hKey);
    }
    return S_OK;
}

__declspec(dllexport) STDAPI DllUnregisterServer() {
    RegDeleteKeyA(HKEY_CLASSES_ROOT,
        "CLSID\\{4845A47E-F4E1-45cd-8561-C7B947BBA936}\\InprocServer32");
    RegDeleteKeyA(HKEY_CLASSES_ROOT,
        "CLSID\\{4845A47E-F4E1-45cd-8561-C7B947BBA936}");
    return S_OK;
}

} // extern "C"

// Crash handler - REMOVED to avoid interfering with host process

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    return TRUE;
}
