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

TEST(TextureLoader, ResolvesLegacyTgaReferencesToCompiledDds) {
    using mxh::gx::dx11::compiledTextureName;
    EXPECT_EQ(compiledTextureName("m_nude.tga"), "m_nude.dds");
    EXPECT_EQ(compiledTextureName("M_HAIR01.TGA"), "M_HAIR01.dds");
    EXPECT_EQ(compiledTextureName("m_face01.tif"), "m_face01.tif");
}


std::uint32_t read_u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0])
         | (static_cast<std::uint32_t>(p[1]) <<  8)
         | (static_cast<std::uint32_t>(p[2]) << 16)
         | (static_cast<std::uint32_t>(p[3]) << 24);
}

mxh::gx::dx11::LoadedTexture make_solid(std::uint8_t r, std::uint8_t g,
                                          std::uint8_t b, std::uint8_t a) {
    mxh::gx::dx11::LoadedTexture t;
    t.width = 4; t.height = 4; t.bps = 32;
    t.pixels.assign(4 * 4 * 4, 0);
    for (std::uint32_t i = 0; i < 4 * 4; ++i) {
        t.pixels[i*4 + 0] = r;
        t.pixels[i*4 + 1] = g;
        t.pixels[i*4 + 2] = b;
        t.pixels[i*4 + 3] = a;
    }
    return t;
}

mxh::gx::dx11::LoadedTexture make_alpha_gradient() {
    // Alpha values strictly between 0 and 255 so has_transparent_alpha()
    // returns true (aMin > 0 && aMax < 255). Otherwise Auto mode would
    // pick BC1, not BC3 / BC7.
    mxh::gx::dx11::LoadedTexture t;
    t.width = 4; t.height = 4; t.bps = 32;
    t.pixels.assign(4 * 4 * 4, 0);
    for (std::uint32_t i = 0; i < 16; ++i) {
        t.pixels[i*4 + 0] = 200;
        t.pixels[i*4 + 1] = 100;
        t.pixels[i*4 + 2] = 50;
        t.pixels[i*4 + 3] = static_cast<std::uint8_t>(50 + (i * 9) % 150);  // 50..199
    }
    return t;
}

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

TEST(TextureLoaderDDS, UncompressedRoundTripPreservesPixels) {
    const auto in = makeCheckerTexture();
    const auto bytes = mxh::gx::dx11::saveDDS(in);
    const auto out = mxh::gx::dx11::loadDDS(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
    EXPECT_EQ(out.width, in.width);
    EXPECT_EQ(out.height, in.height);
    EXPECT_EQ(out.pixels, in.pixels);
}

TEST(TextureLoaderDDS, DXT1RoundTripDecodesSolidColor) {
    const auto in = make_solid(255, 0, 0, 255);
    const auto bytes = mxh::gx::dx11::saveDDS_BC(in, mxh::gx::dx11::BCFormat::BC1);
    const auto out = mxh::gx::dx11::loadDDS(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
    ASSERT_EQ(out.pixels.size(), in.pixels.size());
    EXPECT_GT(out.pixels[0], 245);
    EXPECT_LT(out.pixels[1], 10);
    EXPECT_LT(out.pixels[2], 10);
    EXPECT_EQ(out.pixels[3], 255);
}

TEST(TextureLoaderDDS, DXT5RoundTripRetainsAlpha) {
    const auto in = make_solid(32, 64, 128, 96);
    const auto bytes = mxh::gx::dx11::saveDDS_BC(in, mxh::gx::dx11::BCFormat::BC3);
    const auto out = mxh::gx::dx11::loadTextureFromMemory(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
    ASSERT_EQ(out.pixels.size(), in.pixels.size());
    EXPECT_NEAR(out.pixels[0], 32, 8);
    EXPECT_NEAR(out.pixels[1], 64, 8);
    EXPECT_NEAR(out.pixels[2], 128, 8);
    EXPECT_EQ(out.pixels[3], 96);
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

TEST(SaveDDSBC6H, MagicAndHeaderLayout) {
    auto tex = make_solid(0xAB, 0xCD, 0xEF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    ASSERT_FALSE(dds.empty());
    EXPECT_EQ(dds[0], 'D');
    EXPECT_EQ(dds[1], 'D');
    EXPECT_EQ(dds[2], 'S');
    EXPECT_EQ(dds[3], ' ');
    const std::uint32_t dwSize   = read_u32(dds.data() + 4);
    const std::uint32_t dwWidth  = read_u32(dds.data() + 4 + 12);
    const std::uint32_t dwHeight = read_u32(dds.data() + 4 + 8);
    EXPECT_EQ(dwSize, 124u);
    EXPECT_EQ(dwWidth,  4u);
    EXPECT_EQ(dwHeight, 4u);
}

TEST(SaveDDSBC6H, FourCCIsDX10ForBC6H) {
    auto tex = make_solid(0xAB, 0xCD, 0xEF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    ASSERT_GE(dds.size(), 4u + 124u + 4u);
    const std::uint32_t fourcc = read_u32(dds.data() + 4 + 80);
    EXPECT_EQ(fourcc, 0x30315844u);
}

TEST(SaveDDSBC6H, DX10ExtendedHeaderHasCorrectDXGIFormat) {
    auto tex = make_solid(0xAB, 0xCD, 0xEF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    // dxgiFormat is the first u32 of the DXT10 extended header, which
    // starts at file offset 4 (magic) + 124 (DDS_HEADER) = 128.
    const std::uint32_t dxgiFormat = read_u32(dds.data() + 4 + 124);
    EXPECT_EQ(dxgiFormat, 95u);  // BC6H_UFLOAT
}

TEST(SaveDDSBC6H, DX10ResourceDimensionIsTexture2D) {
    auto tex = make_solid(0xAB, 0xCD, 0xEF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    const std::uint32_t resDim = read_u32(dds.data() + 4 + 124 + 4);
    EXPECT_EQ(resDim, 3u);
}

TEST(SaveDDSBC6H, PayloadIs16BytesPerBlock) {
    auto tex = make_solid(0xAB, 0xCD, 0xEF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    const std::size_t kExpected = 4 + 124 + 20 + 1 * 16;
    EXPECT_EQ(dds.size(), kExpected);
}

TEST(SaveDDSBC6H, NonMultipleOf4PadsToBlockGrid) {
    mxh::gx::dx11::LoadedTexture tex;
    tex.width = 5; tex.height = 5; tex.bps = 32;
    tex.pixels.assign(5 * 5 * 4, 0x80);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    const std::size_t kExpected = 4 + 124 + 20 + 4 * 16;
    EXPECT_EQ(dds.size(), kExpected);
}

TEST(SaveDDSBC6H, BC6HBlockMode1FieldLayout) {
    auto tex = make_solid(0xFF, 0xFF, 0xFF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H);
    ASSERT_GE(dds.size(), 4u + 124u + 20u + 16u);
    const std::uint8_t* block = dds.data() + 4 + 124 + 20;
    EXPECT_EQ(block[0] & 0x1F, 0x01);
    for (std::size_t b = 8; b < 16; ++b) {
        EXPECT_EQ(block[b], 0);
    }
}

TEST(SaveDDSBC7, MagicAndHeaderLayout) {
    auto tex = make_solid(0x12, 0x34, 0x56, 0x78);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC7);
    ASSERT_FALSE(dds.empty());
    EXPECT_EQ(dds[0], 'D');
    EXPECT_EQ(dds[1], 'D');
    EXPECT_EQ(dds[2], 'S');
    EXPECT_EQ(dds[3], ' ');
    const std::uint32_t dwSize = read_u32(dds.data() + 4);
    EXPECT_EQ(dwSize, 124u);
}

TEST(SaveDDSBC7, FourCCIsDX10AndDXGIFormatIsBC7) {
    auto tex = make_solid(0x12, 0x34, 0x56, 0x78);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC7);
    ASSERT_GE(dds.size(), 4u + 124u + 20u + 16u);
    // fourcc lives in DdsPixelFormat.dwFourCC @ header + 80.
    const std::uint32_t fourcc     = read_u32(dds.data() + 4 + 80);
    // dxgiFormat lives in DdsHeaderDxt10.dxgiFormat @ 4 + 124 (DX10 ext start).
    const std::uint32_t dxgiFormat = read_u32(dds.data() + 4 + 124);
    EXPECT_EQ(fourcc,     0x30315844u);  // 'DX10'
    EXPECT_EQ(dxgiFormat, 98u);          // BC7_UNORM
}

TEST(SaveDDSBC7, PayloadIs16BytesPerBlock) {
    auto tex = make_solid(0x12, 0x34, 0x56, 0x78);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC7);
    const std::size_t kExpected = 4 + 124 + 20 + 1 * 16;
    EXPECT_EQ(dds.size(), kExpected);
}

TEST(SaveDDSBC7, BC7BlockMode6FieldLayout) {
    auto tex = make_solid(0xFF, 0xFF, 0xFF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC7);
    ASSERT_GE(dds.size(), 4u + 124u + 20u + 16u);
    const std::uint8_t* block = dds.data() + 4 + 124 + 20;
    EXPECT_EQ(block[0], 0x06);
    EXPECT_EQ(block[1], 0xFF);
    EXPECT_EQ(block[2], 0xFF);
    EXPECT_EQ(block[3], 0xFF);
    EXPECT_EQ(block[4], 0xFF);
    EXPECT_EQ(block[5], 0xFF);
    EXPECT_EQ(block[6], 0xFF);
    EXPECT_EQ(block[7], 0xFF);
    EXPECT_EQ(block[8], 0xFF);
}

TEST(SaveDDSBC7, BC7BlockNonWhiteHasMeanEndpoints) {
    auto tex = make_solid(0xFF, 0x00, 0x00, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC7);
    ASSERT_GE(dds.size(), 4u + 124u + 20u + 16u);
    const std::uint8_t* block = dds.data() + 4 + 124 + 20;
    EXPECT_EQ(block[0], 0x06);
    EXPECT_EQ(block[1], 0xFF);
    EXPECT_EQ(block[2], 0xFF);
    EXPECT_EQ(block[3], 0x00);
    EXPECT_EQ(block[4], 0x00);
    EXPECT_EQ(block[5], 0x00);
    EXPECT_EQ(block[6], 0x00);
}

TEST(SaveDDSBCAuto, AlphaGradientPicksBC3) {
    // Auto mode with alpha gradient must select BC3 (DXT5) to match
    // the legacy ConvertCompressedTexture heuristic. Hosts that want
    // BC7 pass BCFormat::BC7 explicitly (the BC6H/BC7 path requires
    // the DX10 extended header which not every DDS loader can read).
    auto tex = make_alpha_gradient();
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::Auto);
    ASSERT_FALSE(dds.empty());
    const std::uint32_t fourcc = read_u32(dds.data() + 4 + 80);
    EXPECT_EQ(fourcc, 0x35545844u);  // 'DXT5'
}

TEST(SaveDDSBCAuto, NoAlphaPicksBC1) {
    auto tex = make_solid(0xFF, 0xFF, 0xFF, 0xFF);
    auto dds = mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::Auto);
    ASSERT_FALSE(dds.empty());
    const std::uint32_t fourcc = read_u32(dds.data() + 4 + 80);
    EXPECT_EQ(fourcc, 0x31545844u);
}

TEST(SaveDDSBCFormat, EmptyTextureReturnsEmptyForAllFormats) {
    mxh::gx::dx11::LoadedTexture tex;
    EXPECT_TRUE(mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC1).empty());
    EXPECT_TRUE(mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC3).empty());
    EXPECT_TRUE(mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC6H).empty());
    EXPECT_TRUE(mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::BC7).empty());
    EXPECT_TRUE(mxh::gx::dx11::saveDDS_BC(tex, mxh::gx::dx11::BCFormat::Auto).empty());
}
