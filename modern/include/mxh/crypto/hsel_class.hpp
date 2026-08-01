// hsel_class.hpp - Legacy CHSEL/CHSEL_STREAM ABI preservation layer.
//
// 1:1 byte-compatible drop-in for the legacy CHSEL virtual base and
// CHSEL_STREAM concrete derived classes from the [Lib]HSEL header.
// The modern HselStream (hsel_stream.hpp) provides the cryptographic
// core; this header re-exposes the original class hierarchy so legacy
// callers (NetworkMS.cpp, ClientNetwork.cpp, etc.) link unchanged.

// Constraints (ROADMAP 0):
//   * 2 virtual getter signatures of YHLibrary CHSEL MUST stay byte-identical to the
//     legacy header (callers link against vtable offsets, not names).
//   * HselKey and HselInit layout (already in hsel_stream.hpp) MUST
//     stay 1:1: 12 x int32_t = 48 bytes; 16 x int32_t = 64 bytes.
//   * CHSEL_STREAM::Encrypt/Decrypt default-arg semantics must be
//     preserved (iStreamSize = 0 means use the buffer natural size).

#pragma once

#include "mxh/crypto/hsel_stream.hpp"

namespace mxh::crypto {

// CHSEL - pure virtual base class. 1:1 with legacy class CHSEL in HSEL.h.
//
// YHLibrary public ABI has exactly two virtual getters and a non-virtual
// destructor. Encrypt/Decrypt and key methods belong to CHSEL_STREAM.
class CHSEL {
public:
    CHSEL(void) : iVersion(0), iHSELType(0) {}
    ~CHSEL(void) = default;

    virtual std::int32_t GetVersion(void) const { return iVersion; }
    virtual std::int32_t GetHSELType(void) const { return iHSELType; }

protected:
    std::int32_t iVersion;
    std::int32_t iHSELType;
};

class CHSEL_STREAM : public CHSEL {
public:
    CHSEL_STREAM(void);
    ~CHSEL_STREAM(void);

    std::int32_t Initial(HselInit hselinit);

    HselInit GetHSELCustomizeOption(void) const;
    HselKey GetNowHSELKey(void) const;

    bool Encrypt(char* lpStream, const std::int32_t iStreamSize = 0);
    bool Decrypt(char* lpStream, const std::int32_t iStreamSize = 0);

    char GetCRCConvertChar(void) const;
    short GetCRCConvertShort(void) const;
    std::int32_t GetCRCConvertInt(void) const;

    void SetKeyCustom(HselKey IntoKey);
    void SetNextKey(void);
    void GenerateKeys(HselKey& IntoKey);

private:
    HselStream m_stream;
};

}  // namespace mxh::crypto