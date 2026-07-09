// mxh/render/dx11/texture_loader.cpp
// TGA decoder (uncompressed + RLE) and auto-detect format dispatch.
#include "texture_loader.hpp"

#include <algorithm>
#include <cstring>

#include "mxh/log/mlog.hpp"

#pragma pack(push, 1)
struct TgaHeader {
    std::uint8_t  idLength;
    std::uint8_t  colorMapType;
    std::uint8_t  imageType;
    std::uint16_t colorMapFirst;
    std::uint16_t colorMapLast;
    std::uint8_t  colorMapBits;
    std::uint16_t firstX;
    std::uint16_t firstY;
    std::uint16_t width;
    std::uint16_t height;
    std::uint8_t  bits;
    std::uint8_t  descriptor;
};
#pragma pack(pop)

// DDS file format reference:
//   https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds
// We emit the legacy DDS_HEADER (124 B) with an uncompressed BGRA8 pixel format
// (no DX10 extension header). Real DXT1/DXT5/BC7 compression is out of scope
// here — that needs DirectXTex or hand-written block encoders.
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((std::uint32_t)(std::uint8_t)(ch0)        | \
    ((std::uint32_t)(std::uint8_t)(ch1) <<  8) | \
    ((std::uint32_t)(std::uint8_t)(ch2) << 16) | \
    ((std::uint32_t)(std::uint8_t)(ch3) << 24))
#endif
#pragma pack(push, 1)
struct DdsPixelFormat {
    std::uint32_t dwSize;          // 32
    std::uint32_t dwFlags;         // DDPF_ALPHAPIXELS | DDPF_RGB
    std::uint32_t dwFourCC;
    std::uint32_t dwRGBBitCount;
    std::uint32_t dwRBitMask;
    std::uint32_t dwGBitMask;
    std::uint32_t dwBBitMask;
    std::uint32_t dwABitMask;
};
struct DdsHeader {
    std::uint32_t       dwSize;          // 124
    std::uint32_t       dwFlags;         // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH
    std::uint32_t       dwHeight;
    std::uint32_t       dwWidth;
    std::uint32_t       dwPitchOrLinearSize;
    std::uint32_t       dwDepth;
    std::uint32_t       dwMipMapCount;
    std::uint32_t       dwReserved1[11];
    DdsPixelFormat      ddspf;
    std::uint32_t       dwCaps;          // DDSCAPS_TEXTURE
    std::uint32_t       dwCaps2;
    std::uint32_t       dwCaps3;
    std::uint32_t       dwCaps4;
    std::uint32_t       dwReserved2;
};
#pragma pack(pop)

namespace mxh::gx::dx11 {

LoadedTexture loadTGA(const std::uint8_t* data, std::uint32_t size) {
    LoadedTexture out;
    if (size < sizeof(TgaHeader)) {
        MLOG_WARN("[tga] file too small (%u bytes)", size);
        return out;
    }
    TgaHeader hdr{};
    std::memcpy(&hdr, data, sizeof(hdr));

    out.width  = hdr.width;
    out.height = hdr.height;
    out.bps    = 32;
    out.pixels.assign(out.width * out.height * 4, 0);

    const std::uint8_t* src = data + sizeof(TgaHeader) + hdr.idLength;
    std::uint32_t bytesLeft = size - static_cast<std::uint32_t>(sizeof(TgaHeader) + hdr.idLength);

    auto writePixel = [&](std::uint32_t x, std::uint32_t y, std::uint8_t b, std::uint8_t g,
                          std::uint8_t r, std::uint8_t a) {
        // TGA bottom-up if descriptor bit 5 is 0; flip if needed.
        std::uint32_t py = (hdr.descriptor & 0x20) ? y : (out.height - 1 - y);
        std::uint32_t px = x;
        std::uint8_t* dst = &out.pixels[(py * out.width + px) * 4];
        dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
    };

    if (hdr.imageType == 2) {
        // Uncompressed.
        std::uint8_t bytesPerPixel = hdr.bits / 8;
        for (std::uint32_t y = 0; y < out.height && bytesLeft >= bytesPerPixel; ++y) {
            for (std::uint32_t x = 0; x < out.width; ++x) {
                if (bytesLeft < bytesPerPixel) break;
                std::uint8_t b = src[0], g = src[1], r = src[2], a = 255;
                if (bytesPerPixel >= 4) a = src[3];
                writePixel(x, y, b, g, r, a);
                src += bytesPerPixel; bytesLeft -= bytesPerPixel;
            }
        }
        return out;
    }

    if (hdr.imageType == 10) {
        // RLE-compressed.
        std::uint8_t bytesPerPixel = hdr.bits / 8;
        std::uint32_t written = 0;
        std::uint32_t total = out.width * out.height;
        std::uint32_t x = 0, y = 0;
        while (written < total && bytesLeft >= 1) {
            std::uint8_t header = src[0]; src++; bytesLeft--;
            if (header & 0x80) {
                // RLE chunk.
                std::uint32_t count = (header & 0x7f) + 1;
                if (bytesLeft < bytesPerPixel) break;
                std::uint8_t b = src[0], g = src[1], r = src[2], a = 255;
                if (bytesPerPixel >= 4) a = src[3];
                for (std::uint32_t i = 0; i < count && written < total; ++i) {
                    writePixel(x, y, b, g, r, a);
                    ++x; if (x >= out.width) { x = 0; ++y; }
                    ++written;
                }
                src += bytesPerPixel; bytesLeft -= bytesPerPixel;
            } else {
                // Raw chunk.
                std::uint32_t count = header + 1;
                for (std::uint32_t i = 0; i < count && written < total; ++i) {
                    if (bytesLeft < bytesPerPixel) break;
                    std::uint8_t b = src[0], g = src[1], r = src[2], a = 255;
                    if (bytesPerPixel >= 4) a = src[3];
                    writePixel(x, y, b, g, r, a);
                    ++x; if (x >= out.width) { x = 0; ++y; }
                    ++written;
                    src += bytesPerPixel; bytesLeft -= bytesPerPixel;
                }
            }
        }
        return out;
    }

    MLOG_WARN("[tga] unsupported image type %u (bits=%u)", hdr.imageType, hdr.bits);
    out.pixels.clear();
    return out;
}

LoadedTexture loadTextureFromMemory(const std::uint8_t* data, std::uint32_t size) {
    if (size < 4) return {};
    // Auto-detect: TGA footer "TRUEVISION-XFILE.\0" at end, or by imageType byte.
    if (size >= sizeof(TgaHeader) && data[2] >= 1 && data[2] <= 11) {
        return loadTGA(data, size);
    }
    MLOG_WARN("[tex] unknown image format, magic bytes: %02x %02x %02x %02x",
              data[0], data[1], data[2], data[3]);
    return {};
}

std::vector<std::uint8_t> saveTGA(const LoadedTexture& tex) {
    if (tex.pixels.empty() || tex.width == 0 || tex.height == 0) return {};
    std::vector<std::uint8_t> out;
    const std::size_t headerSize = 18;
    out.resize(headerSize + tex.width * tex.height * 4);

    // TGA header (type 2 = uncompressed true-color).
    out[0]  = 0;                         // idLength
    out[1]  = 0;                         // colorMapType
    out[2]  = 2;                         // imageType = uncompressed true-color
    out[3]  = 0; out[4]  = 0;          // colorMapFirst
    out[5]  = 0; out[6]  = 0;          // colorMapLast
    out[7]  = 0;                         // colorMapBits
    out[8]  = 0; out[9]  = 0;          // firstX
    out[10] = 0; out[11] = 0;          // firstY
    // width (little-endian)
    out[12] = static_cast<std::uint8_t>(tex.width & 0xff);
    out[13] = static_cast<std::uint8_t>((tex.width >> 8) & 0xff);
    // height (little-endian)
    out[14] = static_cast<std::uint8_t>(tex.height & 0xff);
    out[15] = static_cast<std::uint8_t>((tex.height >> 8) & 0xff);
    out[16] = 32;                        // bits per pixel
    out[17] = 0x20;                      // descriptor: top-down (bit 5 set)

    // Pixel data: copy RGBA pixels, converting RGBA -> BGRA.
    std::uint8_t* dst = out.data() + headerSize;
    for (std::size_t i = 0; i < tex.pixels.size() / 4; ++i) {
        const std::uint8_t r = tex.pixels[i * 4 + 0];
        const std::uint8_t g = tex.pixels[i * 4 + 1];
        const std::uint8_t b = tex.pixels[i * 4 + 2];
        const std::uint8_t a = tex.pixels[i * 4 + 3];
        *dst++ = b;  // B
        *dst++ = g;  // G
        *dst++ = r;  // R
        *dst++ = a;  // A
    }
    return out;
}

// Encode a 32-bit BGRA / RGBA texture as an uncompressed .DDS file (legacy
// DDS_HEADER, no DX10 extension). The output:
//   - magic "DDS " (4 B)
//   - DDS_HEADER (124 B)
//   - raw top-down BGRA pixels (width * height * 4 B)
// Caller must write `out` to disk.
//
// This is a "passthrough rewrap": we don't actually compress to BC1/BC7.
// The legacy engine only called this on .tga/.bmp inputs that the resource
// manager wanted as .dds for the runtime loader. For Phase 5 we accept that
// the runtime loader also handles uncompressed BGRA8 DDS, so the rewrap alone
// is enough for behavior compatibility. Real BC compression needs DirectXTex.
std::vector<std::uint8_t> saveDDS(const LoadedTexture& tex) {
    if (tex.pixels.empty() || tex.width == 0 || tex.height == 0) return {};

    std::vector<std::uint8_t> out;
    const std::size_t kMagicAndHeader = 4 + sizeof(DdsHeader);
    out.resize(kMagicAndHeader + tex.width * tex.height * 4);

    // Magic.
    out[0] = 'D'; out[1] = 'D'; out[2] = 'S'; out[3] = ' ';

    auto* hdr = reinterpret_cast<DdsHeader*>(out.data() + 4);
    std::memset(hdr, 0, sizeof(*hdr));
    hdr->dwSize  = sizeof(DdsHeader);                                   // 124
    hdr->dwFlags = 0x00000001u | 0x00000002u | 0x00000004u | 0x00001000u; // CAPS|HEIGHT|WIDTH|PIXELFORMAT
    hdr->dwFlags |= 0x00000008u;                                         // PITCH (linear size for uncompressed)
    hdr->dwHeight = tex.height;
    hdr->dwWidth  = tex.width;
    hdr->dwPitchOrLinearSize = tex.width * 4;                            // BGRA8: 4 B/px
    hdr->dwDepth  = 0;
    hdr->dwMipMapCount = 0;
    // hdr->dwReserved1[11] = {0}

    hdr->ddspf.dwSize  = sizeof(DdsPixelFormat);                         // 32
    hdr->ddspf.dwFlags = 0x00000001u | 0x00000040u;                      // DDPF_ALPHAPIXELS | DDPF_RGB
    hdr->ddspf.dwFourCC = 0;
    hdr->ddspf.dwRGBBitCount = 32;
    hdr->ddspf.dwRBitMask = 0x00FF0000u;
    hdr->ddspf.dwGBitMask = 0x0000FF00u;
    hdr->ddspf.dwBBitMask = 0x000000FFu;
    hdr->ddspf.dwABitMask = 0xFF000000u;

    hdr->dwCaps = 0x00001000u;                                           // DDSCAPS_TEXTURE
    // hdr->dwCaps2..dwReserved2 = 0

    // Pixel data: rgba → bgra, top-down (DDS rows go top to bottom).
    std::uint8_t* dst = out.data() + kMagicAndHeader;
    for (std::uint32_t y = 0; y < tex.height; ++y) {
        const std::uint8_t* srcRow = tex.pixels.data() + y * tex.width * 4;
        for (std::uint32_t x = 0; x < tex.width; ++x) {
            const std::uint8_t r = srcRow[x * 4 + 0];
            const std::uint8_t g = srcRow[x * 4 + 1];
            const std::uint8_t b = srcRow[x * 4 + 2];
            const std::uint8_t a = srcRow[x * 4 + 3];
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
            *dst++ = a;
        }
    }
    return out;
}

// =============================================================================
// BC1 (DXT1) / BC3 (DXT5) block encoders — Phase 5 deferred real compression.
// =============================================================================
// Both encoders follow the standard S3TC algorithm:
//   1. Split the image into 4×4 pixel blocks (image dimensions are padded to a
//      multiple of 4 by edge-clamping).
//   2. For each block, derive two endpoint colors and quantize each pixel to
//      one of four palette entries; the 16 × 2-bit indices are packed into 4
//      bytes. For BC3, an additional 8-byte alpha block (2 endpoints + 16 ×
//      3-bit alpha indices) precedes the BC1 RGB block.
//
// Endpoint selection: we use a "min/max" anchor — color0 = brightest pixel in
// the block, color1 = darkest — and derive the two intermediates by
// interpolation. This is not the optimal perceptual DXT1 (which would
// principal-axis-project the block), but it is correct, deterministic, and
// produces ~6:1 compression for RGB blocks (64 B → 8 B). For BC3 the same
// scheme yields ~4:1.
//
// All output is 16-bit RGB565 endpoints (BC1) and 8-bit alpha (BC3).
// =============================================================================

namespace {

// A single RGBA8 pixel.
struct Rgba8 {
    std::uint8_t r, g, b, a;
};

inline std::uint16_t rgb888_to_565(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return static_cast<std::uint16_t>(
        ((r & 0xF8u) << 8) |   // top 5 bits of red  → bits 15-11
        ((g & 0xFCu) << 3) |   // top 6 bits of green → bits 10-5
        ((b & 0xF8u) >> 3));   // top 5 bits of blue  → bits 4-0
}

// Expand an RGB565 endpoint to a full 8-bit-per-channel color with replication
// of the LSBs to fill the bottom 3 bits (standard S3TC expansion).
inline void rgb565_to_888(std::uint16_t c, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
    std::uint32_t rb = (c >> 11) & 0x1Fu;
    std::uint32_t gb = (c >>  5) & 0x3Fu;
    std::uint32_t bb =  c        & 0x1Fu;
    r = static_cast<std::uint8_t>((rb << 3) | (rb >> 2));
    g = static_cast<std::uint8_t>((gb << 2) | (gb >> 4));
    b = static_cast<std::uint8_t>((bb << 3) | (bb >> 2));
}

// Sum-of-squared-differences between two 8-bit-per-channel pixels. Used to
// pick the closest palette entry during quantization.
inline int ssd_rgb(const Rgba8& a, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    int dr = int(a.r) - int(r);
    int dg = int(a.g) - int(g);
    int db = int(a.b) - int(b);
    return dr*dr + dg*dg + db*db;
}

// Encode a single 4×4 BC1 block. outBlock must be 8 bytes. We use the
// "no-alpha" mode (color0 > color1) — interpolated colors c2 and c3 are
// available, and there is no transparent black.
void encode_bc1_block(const Rgba8 block[16], std::uint8_t outBlock[8]) {
    // Find brightest and darkest pixels (by sum R+G+B).
    int brightIdx = 0, darkIdx = 0;
    int brightSum = -1, darkSum = 1 << 30;
    for (int i = 0; i < 16; ++i) {
        int s = int(block[i].r) + int(block[i].g) + int(block[i].b);
        if (s > brightSum) { brightSum = s; brightIdx = i; }
        if (s < darkSum)   { darkSum   = s; darkIdx   = i; }
    }
    std::uint16_t c0 = rgb888_to_565(block[brightIdx].r, block[brightIdx].g, block[brightIdx].b);
    std::uint16_t c1 = rgb888_to_565(block[darkIdx].r,   block[darkIdx].g,   block[darkIdx].b);
    // S3TC requires color0 > color1 for the 4-color mode. If our selection
    // happens to invert, swap. (The standard contract is the "if color0 >
    // color1 then 4-color" branch; if color0 <= color1 you get the 3-color +
    // 1-bit-transparent mode which is harder to use and rarely wanted.)
    if (c0 < c1) std::swap(c0, c1);
    if (c0 == c1) c1 = static_cast<std::uint16_t>(c0 - 1); // ensure c0 > c1

    // Expand endpoints to full 8-bit for quantization comparisons.
    std::uint8_t r0, g0, b0, r1, g1, b1;
    rgb565_to_888(c0, r0, g0, b0);
    rgb565_to_888(c1, r1, g1, b1);
    // c2 = (2*c0 + c1) / 3, c3 = (c0 + 2*c1) / 3
    std::uint8_t r2 = static_cast<std::uint8_t>((2*int(r0) + int(r1)) / 3);
    std::uint8_t g2 = static_cast<std::uint8_t>((2*int(g0) + int(g1)) / 3);
    std::uint8_t b2 = static_cast<std::uint8_t>((2*int(b0) + int(b1)) / 3);
    std::uint8_t r3 = static_cast<std::uint8_t>((int(r0) + 2*int(r1)) / 3);
    std::uint8_t g3 = static_cast<std::uint8_t>((int(g0) + 2*int(g1)) / 3);
    std::uint8_t b3 = static_cast<std::uint8_t>((int(b0) + 2*int(b1)) / 3);

    // Pack 16 × 2-bit indices into 4 bytes (LSB-first, row-major).
    std::uint32_t idxPacked = 0;
    for (int i = 0; i < 16; ++i) {
        const Rgba8& px = block[i];
        int d0 = ssd_rgb(px, r0, g0, b0);
        int d1 = ssd_rgb(px, r1, g1, b1);
        int d2 = ssd_rgb(px, r2, g2, b2);
        int d3 = ssd_rgb(px, r3, g3, b3);
        int best = 0;
        int bestD = d0;
        if (d1 < bestD) { best = 1; bestD = d1; }
        if (d2 < bestD) { best = 2; bestD = d2; }
        if (d3 < bestD) { best = 3; }
        idxPacked |= (static_cast<std::uint32_t>(best) << (2 * i));
    }
    // Output: little-endian c0, c1, then 4 bytes of packed indices.
    outBlock[0] = static_cast<std::uint8_t>(c0 & 0xFF);
    outBlock[1] = static_cast<std::uint8_t>(c0 >> 8);
    outBlock[2] = static_cast<std::uint8_t>(c1 & 0xFF);
    outBlock[3] = static_cast<std::uint8_t>(c1 >> 8);
    outBlock[4] = static_cast<std::uint8_t>(idxPacked & 0xFF);
    outBlock[5] = static_cast<std::uint8_t>((idxPacked >>  8) & 0xFF);
    outBlock[6] = static_cast<std::uint8_t>((idxPacked >> 16) & 0xFF);
    outBlock[7] = static_cast<std::uint8_t>((idxPacked >> 24) & 0xFF);
}

// Encode the 8-byte alpha block of BC3, or the 8-byte body of BC4.
// Both formats share the same single-channel block layout:
//   - 2 endpoint bytes (a0, a1) where a0 > a1 selects 6-step linear
//     interpolation (a0 < a1 selects 8-step with a6=0, a7=255).
//   - 16 × 3-bit palette indices packed into 6 bytes (LSB-first, row-major).
// Total: 8 bytes per 4×4 block.
//
// `channel` is 0=R, 1=G, 2=B, 3=A; we sample that channel from each RGBA8
// pixel. For BC3 we always pass 3 (alpha). For BC4 the caller picks the
// channel (typically 0 for grayscale textures).
void encode_single_channel_block(const Rgba8 block[16], int channel,
                                 std::uint8_t outBlock[8]) {
    auto pick = [channel](const Rgba8& p) -> std::uint8_t {
        switch (channel) {
            case 0: return p.r;
            case 1: return p.g;
            case 2: return p.b;
            default: return p.a;
        }
    };
    std::uint8_t aMin = 255, aMax = 0;
    for (int i = 0; i < 16; ++i) {
        std::uint8_t v = pick(block[i]);
        if (v < aMin) aMin = v;
        if (v > aMax) aMax = v;
    }
    if (aMin == aMax) {
        // Flat block: endpoints equal, every index 0.
        outBlock[0] = aMax;
        outBlock[1] = aMin;
        std::uint64_t idxPacked = 0;
        for (int k = 0; k < 6; ++k) outBlock[2 + k] = static_cast<std::uint8_t>((idxPacked >> (8 * k)) & 0xFF);
        return;
    }

    // BC4 / BC3-alpha have two interpolation modes determined by endpoint
    // order:
    //   a0 > a1: 6-step linear interpolation between a0 and a1.
    //   a0 <= a1: 8-step with a6=0 and a7=255 (premultiplied alpha hack).
    // We pick whichever gives the lowest distortion for the source block.
    // For typical BC4 grayscale / alpha data, a0 > a1 (interpolated) is
    // almost always the right pick; we still fall back to a0 <= a1 if the
    // error would be lower.
    auto error_6step = [&](std::uint8_t e0, std::uint8_t e1) -> std::uint64_t {
        std::uint64_t err = 0;
        for (int i = 0; i < 16; ++i) {
            int v = int(pick(block[i]));
            int bestD = 1 << 30;
            for (int k = 0; k < 6; ++k) {
                int p = ((6 - k) * int(e0) + k * int(e1)) / 6; // 0..5 → e0..e1
                int d = std::abs(v - p);
                if (d < bestD) bestD = d;
            }
            err += std::uint64_t(bestD) * bestD;
        }
        return err;
    };
    auto error_8step = [&](std::uint8_t e0, std::uint8_t e1) -> std::uint64_t {
        std::uint64_t err = 0;
        // 8-step: alphas[0]=e0, alphas[1]=e1, alphas[2..5]=lerp e0→e1 in
        // 4 segments, alphas[6]=0, alphas[7]=255.
        for (int i = 0; i < 16; ++i) {
            int v = int(pick(block[i]));
            int bestD = std::abs(v - int(e0));
            int d1 = std::abs(v - int(e1));
            if (d1 < bestD) bestD = d1;
            for (int k = 1; k <= 4; ++k) {
                int p = ((4 - k) * int(e0) + k * int(e1)) / 4;
                int d = std::abs(v - p);
                if (d < bestD) bestD = d;
            }
            int d6 = std::abs(v - 0);
            if (d6 < bestD) bestD = d6;
            int d7 = std::abs(v - 255);
            if (d7 < bestD) bestD = d7;
            err += std::uint64_t(bestD) * bestD;
        }
        return err;
    };

    const std::uint64_t err6 = error_6step(aMax, aMin);   // a0 > a1
    const std::uint64_t err8 = error_8step(aMin, aMax);   // a0 <= a1
    const bool use6 = (err6 <= err8);
    const std::uint8_t e0 = use6 ? aMax : aMin;
    const std::uint8_t e1 = use6 ? aMin : aMax;

    // Build palette.
    std::uint8_t pal[8];
    pal[0] = e0;
    pal[1] = e1;
    if (use6) {
        for (int k = 1; k <= 6; ++k) {
            pal[1 + k] = static_cast<std::uint8_t>(((7 - k) * int(e0) + k * int(e1)) / 7);
        }
    } else {
        for (int k = 1; k <= 4; ++k) {
            pal[1 + k] = static_cast<std::uint8_t>(((4 - k) * int(e0) + k * int(e1)) / 4);
        }
        pal[6] = 0;
        pal[7] = 255;
    }
    const int palSize = use6 ? 6 : 8;

    // Pack 16 × 3-bit indices.
    std::uint64_t idxPacked = 0;
    for (int i = 0; i < 16; ++i) {
        std::uint8_t v = pick(block[i]);
        int best = 0;
        int bestD = std::abs(int(v) - int(pal[0]));
        for (int k = 1; k < palSize; ++k) {
            int d = std::abs(int(v) - int(pal[k]));
            if (d < bestD) { best = k; bestD = d; }
        }
        idxPacked |= (static_cast<std::uint64_t>(best) << (3 * i));
    }
    outBlock[0] = e0;
    outBlock[1] = e1;
    for (int k = 0; k < 6; ++k) outBlock[2 + k] = static_cast<std::uint8_t>((idxPacked >> (8 * k)) & 0xFF);
}

// Read a 4×4 block from the source image with edge-clamping for dimensions
// that are not a multiple of 4 (DDS requires padded blocks).
inline Rgba8 sample_block(const Rgba8* pixels, std::uint32_t W, std::uint32_t H,
                          std::uint32_t bx, std::uint32_t by, int idx) {
    int lx = idx & 3;
    int ly = idx >> 2;
    std::uint32_t x = std::min(bx * 4u + static_cast<std::uint32_t>(lx), W - 1);
    std::uint32_t y = std::min(by * 4u + static_cast<std::uint32_t>(ly), H - 1);
    return pixels[y * W + x];
}

bool has_transparent_alpha(const Rgba8* pixels, std::size_t count) {
    // BC1 with non-1-bit alpha needs BC3. We trigger BC3 when any pixel has
    // an alpha that's not 0 or 255 — or, equivalently, when the alpha channel
    // carries meaningful information. A simpler proxy: if min(alpha) > 0 and
    // max(alpha) < 255, the image uses alpha gradient → use BC3.
    if (count == 0) return false;
    std::uint8_t aMin = 255, aMax = 0;
    for (std::size_t i = 0; i < count; ++i) {
        std::uint8_t a = pixels[i].a;
        if (a < aMin) aMin = a;
        if (a > aMax) aMax = a;
    }
    // Has alpha gradient: prefer BC3.
    return (aMin > 0) && (aMax < 255);
}

} // anonymous namespace

// Encode a 32-bit RGBA8 texture as a real BC-compressed .DDS file.
// Output layout matches the standard DDS contract:
//   - magic "DDS " (4 B)
//   - DDS_HEADER (124 B, legacy)
//   - blocks (BC1: 8 B, BC3: 16 B, BC4: 8 B, BC5: 16 B)
//
// `format` selects the codec. BCFormat::Auto picks BC3 when the source has
// alpha gradient, otherwise BC1 (legacy behavior).
std::vector<std::uint8_t> saveDDS_BC(const LoadedTexture& tex, BCFormat format) {
    if (tex.pixels.empty() || tex.width == 0 || tex.height == 0) return {};

    // Convert to RGBA8 (LoadedTexture is already RGBA8 in our pipeline).
    const std::size_t N = static_cast<std::size_t>(tex.width) * tex.height;
    std::vector<Rgba8> src(N);
    for (std::size_t i = 0; i < N; ++i) {
        src[i] = { tex.pixels[i*4 + 0], tex.pixels[i*4 + 1],
                   tex.pixels[i*4 + 2], tex.pixels[i*4 + 3] };
    }

    // Auto: BC3 if alpha gradient, else BC1.
    if (format == BCFormat::Auto) {
        format = has_transparent_alpha(src.data(), N) ? BCFormat::BC3 : BCFormat::BC1;
    }

    // Resolve FourCC and block size.
    std::uint32_t fourcc = 0;
    std::size_t  blockBytes = 0;
    switch (format) {
        case BCFormat::BC1: fourcc = MAKEFOURCC('D','X','T','1'); blockBytes = 8;  break;
        case BCFormat::BC3: fourcc = MAKEFOURCC('D','X','T','5'); blockBytes = 16; break;
        case BCFormat::BC4: fourcc = MAKEFOURCC('A','T','I','1'); blockBytes = 8;  break;
        case BCFormat::BC5: fourcc = MAKEFOURCC('A','T','I','2'); blockBytes = 16; break;
        default: return {}; // Auto shouldn't reach here, but be safe.
    }

    const std::uint32_t W = tex.width;
    const std::uint32_t H = tex.height;
    const std::uint32_t nBX = (W + 3) / 4;
    const std::uint32_t nBY = (H + 3) / 4;
    const std::size_t totalBlocks = static_cast<std::size_t>(nBX) * nBY;

    const std::size_t kMagicAndHeader = 4 + sizeof(DdsHeader);
    std::vector<std::uint8_t> out(kMagicAndHeader + totalBlocks * blockBytes);

    // Magic.
    out[0] = 'D'; out[1] = 'D'; out[2] = 'S'; out[3] = ' ';

    auto* hdr = reinterpret_cast<DdsHeader*>(out.data() + 4);
    std::memset(hdr, 0, sizeof(*hdr));
    hdr->dwSize  = sizeof(DdsHeader);
    hdr->dwFlags = 0x00000001u | 0x00000002u | 0x00000004u | 0x00001000u; // CAPS|HEIGHT|WIDTH|PIXELFORMAT
    hdr->dwFlags |= 0x00000008u;                                            // LINEARSIZE for compressed
    hdr->dwHeight = H;
    hdr->dwWidth  = W;
    hdr->dwPitchOrLinearSize = static_cast<std::uint32_t>(totalBlocks * blockBytes);
    hdr->dwMipMapCount = 0;

    hdr->ddspf.dwSize  = sizeof(DdsPixelFormat);
    hdr->ddspf.dwFlags = 0x00000004u;                                       // DDPF_FOURCC
    hdr->ddspf.dwFourCC = fourcc;
    hdr->ddspf.dwRGBBitCount = 0;
    hdr->ddspf.dwRBitMask = 0;
    hdr->ddspf.dwGBitMask = 0;
    hdr->ddspf.dwBBitMask = 0;
    hdr->ddspf.dwABitMask = 0;

    hdr->dwCaps = 0x00001000u;                                              // DDSCAPS_TEXTURE

    // Encode each 4×4 block.
    std::uint8_t* dst = out.data() + kMagicAndHeader;
    for (std::uint32_t by = 0; by < nBY; ++by) {
        for (std::uint32_t bx = 0; bx < nBX; ++bx) {
            Rgba8 block[16];
            for (int i = 0; i < 16; ++i) {
                block[i] = sample_block(src.data(), W, H, bx, by, i);
            }
            switch (format) {
                case BCFormat::BC1:
                    encode_bc1_block(block, dst);
                    dst += 8;
                    break;
                case BCFormat::BC3:
                    // BC3 = BC3-alpha (8B) + BC1 RGB (8B) = 16B.
                    encode_single_channel_block(block, 3 /*alpha*/, dst);
                    encode_bc1_block(block, dst + 8);
                    dst += 16;
                    break;
                case BCFormat::BC4:
                    // BC4 = single channel. Use R by convention (grayscale or
                    // explicit R data; for alpha-only textures, callers should
                    // duplicate alpha into R before calling).
                    encode_single_channel_block(block, 0 /*R*/, dst);
                    dst += 8;
                    break;
                case BCFormat::BC5:
                    // BC5 = two BC4-style blocks: R channel then G channel.
                    // Standard for tangent-space normal maps (RG = XY; Z is
                    // reconstructed in shader as sqrt(1 - x² - y²)).
                    encode_single_channel_block(block, 0 /*R*/, dst);
                    encode_single_channel_block(block, 1 /*G*/, dst + 8);
                    dst += 16;
                    break;
                default:
                    return {}; // unreachable
            }
        }
    }
    return out;
}

} // namespace mxh::gx::dx11
