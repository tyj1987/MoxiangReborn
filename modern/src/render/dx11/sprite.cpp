// mxh/render/dx11/sprite.cpp
// IDISpriteObject DX11 implementation. Backed by a single ID3D11Texture2D + SRV.
#include "sprite.hpp"
#include "device.hpp"
#include "primitives.hpp"
#include "texture_loader.hpp"

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

SpriteObject::~SpriteObject() {
    m_srv.Reset();
    m_texture.Reset();
}

STDMETHODIMP SpriteObject::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IDISpriteObject*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) SpriteObject::AddRef() {
    return ++m_refCount;
}

STDMETHODIMP_(ULONG) SpriteObject::Release() {
    ULONG r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

bool SpriteObject::initialize(Device* dev, std::uint32_t w, std::uint32_t h, TEXTURE_FORMAT fmt, const void* bits) {
    m_dev    = dev;
    m_width  = w;
    m_height = h;

    DXGI_FORMAT dxgiFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
    switch (fmt) {
    case TEXTURE_FORMAT_A8R8G8B8: dxgiFmt = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    case TEXTURE_FORMAT_A4R4G4B4: dxgiFmt = DXGI_FORMAT_B4G4R4A4_UNORM; break;
    case TEXTURE_FORMAT_R5G6B5:   dxgiFmt = DXGI_FORMAT_B5G6R5_UNORM;   break;
    case TEXTURE_FORMAT_A1R5G5B5: dxgiFmt = DXGI_FORMAT_B5G5R5A1_UNORM; break;
    }

    D3D11_TEXTURE2D_DESC td{};
    td.Width              = w;
    td.Height             = h;
    td.MipLevels          = 1;
    td.ArraySize          = 1;
    td.Format             = dxgiFmt;
    td.SampleDesc.Count   = 1;
    td.Usage              = D3D11_USAGE_DEFAULT;
    td.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags     = 0;

    if (bits) {
        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem          = bits;
        init.SysMemPitch      = w * 4;
        init.SysMemSlicePitch = 0;
        if (FAILED(dev->rawDevice()->CreateTexture2D(&td, &init, &m_texture))) {
            MLOG_ERROR("[sprite] CreateTexture2D failed (%ux%u fmt=%u)", w, h, dxgiFmt);
            return false;
        }
    } else {
        if (FAILED(dev->rawDevice()->CreateTexture2D(&td, nullptr, &m_texture))) {
            MLOG_ERROR("[sprite] CreateTexture2D(empty) failed");
            return false;
        }
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    srvd.Format        = dxgiFmt;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(dev->rawDevice()->CreateShaderResourceView(m_texture.Get(), &srvd, &m_srv))) {
        MLOG_ERROR("[sprite] CreateShaderResourceView failed");
        return false;
    }
    return true;
}

SpriteObject* SpriteObject::create(Device* dev, std::uint32_t w, std::uint32_t h,
                                   TEXTURE_FORMAT fmt, const void* initialBits) {
    auto* s = new SpriteObject();
    if (!s->initialize(dev, w, h, fmt, initialBits)) {
        s->Release();
        return nullptr;
    }
    return s;
}

SpriteObject* SpriteObject::createFromFile(Device* dev, I4DyuchiFileStorage* storage,
                                           const char* szFileName, std::uint32_t /*dwFlag*/) {
    if (!storage || !szFileName) return nullptr;

    // Resolve file data via the FileStorage interface.
    void* fp = storage->FSOpenFile(const_cast<char*>(szFileName), 0 /*read*/);
    if (!fp) {
        MLOG_WARN("[sprite] FSOpenFile failed for '%s'", szFileName);
        return nullptr;
    }

    const std::uint32_t fileSize = storage->FSSeek(fp, 0, FSFILE_SEEK_END);
    storage->FSSeek(fp, 0, FSFILE_SEEK_SET);
    if (fileSize == 0) {
        storage->FSCloseFile(fp);
        MLOG_WARN("[sprite] empty file '%s'", szFileName);
        return nullptr;
    }
    std::vector<std::uint8_t> buf(fileSize);
    const std::uint32_t read = storage->FSRead(fp, buf.data(), fileSize);
    storage->FSCloseFile(fp);
    if (read != fileSize) {
        MLOG_WARN("[sprite] incomplete read for '%s' (%u/%u)", szFileName, read, fileSize);
        return nullptr;
    }
    LoadedTexture t = loadTextureFromMemory(buf.data(), read);
    fprintf(stderr,
            "[sprite] loaded %s %ux%u px=%zu first_px rgba=(%u,%u,%u,%u)",
            szFileName, t.width, t.height, t.pixels.size() / 4,
            static_cast<unsigned>(t.pixels[0]), static_cast<unsigned>(t.pixels[1]),
            static_cast<unsigned>(t.pixels[2]), static_cast<unsigned>(t.pixels[3]));
    if (t.pixels.empty()) return nullptr;
    return create(dev, t.width, t.height, TEXTURE_FORMAT_A8R8G8B8, t.pixels.data());
}

BOOL __stdcall SpriteObject::Draw(VECTOR2* pv2Scaling, float fRot, VECTOR2* pv2Trans,
                                  RECT* pRect, std::uint32_t dwColor, std::uint32_t /*dwFlag*/) {
    if (!m_dev || !m_srv) return FALSE;

    // Compute destination rect.
    float dstX = pv2Trans ? pv2Trans->x : 0.0f;
    float dstY = pv2Trans ? pv2Trans->y : 0.0f;
    float dstW = static_cast<float>(m_width);
    float dstH = static_cast<float>(m_height);
    if (pv2Scaling) { dstW *= pv2Scaling->x; dstH *= pv2Scaling->y; }

    // Source UV rect from pRect (pixels) 鈫?normalized.
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
    if (pRect) {
        u0 = static_cast<float>(pRect->left)   / static_cast<float>(m_width);
        v0 = static_cast<float>(pRect->top)    / static_cast<float>(m_height);
        u1 = static_cast<float>(pRect->right)  / static_cast<float>(m_width);
        v1 = static_cast<float>(pRect->bottom) / static_cast<float>(m_height);
    }

    // Coordinate space: image origin at top-left, +Y down.  Flip V to match DX11 UV.
    v0 = 1.0f - v0;
    v1 = 1.0f - v1;
    std::swap(v0, v1);

    PrimitiveDrawer drawer;
    drawer.initialize(m_dev);
    drawer.setViewProj(m_dev->viewProjMatrix());
    drawer.drawTexturedQuad(m_srv.Get(), dstX, dstY, dstW, dstH, u0, v0, u1, v1, dwColor, fRot, 0);
    return TRUE;
}

BOOL __stdcall SpriteObject::Resize(float fWidth, float fHeight) {
    // Re-allocate texture at new size, preserving aspect.
    if (!m_dev) return FALSE;
    std::uint32_t newW = static_cast<std::uint32_t>(std::max(1.0f, fWidth));
    std::uint32_t newH = static_cast<std::uint32_t>(std::max(1.0f, fHeight));

    D3D11_TEXTURE2D_DESC desc{};
    m_texture->GetDesc(&desc);
    desc.Width  = newW;
    desc.Height = newH;
    m_texture.Reset();
    m_srv.Reset();

    if (FAILED(m_dev->rawDevice()->CreateTexture2D(&desc, nullptr, &m_texture))) return FALSE;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    srvd.Format        = desc.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(m_dev->rawDevice()->CreateShaderResourceView(m_texture.Get(), &srvd, &m_srv))) return FALSE;

    m_width  = newW;
    m_height = newH;
    return TRUE;
}

BOOL __stdcall SpriteObject::GetImageHeader(IMAGE_HEADER* pImgHeader, std::uint32_t /*dwFrameIndex*/) {
    if (!pImgHeader) return FALSE;
    pImgHeader->dwWidth  = m_width;
    pImgHeader->dwHeight = m_height;
    pImgHeader->dwPitch  = m_width * 4;
    pImgHeader->dwBPS    = 32;
    return TRUE;
}

BOOL __stdcall SpriteObject::LockRect(LOCKED_RECT* pOutLockedRect, RECT* /*pRect*/, TEXTURE_FORMAT /*TexFormat*/) {
    // Phase 5 stub: real read-back staging texture would be needed.
    if (pOutLockedRect) {
        pOutLockedRect->Pitch = 0;
        pOutLockedRect->pBits = nullptr;
    }
    return FALSE;
}

BOOL __stdcall SpriteObject::UnlockRect() { return TRUE; }

} // namespace mxh::gx::dx11
