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

} // namespace mxh::gx::dx11
