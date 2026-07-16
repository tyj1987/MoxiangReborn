// primitives_3d_shader_test.cpp - R-9.x shader source validation.
//
// The 3D solid VS (kVS_Solid3D) is the new addition for the
// drawBox 3D upgrade. This test compiles the HLSL source via the
// D3D compiler and verifies the input signature contains the
// expected elements (POSITION as float3 + COLOR0 as float4),
// and the cbuffer binding is correct.
//
// The test does NOT require a D3D11 device — D3DCompile from
// d3dcompiler.h works offline and returns a compiled blob + an
// ID3D11ShaderReflection that lets us inspect the input signature
// and cbuffer layout. This catches shader source bugs at unit-
// test time rather than at first GPU draw.

#include "primitives_shader_source.hpp"

#include <d3dcompiler.h>
#include <d3d11shader.h>

#include <wrl/client.h>

#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace mxh::gx::dx11::test {

namespace {

// Compile a VS source to a blob + reflect its input signature.
// Returns nullptr on failure; on success, out_signature holds
// the input element descriptions (caller must Release each).
ID3DBlob* CompileVS(const char* source) {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> err;
    // D3DCOMPILE_OPTIMIZATION_LEVEL0 keeps the cbuffer
    // reflection data intact (D3DCOMPILE_OPTIMIZATION_LEVEL3
    // strips unused cbuffers, which can hide the viewProj
    // binding from reflection).
    HRESULT hr = D3DCompile(source, std::strlen(source),
                           "shader_source", nullptr, nullptr,
                           "main", "vs_4_0",
                           D3DCOMPILE_OPTIMIZATION_LEVEL0, 0,
                           &blob, &err);
    if (FAILED(hr)) {
        ADD_FAILURE() << "D3DCompile failed: "
                      << (err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
        return nullptr;
    }
    // Return a copy (the ComPtr would release the blob on
    // scope exit otherwise).
    Microsoft::WRL::ComPtr<ID3DBlob> copy;
    if (FAILED(D3DCreateBlob(blob->GetBufferSize(), &copy))) return nullptr;
    std::memcpy(copy->GetBufferPointer(), blob->GetBufferPointer(),
                blob->GetBufferSize());
    copy->AddRef();
    return copy.Get();
}

}  // namespace

// ===========================================================================
// kVS_Solid2D (legacy, used by 2D draw methods)
// ===========================================================================

TEST(Primitives3DShader, Solid2DVSCompiles) {
    ID3DBlob* blob = CompileVS(kVS_Solid2D);
    ASSERT_NE(blob, nullptr);
    blob->Release();
}

TEST(Primitives3DShader, Solid2DVSInputSignature) {
    // 1:1 quirk: kVS_Solid2D's input position is float2 (not
    // float3) — the 2D draw methods use screen-space
    // coordinates with the Z component implicit 0.
    ID3DBlob* blob = CompileVS(kVS_Solid2D);
    ASSERT_NE(blob, nullptr);

    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> refl;
    ASSERT_HRESULT_SUCCEEDED(D3DReflect(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&refl)));

    D3D11_SHADER_DESC desc{};
    refl->GetDesc(&desc);
    EXPECT_EQ(desc.InputParameters, 2u);

    // The input elements should be POSITION (float2) + COLOR0 (float4).
    bool found_pos2d = false;
    bool found_color  = false;
    for (UINT i = 0; i < desc.InputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC p{};
        refl->GetInputParameterDesc(i, &p);
        std::string name(p.SemanticName ? p.SemanticName : "");
        if (name == "POSITION") {
            // Mask = 3 = XY (2 components)
            EXPECT_EQ(p.Mask, 3u) << "POSITION should have XY mask (2 components)";
            EXPECT_EQ(p.ComponentType, D3D_REGISTER_COMPONENT_FLOAT32);
            found_pos2d = true;
        } else if (name == "COLOR") {
            // Mask = 15 = XYZW (4 components)
            EXPECT_EQ(p.Mask, 15u);
            EXPECT_EQ(p.ComponentType, D3D_REGISTER_COMPONENT_FLOAT32);
            found_color = true;
        }
    }
    EXPECT_TRUE(found_pos2d) << "POSITION element not found";
    EXPECT_TRUE(found_color) << "COLOR0 element not found";
    blob->Release();
}

// ===========================================================================
// kVS_Solid3D (R-9.x, used by drawBox)
// ===========================================================================

TEST(Primitives3DShader, Solid3DVSCompiles) {
    // R-9.x: the new 3D solid VS must compile without errors.
    // This is the core validation — if kVS_Solid3D source
    // has a syntax error or missing binding, this fails
    // before any GPU draw ever happens.
    ID3DBlob* blob = CompileVS(kVS_Solid3D);
    ASSERT_NE(blob, nullptr);
    blob->Release();
}

TEST(Primitives3DShader, Solid3DVSInputSignature) {
    // R-9.x: kVS_Solid3D's input position is float3 (not
    // float2 like the 2D VS). The MASK field is 7 (binary
    // 0111) = XYZ (3 components), where the 2D VS uses
    // MASK=3 = XY.
    ID3DBlob* blob = CompileVS(kVS_Solid3D);
    ASSERT_NE(blob, nullptr);

    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> refl;
    ASSERT_HRESULT_SUCCEEDED(D3DReflect(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&refl)));

    D3D11_SHADER_DESC desc{};
    refl->GetDesc(&desc);
    EXPECT_EQ(desc.InputParameters, 2u);

    bool found_pos3d = false;
    bool found_color  = false;
    for (UINT i = 0; i < desc.InputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC p{};
        refl->GetInputParameterDesc(i, &p);
        std::string name(p.SemanticName ? p.SemanticName : "");
        if (name == "POSITION") {
            // R-9.x: 3D VS uses XYZ mask (binary 0111 = 7),
            // not the XY mask (3) of the 2D VS. This is
            // the key signature difference that the
            // 3D input layout (R32G32B32_FLOAT) requires.
            EXPECT_EQ(p.Mask, 7u) << "POSITION should have XYZ mask (3 components) for R-9.x 3D VS";
            EXPECT_EQ(p.ComponentType, D3D_REGISTER_COMPONENT_FLOAT32);
            found_pos3d = true;
        } else if (name == "COLOR") {
            EXPECT_EQ(p.Mask, 15u);
            EXPECT_EQ(p.ComponentType, D3D_REGISTER_COMPONENT_FLOAT32);
            found_color = true;
        }
    }
    EXPECT_TRUE(found_pos3d) << "POSITION element not found";
    EXPECT_TRUE(found_color) << "COLOR0 element not found";
    blob->Release();
}

TEST(Primitives3DShader, Solid3DVSHasViewProjCBuffer) {
    // R-9.x: the 3D solid VS must bind a cbuffer at register
    // b0 named "CBuf" (or compatible) with at least one mat4
    // variable named "viewProj" (or compatible). This is the
    // cbuffer that the CPU side updates via
    // PrimitiveDrawer::updateViewProj. If the binding is
    // missing or renamed, the draw call would silently
    // project with garbage.
    ID3DBlob* blob = CompileVS(kVS_Solid3D);
    ASSERT_NE(blob, nullptr);

    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> refl;
    ASSERT_HRESULT_SUCCEEDED(D3DReflect(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&refl)));

    D3D11_SHADER_DESC desc{};
    refl->GetDesc(&desc);
    bool found_viewproj = false;
    for (UINT i = 0; i < desc.ConstantBuffers; ++i) {
        ID3D11ShaderReflectionConstantBuffer* cb = refl->GetConstantBufferByIndex(i);
        D3D11_SHADER_BUFFER_DESC cbDesc{};
        HRESULT hr = cb->GetDesc(&cbDesc);
        if (FAILED(hr)) continue;
        for (UINT j = 0; j < cbDesc.Variables; ++j) {
            ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
            D3D11_SHADER_VARIABLE_DESC varDesc{};
            var->GetDesc(&varDesc);
            std::string name(varDesc.Name ? varDesc.Name : "");
            if (name == "viewProj") {
                ID3D11ShaderReflectionType* type = var->GetType();
                D3D11_SHADER_TYPE_DESC typeDesc{};
                type->GetDesc(&typeDesc);
                EXPECT_EQ(typeDesc.Class, D3D_SVC_MATRIX_COLUMNS);
                EXPECT_EQ(typeDesc.Type, D3D_SVT_FLOAT);
                EXPECT_EQ(typeDesc.Rows, 4u);
                EXPECT_EQ(typeDesc.Columns, 4u);
                // Elements = 0 means scalar (not array).
                // Elements > 0 means array of N. For a single
                // mat4 viewProj, Elements should be 0.
                EXPECT_EQ(typeDesc.Elements, 0u)
                    << "viewProj should be a single mat4, not an array";
                found_viewproj = true;
            }
        }
    }
    // Diagnostic: if the test fails, print all cbuffer
    // names + variable names so we can see what D3DCompile
    // kept.
    if (!found_viewproj) {
        for (UINT i = 0; i < desc.ConstantBuffers; ++i) {
            ID3D11ShaderReflectionConstantBuffer* cb = refl->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC cbDesc{};
            if (FAILED(cb->GetDesc(&cbDesc))) continue;
            ADD_FAILURE() << "cbuffer[" << i << "] name=" << (cbDesc.Name ? cbDesc.Name : "?")
                          << " variables=" << cbDesc.Variables;
            for (UINT j = 0; j < cbDesc.Variables; ++j) {
                ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
                D3D11_SHADER_VARIABLE_DESC varDesc{};
                if (FAILED(var->GetDesc(&varDesc))) continue;
                ADD_FAILURE() << "  variable[" << j << "] name=" << (varDesc.Name ? varDesc.Name : "?");
            }
        }
        ADD_FAILURE() << "desc.ConstantBuffers=" << desc.ConstantBuffers;
    }
    EXPECT_TRUE(found_viewproj) << "viewProj matrix not found in any cbuffer";
    blob->Release();
}

// ===========================================================================
// kPS_Solid (shared by 2D + 3D VS)
// ===========================================================================

TEST(Primitives3DShader, SolidPSCompiles) {
    // The solid PS is shared by both 2D and 3D draw paths.
    // Verify it compiles for ps_4_0.
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> err;
    HRESULT hr = D3DCompile(kPS_Solid, std::strlen(kPS_Solid),
                           "shader_source", nullptr, nullptr,
                           "main", "ps_4_0",
                           D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
                           &blob, &err);
    if (FAILED(hr)) {
        ADD_FAILURE() << "Solid PS compile failed: "
                      << (err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
    }
    EXPECT_TRUE(SUCCEEDED(hr));
}

}  // namespace mxh::gx::dx11::test
