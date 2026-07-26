// hsel_class.hpp - Legacy CHSEL/CHSEL_STREAM ABI preservation layer.
//
// 1:1 byte-compatible drop-in for the legacy CHSEL virtual base and
// CHSEL_STREAM concrete derived classes from the [Lib]HSEL header.
// The modern HselStream (hsel_stream.hpp) provides the cryptographic
// core; this header re-exposes the original class hierarchy so legacy
// callers (NetworkMS.cpp, ClientNetwork.cpp, etc.) link unchanged.

// Constraints (ROADMAP 0):
//   * 10 virtual signatures of CHSEL MUST stay byte-identical to the
//     legacy header (callers link against vtable offsets, not names).
//   * HselKey and HselInit layout (already in hsel_stream.hpp) MUST
//     stay 1:1: 12 x int32_t = 48 bytes; 16 x int32_t = 64 bytes.
//   * CHSEL_STREAM::Encrypt/Decrypt const-default-arg semantics must be
//     preserved (iStreamSize = 0 means use the buffer natural size).

#pragma once

#include "mxh/crypto/hsel_stream.hpp"

namespace mxh::crypto {

// CHSEL - pure virtual base class. 1:1 with legacy class CHSEL in HSEL.h.
//
// 10 pure virtual methods. vtable layout MUST stay unchanged; new virtuals
// must be appended (and legacy callers recompiled) - never reorder.
class CHSEL {
public:
    virtual ~CHSEL() = default;

    virtual std::int32_t GetVersion(void)               = 0;
    virtual std::int32_t GetHSELType(void)              = 0;

    virtual bool Encrypt(char* lpStream,
                         const std::int32_t iStreamSize = 0) = 0;
    virtual bool Decrypt(char* lpStream,
                         const std::int32_t iStreamSize = 0) = 0;

    virtual char    GetCRCConvertChar(void)             = 0;
    virtual short   GetCRCConvertShort(void)            = 0;
    virtual std::int32_t GetCRCConvertInt(void)         = 0;

    virtual void SetKeyCustom(HselKey IntoKey)          = 0;
    virtual void SetNextKey(void)                       = 0;
    virtual void GenerateKeys(HselKey& IntoKey)         = 0;
};

// CHSEL_STREAM - concrete derived class. 1:1 with legacy CHSEL_STREAM.
//
// Wraps the modern HselStream (the byte-identical reimplementation) and
// forwards every call. This is the class callers actually instantiate.
class CHSEL_STREAM : public CHSEL {
public:
    CHSEL_STREAM(void);
    ~CHSEL_STREAM(void) override;

    // Legacy Initial(HSEL_INITIAL hselinit) - returns HSEL type bits.
    std::int32_t Initial(HselInit hselinit);

    // Legacy convenience accessors.
    HselInit GetHSELCustomizeOption(void) const;
    HselKey  GetNowHSELKey(void)         const;

    // --- CHSEL overrides ---
    std::int32_t GetVersion(void) override;
    std::int32_t GetHSELType(void) override;

    bool Encrypt(char* lpStream,
                 const std::int32_t iStreamSize = 0) override;
    bool Decrypt(char* lpStream,
                 const std::int32_t iStreamSize = 0) override;

    char    GetCRCConvertChar(void)  override;
    short   GetCRCConvertShort(void) override;
    std::int32_t GetCRCConvertInt(void) override;

    void SetKeyCustom(HselKey IntoKey) override;
    void SetNextKey(void) override;
    void GenerateKeys(HselKey& IntoKey) override;

private:
    HselStream m_stream;
};

}  // namespace mxh::crypto