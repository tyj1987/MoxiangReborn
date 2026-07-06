// mxh/render/dx11/sprite.hpp
// IDISpriteObject DX11 implementation backed by a single 2D textured quad.
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <string>

#include "mxh/render/IRenderer.hpp"
#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

class Device;

class SpriteObject : public IDISpriteObject {
public:
    // Static factory: builds a sprite from raw pixel data (TGA decoded already
    // by the texture loader). Lifetime managed via IUnknown refcount.
    static SpriteObject* create(Device* dev, std::uint32_t width, std::uint32_t height,
                                TEXTURE_FORMAT fmt, const void* initialBits);

    // Static factory: builds a sprite from a file (TGA / bmp-style raw texture
    // inside .pak). Returns nullptr on failure.
    static SpriteObject* createFromFile(Device* dev, I4DyuchiFileStorage* pStorage,
                                        const char* szFileName, std::uint32_t dwFlag);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDISpriteObject
    BOOL __stdcall Draw(VECTOR2* pv2Scaling, float fRot, VECTOR2* pv2Trans, RECT* pRect,
                        std::uint32_t dwColor, std::uint32_t dwFlag) override;
    BOOL __stdcall Resize(float fWidth, float fHeight) override;
    BOOL __stdcall GetImageHeader(IMAGE_HEADER* pImgHeader, std::uint32_t dwFrameIndex) override;
    BOOL __stdcall LockRect(LOCKED_RECT* pOutLockedRect, RECT* pRect, TEXTURE_FORMAT TexFormat) override;
    BOOL __stdcall UnlockRect() override;

    std::uint32_t width()  const { return m_width; }
    std::uint32_t height() const { return m_height; }

private:
    SpriteObject() = default;
    ~SpriteObject();

    bool initialize(Device* dev, std::uint32_t w, std::uint32_t h, TEXTURE_FORMAT fmt, const void* bits);
    void renderImpl(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
                    std::uint32_t dwColor, float rotationRad, int zOrder);

    Device*                                          m_dev = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
    std::uint32_t                                    m_width  = 0;
    std::uint32_t                                    m_height = 0;
    std::uint32_t                                    m_refCount = 1;
};

} // namespace mxh::gx::dx11
