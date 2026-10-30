// x86-only read-only oracle for the original shipped math DLL.
#include <windows.h>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    auto dll = LoadLibraryExA(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!dll) { std::cerr << GetLastError(); return 3; }
    using MatrixFn = void (__stdcall*)(float*, float*);
    using SlerpFn = void (__stdcall*)(float*, float*, float*, float);
    auto matrix = reinterpret_cast<MatrixFn>(GetProcAddress(dll,"_MatrixFromQuaternion@8"));
    auto slerp = reinterpret_cast<SlerpFn>(GetProcAddress(dll,"_QuaternionSlerp@16"));
    if (!matrix || !slerp) return 4;
    std::cerr << "slerp epsilon=" << *reinterpret_cast<const float*>(reinterpret_cast<const char*>(dll)+0x15144) << '\n';
    std::array<std::array<float,4>,6> cases{{{0,0,0,1},{0,0,0.707106781f,0.707106781f},
        {0.5f,0.5f,0.5f,0.5f},{0,0,-0.707106781f,-0.707106781f},{0.182574186f,-0.365148372f,0.547722558f,0.730296744f},
        {0,0,0.1f,0.994987437f}}};
    std::ofstream out(argv[2]); out << std::setprecision(9) << "{\"cases\":[";
    bool first = true;
    for (auto a : cases) for (auto b : cases) {
        float q[4]{}, m[16]{}; slerp(q,a.data(),b.data(),0.37f); matrix(m,q);
        if (!first) out << ','; first = false;
        auto values = [&out](const float* v,int n) { out << '['; for(int i=0;i<n;++i){if(i)out<<',';out<<v[i];}out<<']'; };
        out << "{\"a\":"; values(a.data(),4); out << ",\"b\":"; values(b.data(),4);
        out << ",\"alpha\":0.37,\"quaternion\":"; values(q,4); out << ",\"matrix\":"; values(m,16); out << '}';
    }
    out << "]}"; FreeLibrary(dll); return out ? 0 : 5;
}
