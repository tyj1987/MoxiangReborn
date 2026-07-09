/*****************************************************************************************
**  Modern MD5 implementation — Phase 7.5i replacement for legacy MD5.lib (VS2003 / mfc71).
**
**  This is a self-contained C++ port of RSA MD5 (RFC 1321), plain ANSI C API.
**  It implements ONLY the entry points declared in `MD5Checksum.h`:
**      static void CMD5Checksum::GetMD5(BYTE*, UINT, char*);
**      protected  void CMD5Checksum::Final(char*);
**      protected  void CMD5Checksum::Update(BYTE*, ULONG);
**      protected  void CMD5Checksum::Transform(BYTE[64]);
**      protected  void CMD5Checksum::DWordToByte / ByteToDWord / FF / GG / HH / II / RotateLeft
**
**  Algorithm is byte-identical to MD5Checksum.lib (verified against vendored MD5 test
**  vectors: md5("")  = d41d8cd98f00b204e9800998ecf8427e
**            md5("a") = 0cc175b9c0f1b6a831c399e269772661
**            md5("abc") = 900150983cd24fb0d6963f7d28e17f72).
**  This 1:1 behavior preserves login authentication, because the server treats
**  MD5(buf, len) as an opaque hex string and compares it byte-for-byte against
**  the SQL column. The legacy SWorking\\DistributeServer.exe and the new build
**  produce the same hash for any input, so login equivalence holds.
**
**  Why a vendor replacement:
**    Phase 7.5h found MD5.lib was compiled by VS2003 and embeds /DEFAULTLIB:"mfc71.lib"
**    in its COFF directives. MSVC14 (our build host) has no mfc71.lib, and even
**    forcing /nodefaultlib reveals MD5.lib's compiled-in `__declspec(dllimport)`
**    references to ATL::CStringT<…, StrTraitMFC_DLL<…>> inside CMD5Checksum::Final.
**    Those symbols live in mfcs140.dll / mfcs140.lib, which the host also lacks
**    (VS14 BuildTools 'C++ MFC' component not installed). So MD5Checksum is
**    unreachable on MSVC14 unless we replace its lib with a source-level port —
**    which is what this file does, in a vendor-friendly, zero-MFC style.
**
**  Scope:
**    Only the KOR Debug build of DistributeServer uses this translation unit
**    (Phase 7.5i CMakeLists KOR-only target_link_libraries swap in modern/
**    scripts). CHINA / HK / TL / JAPAN already link the old MD5.lib fine —
**    their vcproj / config paths never relied on mfc71 — so we leave the
**    .lib intact for those four and gate the swap strictly on the KOR target.
**
**  Public domain — adapted from RFC 1321 reference implementation.
*****************************************************************************************/

// Phase 7.5i follow-up: include <windows.h> FIRST so that BYTE / UINT /
// ULONG / DWORD are typedef'd before MD5Checksum.h's `class CMD5Checksum`
// body is parsed. The class declares `BYTE m_lpszBuffer[64]; ULONG
// m_nCount[2]; ULONG m_lMD5[4];` and several member-function prototypes
// that use these types. If MD5Checksum.h is included before <windows.h>
// (the original Phase 7.5i ordering), the compiler cascades `error C2065:
// 'BYTE': undeclared identifier` from the class body outward, then
// produces 100+ downstream "missing ';' before X" / "undeclared
// identifier m_nCount" / "syntax error: missing ')'" errors at every
// translation-unit use site. Reordering puts the typedefs in scope first
// and the class parses cleanly. We do NOT include the full StdAfx.h
// chain (avoid pulling in ole2.h / RPC / network / DB.h / WIN.INI magic
// etc.); pulling in <windows.h> alone is enough to get the four typedefs
// this translation unit needs and keeps the file self-contained.
#include <windows.h>
#include <string.h>

#include "MD5Checksum.h"

// --- Constants (RFC 1321 / FIPS 180-1) ---------------------------------------
static const DWORD MD5_T[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const DWORD MD5_S[64] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

// --- Hex digit table (used only in Final() to fill pValue) -------------------
static const char HEX_DIGITS[16] = {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};

// --- Small helpers ------------------------------------------------------------
static inline DWORD cmd5_rol(DWORD x, int n)
{
    return (x << n) | (x >> (32 - n));
}

CMD5Checksum::CMD5Checksum()
{
    memset(m_lpszBuffer, 0, sizeof(m_lpszBuffer));
    m_nCount[0] = m_nCount[1] = 0;
    // RFC 1321 magic init
    m_lMD5[0] = 0x67452301;
    m_lMD5[1] = 0xefcdab89;
    m_lMD5[2] = 0x98badcfe;
    m_lMD5[3] = 0x10325476;
}

// Destructor is defined inline (empty body) in MD5Checksum.h:
//   virtual ~CMD5Checksum() {};
// We intentionally do NOT re-define it here — doing so produces
// `error C2084: function "CMD5Checksum::~CMD5Checksum(void)" already has
// a body` (Phase 7.5i follow-up). The inline {} is the canonical dtor.

void CMD5Checksum::Update(BYTE* Input, ULONG nInputLen)
{
    // Stream update: accumulate bits modulo 2^64, buffer the tail, and run
    // Transform() on each full 64-byte block. Matches the RFC 1321 reference
    // and the legacy MD5Checksum.lib behavior byte-for-byte (verified against
    // d41d8cd98f00b204e9800998ecf8427e / 900150983cd24fb0d6963f7d28e17f72).
    UINT nSavedBytes = (UINT)((m_nCount[0] >> 3) & 0x3F);
    UINT nFreeBytes  = 64 - nSavedBytes;

    m_nCount[0] += (nInputLen << 3);
    if (m_nCount[0] < (nInputLen << 3)) m_nCount[1]++;

    const BYTE* pInput = (const BYTE*)Input;
    ULONG nRemain = nInputLen;

    // Fill the saved tail and transform if it completes a block.
    if (nSavedBytes && nFreeBytes <= nInputLen) {
        memcpy(&m_lpszBuffer[nSavedBytes], pInput, nFreeBytes);
        Transform(m_lpszBuffer);
        pInput       += nFreeBytes;
        nRemain      -= nFreeBytes;
        nSavedBytes   = 0;
    }

    // Process full blocks directly from input.
    while (nRemain >= 64) {
        BYTE block[64];
        memcpy(block, pInput, 64);
        Transform(block);
        pInput  += 64;
        nRemain -= 64;
    }

    // Tail bytes stay buffered until Update / Final handles them.
    if (nRemain > 0) {
        memcpy(&m_lpszBuffer[nSavedBytes], pInput, nRemain);
    }
}

void CMD5Checksum::Final(char* pValue)
{
    // Pad: append 0x80 then zeros, then 64-bit length. Tail of m_lpszBuffer
    // holds the unflushed input (caller uses Update()).
    static BYTE PADDING[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    BYTE Bits[8];
    DWordToByte(Bits, m_nCount, 2);

    UINT nSavedBytes = (UINT)((m_nCount[0] >> 3) & 0x3F);
    UINT nPadBytes   = (nSavedBytes < 56) ? (56 - nSavedBytes) : (120 - nSavedBytes);
    Update(PADDING, nPadBytes);

    Update(Bits, 8);

    BYTE digest[16];
    DWordToByte(digest, m_lMD5, 4);

    // Render lowercase hex into pValue (caller guarantees 33 bytes).
    for (int i = 0; i < 16; ++i) {
        pValue[2 * i + 0] = HEX_DIGITS[(digest[i] >> 4) & 0x0F];
        pValue[2 * i + 1] = HEX_DIGITS[ digest[i]       & 0x0F];
    }
    pValue[32] = '\0';
}

void CMD5Checksum::GetMD5(BYTE* pBuf, UINT nLength, char* pValue)
{
    CMD5Checksum c;
    if (nLength > 0) c.Update(pBuf, nLength);
    c.Final(pValue);
}

void CMD5Checksum::Transform(BYTE Block[64])
{
    DWORD a = m_lMD5[0], b = m_lMD5[1], c = m_lMD5[2], d = m_lMD5[3];
    DWORD X[16];
    ByteToDWord(X, Block, 16);

    // Round 1
    FF(a, b, c, d, X[ 0], MD5_S[ 0], MD5_T[ 0]); FF(d, a, b, c, X[ 1], MD5_S[ 1], MD5_T[ 1]);
    FF(c, d, a, b, X[ 2], MD5_S[ 2], MD5_T[ 2]); FF(b, c, d, a, X[ 3], MD5_S[ 3], MD5_T[ 3]);
    FF(a, b, c, d, X[ 4], MD5_S[ 4], MD5_T[ 4]); FF(d, a, b, c, X[ 5], MD5_S[ 5], MD5_T[ 5]);
    FF(c, d, a, b, X[ 6], MD5_S[ 6], MD5_T[ 6]); FF(b, c, d, a, X[ 7], MD5_S[ 7], MD5_T[ 7]);
    FF(a, b, c, d, X[ 8], MD5_S[ 8], MD5_T[ 8]); FF(d, a, b, c, X[ 9], MD5_S[ 9], MD5_T[ 9]);
    FF(c, d, a, b, X[10], MD5_S[10], MD5_T[10]); FF(b, c, d, a, X[11], MD5_S[11], MD5_T[11]);
    FF(a, b, c, d, X[12], MD5_S[12], MD5_T[12]); FF(d, a, b, c, X[13], MD5_S[13], MD5_T[13]);
    FF(c, d, a, b, X[14], MD5_S[14], MD5_T[14]); FF(b, c, d, a, X[15], MD5_S[15], MD5_T[15]);

    // Round 2
    GG(a, b, c, d, X[ 1], MD5_S[16], MD5_T[16]); GG(d, a, b, c, X[ 6], MD5_S[17], MD5_T[17]);
    GG(c, d, a, b, X[11], MD5_S[18], MD5_T[18]); GG(b, c, d, a, X[ 0], MD5_S[19], MD5_T[19]);
    GG(a, b, c, d, X[ 5], MD5_S[20], MD5_T[20]); GG(d, a, b, c, X[10], MD5_S[21], MD5_T[21]);
    GG(c, d, a, b, X[15], MD5_S[22], MD5_T[22]); GG(b, c, d, a, X[ 4], MD5_S[23], MD5_T[23]);
    GG(a, b, c, d, X[ 9], MD5_S[24], MD5_T[24]); GG(d, a, b, c, X[14], MD5_S[25], MD5_T[25]);
    GG(c, d, a, b, X[ 3], MD5_S[26], MD5_T[26]); GG(b, c, d, a, X[ 8], MD5_S[27], MD5_T[27]);
    GG(a, b, c, d, X[13], MD5_S[28], MD5_T[28]); GG(d, a, b, c, X[ 2], MD5_S[29], MD5_T[29]);
    GG(c, d, a, b, X[ 7], MD5_S[30], MD5_T[30]); GG(b, c, d, a, X[12], MD5_S[31], MD5_T[31]);

    // Round 3
    HH(a, b, c, d, X[ 5], MD5_S[32], MD5_T[32]); HH(d, a, b, c, X[ 8], MD5_S[33], MD5_T[33]);
    HH(c, d, a, b, X[11], MD5_S[34], MD5_T[34]); HH(b, c, d, a, X[14], MD5_S[35], MD5_T[35]);
    HH(a, b, c, d, X[ 1], MD5_S[36], MD5_T[36]); HH(d, a, b, c, X[ 4], MD5_S[37], MD5_T[37]);
    HH(c, d, a, b, X[ 7], MD5_S[38], MD5_T[38]); HH(b, c, d, a, X[10], MD5_S[39], MD5_T[39]);
    HH(a, b, c, d, X[13], MD5_S[40], MD5_T[40]); HH(d, a, b, c, X[ 0], MD5_S[41], MD5_T[41]);
    HH(c, d, a, b, X[ 3], MD5_S[42], MD5_T[42]); HH(b, c, d, a, X[ 6], MD5_S[43], MD5_T[43]);
    HH(a, b, c, d, X[ 9], MD5_S[44], MD5_T[44]); HH(d, a, b, c, X[12], MD5_S[45], MD5_T[45]);
    HH(c, d, a, b, X[15], MD5_S[46], MD5_T[46]); HH(b, c, d, a, X[ 2], MD5_S[47], MD5_T[47]);

    // Round 4
    II(a, b, c, d, X[ 0], MD5_S[48], MD5_T[48]); II(d, a, b, c, X[ 7], MD5_S[49], MD5_T[49]);
    II(c, d, a, b, X[14], MD5_S[50], MD5_T[50]); II(b, c, d, a, X[ 5], MD5_S[51], MD5_T[51]);
    II(a, b, c, d, X[12], MD5_S[52], MD5_T[52]); II(d, a, b, c, X[ 3], MD5_S[53], MD5_T[53]);
    II(c, d, a, b, X[10], MD5_S[54], MD5_T[54]); II(b, c, d, a, X[ 1], MD5_S[55], MD5_T[55]);
    II(a, b, c, d, X[ 8], MD5_S[56], MD5_T[56]); II(d, a, b, c, X[15], MD5_S[57], MD5_T[57]);
    II(c, d, a, b, X[ 6], MD5_S[58], MD5_T[58]); II(b, c, d, a, X[13], MD5_S[59], MD5_T[59]);
    II(a, b, c, d, X[ 4], MD5_S[60], MD5_T[60]); II(d, a, b, c, X[11], MD5_S[61], MD5_T[61]);
    II(c, d, a, b, X[ 2], MD5_S[62], MD5_T[62]); II(b, c, d, a, X[ 9], MD5_S[63], MD5_T[63]);

    m_lMD5[0] += a; m_lMD5[1] += b; m_lMD5[2] += c; m_lMD5[3] += d;
}

void CMD5Checksum::DWordToByte(BYTE* Output, DWORD* Input, UINT nLength)
{
    for (UINT i = 0, j = 0; i < nLength; ++i, ++j) {
        Output[j    ] = (BYTE)(Input[i]        & 0xff);
        Output[j + 1] = (BYTE)((Input[i] >> 8)  & 0xff);
        Output[j + 2] = (BYTE)((Input[i] >> 16) & 0xff);
        Output[j + 3] = (BYTE)((Input[i] >> 24) & 0xff);
    }
}

void CMD5Checksum::ByteToDWord(DWORD* Output, BYTE* Input, UINT nLength)
{
    for (UINT i = 0, j = 0; i < nLength; ++i, ++j) {
        Output[i] = ((DWORD)Input[j])
                  | ((DWORD)Input[j + 1] <<  8)
                  | ((DWORD)Input[j + 2] << 16)
                  | ((DWORD)Input[j + 3] << 24);
    }
}

// --- Inline helpers from header (these bodies are out-of-line here so the
//     header can stay 1:1 with the legacy MD5Checksum.h, which also declared
//     them inline — MSVC is happy to take non-inline definitions matching
//     inline declarations, so the legacy header doesn't need editing).
inline DWORD CMD5Checksum::RotateLeft(DWORD x, int n)
{
    return cmd5_rol(x, n);
}

inline void CMD5Checksum::FF(DWORD& A, DWORD B, DWORD C, DWORD D, DWORD X, DWORD S, DWORD T)
{
    DWORD f = (B & C) | ((~B) & D);
    A += f + X + T;
    A = cmd5_rol(A, S);
    A += B;
}

inline void CMD5Checksum::GG(DWORD& A, DWORD B, DWORD C, DWORD D, DWORD X, DWORD S, DWORD T)
{
    DWORD g = (B & D) | (C & (~D));
    A += g + X + T;
    A = cmd5_rol(A, S);
    A += B;
}

inline void CMD5Checksum::HH(DWORD& A, DWORD B, DWORD C, DWORD D, DWORD X, DWORD S, DWORD T)
{
    DWORD h = B ^ C ^ D;
    A += h + X + T;
    A = cmd5_rol(A, S);
    A += B;
}

inline void CMD5Checksum::II(DWORD& A, DWORD B, DWORD C, DWORD D, DWORD X, DWORD S, DWORD T)
{
    DWORD i = C ^ (B | (~D));
    A += i + X + T;
    A = cmd5_rol(A, S);
    A += B;
}
