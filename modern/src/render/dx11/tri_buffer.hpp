// mxh/render/dx11/tri_buffer.hpp
// TriBuffer: per-frame dynamic triangle rendering system.
// AllocRenderTriBuffer creates a D3D11 vertex (+ optional index) buffer.
// EnableRenderTriBuffer sets it as active IA; DisableRenderTriBuffer restores defaults.
// FreeRenderTriBuffer releases the underlying buffers.
#pragma once

#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>
#include "mxh/render/render_typedef.hpp"

namespace mxh::gx::dx11 {

struct TriBuffer {
    std::uint64_t        magic = TRI_BUFFER_MAGIC;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vb;
    Microsoft::WRL::ComPtr<ID3D11Buffer> ib;  // null if non-indexed
    std::uint32_t        vertexCount = 0;
    std::uint32_t        indexCount  = 0;   // 0 means non-indexed draw
    void*               mtlHandle = nullptr;  // material handle (opaque)
    std::uint32_t        faceCount = 0;
    bool                indexed = false;
};

} // namespace mxh::gx::dx11
