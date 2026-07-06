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

} // namespace mxh::gx::dx11
