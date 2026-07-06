// Tests for mxh::gx::dx11::loadTGA (TGA decoder, uncompressed + RLE).
//
// We build tiny in-memory TGAs from scratch and verify the decoder produces
// the expected pixel data. These tests do NOT touch D3D11 — texture_loader
// is pure CPU-side decoding, so we can exercise it without a device.

#include "texture_loader.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

using mxh::gx::dx11::LoadedTexture;
using mxh::gx::dx11::loadTGA;
using mxh::gx::dx11::loadTextureFromMemory;

namespace {

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

// Build a 24-bit uncompressed BGR TGA buffer.
std::vector<std::uint8_t> makeUncompressedRGB24(std::uint16_t w, std::uint16_t h,
                                                std::uint8_t descriptor = 0x20) {
    TgaHeader hdr{};
    hdr.idLength = 0;
    hdr.colorMapType = 0;
    hdr.imageType = 2;  // uncompressed true-color
    hdr.width = w;
    hdr.height = h;
    hdr.bits = 24;
    hdr.descriptor = descriptor;

    std::vector<std::uint8_t> blob(sizeof(hdr) + w * h * 3);
    std::memcpy(blob.data(), &hdr, sizeof(hdr));
    auto* pixels = blob.data() + sizeof(hdr);
    for (std::uint16_t y = 0; y < h; ++y) {
        for (std::uint16_t x = 0; x < w; ++x) {
            auto* p = pixels + (y * w + x) * 3;
            // Encode position so we can verify decoding.
            p[0] = static_cast<std::uint8_t>((x * 7) & 0xff);   // B
            p[1] = static_cast<std::uint8_t>((y * 11) & 0xff);  // G
            p[2] = static_cast<std::uint8_t>(((x + y) * 5) & 0xff);  // R
        }
    }
    return blob;
}

// Build a 32-bit uncompressed BGRA TGA buffer.
std::vector<std::uint8_t> makeUncompressedRGBA32(std::uint16_t w, std::uint16_t h) {
    TgaHeader hdr{};
    hdr.imageType = 2;
    hdr.width = w;
    hdr.height = h;
    hdr.bits = 32;
    hdr.descriptor = 0x20;  // top-down

    std::vector<std::uint8_t> blob(sizeof(hdr) + w * h * 4);
    std::memcpy(blob.data(), &hdr, sizeof(hdr));
    auto* pixels = blob.data() + sizeof(hdr);
    for (std::uint16_t y = 0; y < h; ++y) {
        for (std::uint16_t x = 0; x < w; ++x) {
            auto* p = pixels + (y * w + x) * 4;
            p[0] = static_cast<std::uint8_t>(x);     // B
            p[1] = static_cast<std::uint8_t>(y);     // G
            p[2] = 0xff;                             // R
            p[3] = 0x80;                             // A
        }
    }
    return blob;
}

}  // namespace

TEST(TgaLoader, RejectsTruncatedHeader) {
    std::vector<std::uint8_t> tooSmall(4, 0);
    auto t = loadTGA(tooSmall.data(), static_cast<std::uint32_t>(tooSmall.size()));
    EXPECT_EQ(t.width, 0u);
    EXPECT_EQ(t.height, 0u);
    EXPECT_TRUE(t.pixels.empty());
}

TEST(TgaLoader, Decodes24BitTopDown) {
    auto blob = makeUncompressedRGB24(4, 4, 0x20);  // top-down (descriptor bit 5 set)
    auto t = loadTGA(blob.data(), static_cast<std::uint32_t>(blob.size()));
    ASSERT_EQ(t.width, 4u);
    ASSERT_EQ(t.height, 4u);
    ASSERT_EQ(t.pixels.size(), 4u * 4u * 4u);
    // Pixel (0,0): B=(0*7)&0xff=0, G=(0*11)&0xff=0, R=((0+0)*5)&0xff=0, A=255.
    EXPECT_EQ(t.pixels[0], 0);   // R
    EXPECT_EQ(t.pixels[1], 0);   // G
    EXPECT_EQ(t.pixels[2], 0);   // B
    EXPECT_EQ(t.pixels[3], 255); // A
    // Pixel (2,1): R=((2+1)*5)&0xff=15, G=(1*11)&0xff=11, B=(2*7)&0xff=14.
    auto* p = &t.pixels[(1 * 4 + 2) * 4];
    EXPECT_EQ(p[0], 15);
    EXPECT_EQ(p[1], 11);
    EXPECT_EQ(p[2], 14);
}

TEST(TgaLoader, Decodes24BitBottomUpFlipped) {
    // descriptor bit 5 = 0 -> bottom-up; decoder must flip Y.
    auto blob = makeUncompressedRGB24(2, 2, 0x00);
    auto t = loadTGA(blob.data(), static_cast<std::uint32_t>(blob.size()));
    ASSERT_EQ(t.width, 2u);
    ASSERT_EQ(t.height, 2u);
    ASSERT_EQ(t.pixels.size(), 2u * 2u * 4u);
    // Source (x=0,y=0) lands at output (x=0, py=height-1-0=1).
    auto* p = &t.pixels[(1 * 2 + 0) * 4];
    EXPECT_EQ(p[0], 0);   // R
    EXPECT_EQ(p[1], 0);   // G
    EXPECT_EQ(p[2], 0);   // B
    EXPECT_EQ(p[3], 255); // A
}

TEST(TgaLoader, Decodes32BitPreservesAlpha) {
    auto blob = makeUncompressedRGBA32(2, 2);
    auto t = loadTGA(blob.data(), static_cast<std::uint32_t>(blob.size()));
    ASSERT_EQ(t.width, 2u);
    ASSERT_EQ(t.height, 2u);
    ASSERT_EQ(t.pixels.size(), 2u * 2u * 4u);
    // (x=1,y=0): B=1, G=0, R=0xff, A=0x80.
    auto* p = &t.pixels[(0 * 2 + 1) * 4];
    EXPECT_EQ(p[0], 0xff);
    EXPECT_EQ(p[1], 0);
    EXPECT_EQ(p[2], 1);
    EXPECT_EQ(p[3], 0x80);
}

TEST(TgaLoader, AutoDetectDispatchesToTga) {
    auto blob = makeUncompressedRGBA32(1, 1);
    auto t = loadTextureFromMemory(blob.data(), static_cast<std::uint32_t>(blob.size()));
    EXPECT_EQ(t.width, 1u);
    EXPECT_EQ(t.height, 1u);
    EXPECT_EQ(t.pixels.size(), 4u);
}

TEST(TgaLoader, UnknownImageTypeProducesEmpty) {
    TgaHeader hdr{};
    hdr.imageType = 99;  // invalid
    hdr.width = 2;
    hdr.height = 2;
    hdr.bits = 24;
    hdr.descriptor = 0x20;
    std::vector<std::uint8_t> blob(sizeof(hdr) + 16, 0);
    std::memcpy(blob.data(), &hdr, sizeof(hdr));
    auto t = loadTGA(blob.data(), static_cast<std::uint32_t>(blob.size()));
    EXPECT_TRUE(t.pixels.empty());
}

TEST(TgaLoader, RLEPackAndUnpackRoundtrip) {
    // 4x1 image, RLE-encoded, 32 bpp, all same pixel (0x80,0x40,0x20,0xFF).
    // RLE packet: header=0x80|(count-1)=0x87, then 4 bytes pixel.
    TgaHeader hdr{};
    hdr.imageType = 10;  // RLE-compressed true-color
    hdr.width = 4;
    hdr.height = 1;
    hdr.bits = 32;
    hdr.descriptor = 0x20;
    std::vector<std::uint8_t> blob;
    blob.resize(sizeof(hdr));
    std::memcpy(blob.data(), &hdr, sizeof(hdr));
    // RLE packet header: 0x80 | (4-1) = 0x83
    blob.push_back(0x83);
    // Pixel BGRA = (0x80,0x40,0x20,0xFF)
    blob.push_back(0x80);
    blob.push_back(0x40);
    blob.push_back(0x20);
    blob.push_back(0xff);

    auto t = loadTGA(blob.data(), static_cast<std::uint32_t>(blob.size()));
    ASSERT_EQ(t.width, 4u);
    ASSERT_EQ(t.height, 1u);
    ASSERT_EQ(t.pixels.size(), 4u * 4u);
    for (std::uint32_t i = 0; i < 4; ++i) {
        auto* p = &t.pixels[i * 4];
        EXPECT_EQ(p[0], 0x20) << "R mismatch at " << i;  // R is channel 0 in output
        EXPECT_EQ(p[1], 0x40) << "G mismatch at " << i;
        EXPECT_EQ(p[2], 0x80) << "B mismatch at " << i;
        EXPECT_EQ(p[3], 0xff) << "A mismatch at " << i;
    }
}
