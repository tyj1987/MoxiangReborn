// Tests for mxh::gx::dx11 texture_loader helpers (loadTGA / saveTGA / saveDDS).
//
// All CPU-side: we don't need a D3D11 device. We just exercise the codec
// routines and verify the byte-level output matches the legacy DX8 formats
// closely enough to round-trip.

#include "texture_loader.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

namespace {

constexpr std::uint32_t kTestWidth  = 8;
constexpr std::uint32_t kTestHeight = 6;

// Build a simple LoadedTexture with a known per-pixel pattern.
mxh::gx::dx11::LoadedTexture makeCheckerTexture() {
    mxh::gx::dx11::LoadedTexture tex;
    tex.width  = kTestWidth;
    tex.height = kTestHeight;
    tex.bps    = 32;
    tex.pixels.resize(static_cast<std::size_t>(kTestWidth) * kTestHeight * 4);
    for (std::uint32_t y = 0; y < kTestHeight; ++y) {
        for (std::uint32_t x = 0; x < kTestWidth; ++x) {
            std::uint8_t* p = &tex.pixels[(y * kTestWidth + x) * 4];
            const bool on = ((x + y) & 1) == 0;
            p[0] = on ? 0xFF : 0x10;   // R
            p[1] = on ? 0x80 : 0x20;   // G
            p[2] = on ? 0x40 : 0xCC;   // B
            p[3] = on ? 0xFF : 0x80;   // A
        }
    }
    return tex;
}

}  // namespace

// ===== TGA round-trip =====

TEST(TextureLoaderTGA, SaveTGARoundTripPreservesPixels) {
    auto in = makeCheckerTexture();
    auto tgaBytes = mxh::gx::dx11::saveTGA(in);
    ASSERT_FALSE(tgaBytes.empty());

    // TGA header is 18 bytes: type at offset 2 must be 2 (uncompressed true-color).
    ASSERT_GE(tgaBytes.size(), 18u + kTestWidth * kTestHeight * 4u);
    EXPECT_EQ(tgaBytes[0], 0);              // idLength
    EXPECT_EQ(tgaBytes[1], 0);              // colorMapType
    EXPECT_EQ(tgaBytes[2], 2);              // imageType
    EXPECT_EQ(tgaBytes[12], kTestWidth & 0xff);
    EXPECT_EQ(tgaBytes[13], (kTestWidth >> 8) & 0xff);
    EXPECT_EQ(tgaBytes[14], kTestHeight & 0xff);
    EXPECT_EQ(tgaBytes[15], (kTestHeight >> 8) & 0xff);
    EXPECT_EQ(tgaBytes[16], 32);            // bits per pixel
    EXPECT_EQ(tgaBytes[17], 0x20);          // descriptor: top-down (bit 5)

    // Decode the produced TGA and verify pixels match the input (BGRA-on-disk
    // means loadTGA reverses R/B; the higher-level RGBA representation in
    // memory is preserved verbatim).
    auto out = mxh::gx::dx11::loadTGA(tgaBytes.data(),
                                      static_cast<std::uint32_t>(tgaBytes.size()));
    EXPECT_EQ(out.width,  in.width);
    EXPECT_EQ(out.height, in.height);
    ASSERT_EQ(out.pixels.size(), in.pixels.size());
    for (std::size_t i = 0; i < in.pixels.size(); i += 4) {
        EXPECT_EQ(out.pixels[i + 0], in.pixels[i + 0]);  // R
        EXPECT_EQ(out.pixels[i + 1], in.pixels[i + 1]);  // G
        EXPECT_EQ(out.pixels[i + 2], in.pixels[i + 2]);  // B
        EXPECT_EQ(out.pixels[i + 3], in.pixels[i + 3]);  // A
    }
}

// ===== DDS round-trip =====

TEST(TextureLoaderDDS, SaveDDSMagicAndHeaderLayout) {
    auto in = makeCheckerTexture();
    auto ddsBytes = mxh::gx::dx11::saveDDS(in);
    ASSERT_FALSE(ddsBytes.empty());

    // Magic: "DDS "
    ASSERT_GE(ddsBytes.size(), 4u);
    EXPECT_EQ(ddsBytes[0], 'D');
    EXPECT_EQ(ddsBytes[1], 'D');
    EXPECT_EQ(ddsBytes[2], 'S');
    EXPECT_EQ(ddsBytes[3], ' ');

    // DDS_HEADER immediately after magic:
    //   dwSize       @ +0   = 124
    //   dwFlags      @ +4
    //   dwHeight     @ +8
    //   dwWidth      @ +12
    //   ...          @ +16..+72
    //   ddspf        @ +72  (32 bytes), ddspf.dwSize = 32
    ASSERT_GE(ddsBytes.size(), 4u + 124u);
    auto headerBytes = &ddsBytes[4];
    std::uint32_t dwSize;
    std::memcpy(&dwSize, headerBytes + 0, sizeof(dwSize));
    EXPECT_EQ(dwSize, 124u);

    std::uint32_t dwHeight, dwWidth;
    std::memcpy(&dwHeight, headerBytes + 8,  sizeof(dwHeight));
    std::memcpy(&dwWidth,  headerBytes + 12, sizeof(dwWidth));
    EXPECT_EQ(dwHeight, kTestHeight);
    EXPECT_EQ(dwWidth,  kTestWidth);

    // ddspf.dwSize at header offset 72.
    std::uint32_t pfSize;
    std::memcpy(&pfSize, headerBytes + 72, sizeof(pfSize));
    EXPECT_EQ(pfSize, 32u);

    // Pixel data starts at offset 4 + 124 = 128, size = w*h*4.
    EXPECT_EQ(ddsBytes.size(), 4u + 124u + kTestWidth * kTestHeight * 4u);
}

TEST(TextureLoaderDDS, SaveDDSPixelDataIsBGRA) {
    auto in = makeCheckerTexture();
    auto ddsBytes = mxh::gx::dx11::saveDDS(in);
    ASSERT_GE(ddsBytes.size(), 4u + 124u);

    // First pixel of input RGBA: (0xFF, 0x80, 0x40, 0xFF).
    // DDS stores BGRA → bytes must be (0x40, 0x80, 0xFF, 0xFF).
    auto pixelStart = ddsBytes.data() + 4 + 124;
    EXPECT_EQ(pixelStart[0], 0x40);  // B
    EXPECT_EQ(pixelStart[1], 0x80);  // G
    EXPECT_EQ(pixelStart[2], 0xFF);  // R
    EXPECT_EQ(pixelStart[3], 0xFF);  // A
}

TEST(TextureLoaderDDS, SaveDDSRejectsEmptyTexture) {
    mxh::gx::dx11::LoadedTexture bad;
    auto bytes = mxh::gx::dx11::saveDDS(bad);
    EXPECT_TRUE(bytes.empty());
}

TEST(TextureLoaderTGA, SaveTGARejectsEmptyTexture) {
    mxh::gx::dx11::LoadedTexture bad;
    auto bytes = mxh::gx::dx11::saveTGA(bad);
    EXPECT_TRUE(bytes.empty());
}

// ===== Auto-detect / mismatch =====

TEST(TextureLoaderAutoDetect, UnknownHeaderReturnsEmpty) {
    // Five junk bytes that aren't a valid TGA imageType (must be 1..11).
    std::uint8_t junk[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    auto tex = mxh::gx::dx11::loadTextureFromMemory(junk, sizeof(junk));
    EXPECT_TRUE(tex.pixels.empty());
}
