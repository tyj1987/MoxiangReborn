// mxh/render/dx11/mesh_shaders.hpp
// Lit HLSL shaders for 3D mesh rendering: per-pixel directional light + texture.
#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace mxh::gx::dx11 {

struct MeshShaders {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vsLit;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  psLit;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  psEffect;     // dot3 + effect texture (slot t2)
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  ilLit;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbWorld;       // world matrix
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbViewProj;    // view*proj
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbLight;       // light + ambient

    bool init(ID3D11Device* device);
    void release();
};

} // namespace mxh::gx::dx11