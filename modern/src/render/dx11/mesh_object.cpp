// mxh/render/dx11/mesh_object.cpp
// IDIMeshObject DX11 implementation.
#include "mesh_object.hpp"
#include "device.hpp"
#include "effect_shader.hpp"
#include "mesh_shaders.hpp"
#include "texture_loader.hpp"

#include "mxh/log/mlog.hpp"

namespace mxh::gx::dx11 {

MeshObject::~MeshObject() {
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
}

STDMETHODIMP MeshObject::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IDIMeshObject*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MeshObject::AddRef() { return ++m_refCount; }
STDMETHODIMP_(ULONG) MeshObject::Release() {
    ULONG r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

bool MeshObject::finalizeVB() {
    if (!m_dev) return false;
    auto* dev = m_dev->rawDevice();

    D3D11_BUFFER_DESC vbd{};
    vbd.Usage          = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth      = static_cast<UINT>(m_vertices.size() * sizeof(Vertex));
    vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA vinit{};
    vinit.pSysMem = m_vertices.data();
    if (FAILED(dev->CreateBuffer(&vbd, &vinit, &m_vertexBuffer))) {
        MLOG_ERROR("[mesh] CreateBuffer VB failed");
        return false;
    }

    D3D11_BUFFER_DESC ibd{};
    ibd.Usage          = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth      = static_cast<UINT>(m_indices.size() * sizeof(std::uint16_t));
    ibd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA iinit{};
    iinit.pSysMem = m_indices.data();
    if (FAILED(dev->CreateBuffer(&ibd, &iinit, &m_indexBuffer))) {
        MLOG_ERROR("[mesh] CreateBuffer IB failed");
        return false;
    }

    m_vertexCount = static_cast<std::uint32_t>(m_vertices.size());
    m_indexCount  = static_cast<std::uint32_t>(m_indices.size());
    return true;
}

MeshObject* MeshObject::createEmpty(Device* dev, CMeshFlag /*flag*/) {
    auto* m = new MeshObject();
    m->m_dev = dev;
    return m;
}

bool MeshObject::initializeCube(Device* dev, ID3D11ShaderResourceView* diffuseSRV) {
    m_dev = dev;
    // 8 cube vertices, 12 triangles (6 faces × 2), 36 indices.
    // Each face has its own normal so we duplicate vertices per face (24 verts total).
    struct Face {
        float nx, ny, nz;
        float ux, uy;
    };
    Face faces[6] = {
        {  0,  0, -1, 0, 1 },  // back
        {  0,  0,  1, 0, 0 },  // front
        { -1,  0,  0, 1, 1 },  // left
        {  1,  0,  0, 0, 1 },  // right
        {  0,  1,  0, 0, 1 },  // top
        {  0, -1,  0, 0, 0 },  // bottom
    };
    float corners[8][3] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    };
    int faceQuads[6][4] = {
        { 0, 1, 2, 3 },  // back  z-
        { 5, 4, 7, 6 },  // front z+
        { 4, 0, 3, 7 },  // left  x-
        { 1, 5, 6, 2 },  // right x+
        { 3, 2, 6, 7 },  // top   y+
        { 4, 5, 1, 0 },  // bot   y-
    };

    m_vertices.clear();
    m_indices.clear();
    for (int f = 0; f < 6; ++f) {
        std::uint16_t base = static_cast<std::uint16_t>(m_vertices.size());
        for (int v = 0; v < 4; ++v) {
            int ci = faceQuads[f][v];
            Vertex vert{};
            vert.x = corners[ci][0];
            vert.y = corners[ci][1];
            vert.z = corners[ci][2];
            vert.nx = faces[f].nx; vert.ny = faces[f].ny; vert.nz = faces[f].nz;
            vert.u = (v == 1 || v == 2) ? 1.0f : 0.0f;
            vert.v = (v == 2 || v == 3) ? 1.0f : 0.0f;
            m_vertices.push_back(vert);
        }
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 3);
    }
    if (!finalizeVB()) return false;

    // Single face group covering the whole cube.
    FaceGroup fg{};
    fg.startIndex = 0;
    fg.indexCount = static_cast<std::uint32_t>(m_indices.size());
    fg.mtlIndex   = 0;
    fg.diffuseSRV = diffuseSRV;
    m_faceGroups.push_back(fg);
    return true;
}

MeshObject* MeshObject::createFromFile(Device* dev, I4DyuchiFileStorage* storage, const char* path) {
    if (!dev || !storage || !path) return nullptr;
    // Phase 5: .chx loader not implemented yet; placeholder for unit-cube test.
    void* fp = storage->FSOpenFile(const_cast<char*>(path), 0);
    if (!fp) {
        MLOG_WARN("[mesh] FSOpenFile failed for '%s'", path);
        return nullptr;
    }
    storage->FSCloseFile(fp);
    auto* m = new MeshObject();
    m->m_dev = dev;
    return m;
}

BOOL __stdcall MeshObject::StartInitialize(MESH_DESC* pDesc, IGeometryController* /*pCtrl*/,
                                            IGeometryControllerStatic* /*pCtrlStatic*/) {
    if (!m_dev || !pDesc || !pDesc->pv3WorldList) return FALSE;
    m_vertices.clear();
    m_vertices.resize(pDesc->dwVertexNum);
    for (std::uint32_t i = 0; i < pDesc->dwVertexNum; ++i) {
        m_vertices[i].x = pDesc->pv3WorldList[i].x;
        m_vertices[i].y = pDesc->pv3WorldList[i].y;
        m_vertices[i].z = pDesc->pv3WorldList[i].z;
        m_vertices[i].u = (pDesc->ptvTexCoordList && i < pDesc->dwTexVertexNum) ? pDesc->ptvTexCoordList[i].u : 0.0f;
        m_vertices[i].v = (pDesc->ptvTexCoordList && i < pDesc->dwTexVertexNum) ? pDesc->ptvTexCoordList[i].v : 0.0f;
        m_vertices[i].nx = m_vertices[i].ny = 0.0f;
        m_vertices[i].nz = 1.0f;
        if (pDesc->pv3NormalLocal) {
            m_vertices[i].nx = pDesc->pv3NormalLocal[i].x;
            m_vertices[i].ny = pDesc->pv3NormalLocal[i].y;
            m_vertices[i].nz = pDesc->pv3NormalLocal[i].z;
        }
    }
    m_indexCount = 0;
    return TRUE;
}

void __stdcall MeshObject::EndInitialize() {
    if (m_vertices.empty()) return;
    finalizeVB();
}

BOOL __stdcall MeshObject::InsertFaceGroup(FACE_DESC* pDesc) {
    if (!pDesc || !pDesc->pIndex || pDesc->dwFacesNum == 0) return FALSE;
    std::uint32_t baseIdx = static_cast<std::uint32_t>(m_indices.size());
    for (std::uint32_t i = 0; i < pDesc->dwFacesNum * 3; ++i) {
        m_indices.push_back(pDesc->pIndex[i]);
    }
    FaceGroup fg{};
    fg.startIndex = baseIdx;
    fg.indexCount = pDesc->dwFacesNum * 3;
    fg.mtlIndex   = pDesc->dwMtlIndex;
    m_faceGroups.push_back(fg);
    return TRUE;
}

BOOL __stdcall MeshObject::Render(std::uint32_t /*refIdx*/, std::uint32_t /*alpha*/,
                                   LIGHT_INDEX_DESC* /*pDyn*/, std::uint32_t /*dynNum*/,
                                   LIGHT_INDEX_DESC* /*pSpot*/, std::uint32_t /*spotNum*/,
                                   std::uint32_t /*mtlSet*/, std::uint32_t /*effect*/, std::uint32_t /*flag*/) {
    // Direct rendering is performed by CoD3DDeviceDX11::RenderMeshObject, which
    // calls into MeshDrawer (set up by the renderer). This method is a stub for
    // compatibility with Executive-driven render paths.
    return FALSE;
}

BOOL __stdcall MeshObject::RenderProjection(std::uint32_t, std::uint32_t, std::uint8_t*,
                                             std::uint32_t, std::uint32_t) { return FALSE; }
BOOL __stdcall MeshObject::Update(std::uint32_t) { return TRUE; }
void __stdcall MeshObject::DisableUpdate() {}

void MeshObject::setDiffuseSRV(std::uint32_t groupIndex, ID3D11ShaderResourceView* srv) {
    if (groupIndex >= m_faceGroups.size()) return;
    m_faceGroups[groupIndex].diffuseSRV = srv;
}

void MeshObject::setRenderer(Device* dev) { m_dev = dev; }

void MeshObject::setEffectPalette(EffectShaderPalette* palette) { m_effectPalette = palette; }

void MeshObject::RenderEffect(ID3D11PixelShader* psEffect, const MATRIX4* matWorld,
                               EffectEntry* pEffect, std::uint32_t /*alpha*/) {
    if (!m_dev || !psEffect || !pEffect || !pEffect->bSuccess) return;

    auto* dev11 = m_dev->rawDevice();
    auto* ctx   = m_dev->rawContext();
    if (!dev11 || !ctx) return;

    if (!m_vertexBuffer || !m_indexBuffer || m_faceGroups.empty()) return;

    // Set vertex buffer.
    constexpr UINT vertexStride = sizeof(Vertex);
    constexpr UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &offset);
    ctx->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set effect pixel shader (dot3 + effect texture on t2).
    ctx->PSSetShader(psEffect, nullptr, 0);

    // Set effect SRV on texture slot 2 (matches original: SetTexture(..., 2)).
    if (pEffect->srv) {
        ctx->PSSetShaderResources(2, 1, pEffect->srv.GetAddressOf());
    }

    // Set texture matrix: sphere-map base + wave offset.
    const MATRIX4 identity = MatrixIdentity();
    const MATRIX4* matWorldIn = matWorld ? matWorld : &identity;
    const MATRIX4* viewMat = &m_dev->viewMatrix();
    MATRIX4 matTex;
    m_effectPalette->setSphereMapMatrix(&matTex, matWorldIn, viewMat);

    if (pEffect->method == TEXGEN_METHOD_WAVE && m_effectPalette) {
        MATRIX4 matWave;
        m_effectPalette->setWaveTexMatrix(&matWave);
        MATRIX4 matTmp;
        MatrixMultiply2(&matTmp, &matTex, &matWave);
        matTex = matTmp;
    }

    // Upload texture matrix to VS constant slot 25 (matches original engine).
    ctx->VSSetConstantBuffers(25, 1, &m_texMatrixBuffer);
    if (!m_texMatrixBuffer) {
        D3D11_BUFFER_DESC bd{};
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth      = sizeof(MATRIX4);
        bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev11->CreateBuffer(&bd, nullptr, &m_texMatrixBuffer);
    }
    if (m_texMatrixBuffer) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        ctx->Map(m_texMatrixBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (mapped.pData) {
            std::memcpy(mapped.pData, &matTex, sizeof(MATRIX4));
            ctx->Unmap(m_texMatrixBuffer.Get(), 0);
        }
    }
    ctx->VSSetConstantBuffers(25, 1, m_texMatrixBuffer.GetAddressOf());

    // Draw each face group with the effect SRV.
    for (auto& fg : m_faceGroups) {
        ctx->DrawIndexed(fg.indexCount, fg.startIndex, 0);
    }

    // Unbind effect SRV to avoid leaving stale bindings.
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(2, 1, &nullSRV);
}

} // namespace mxh::gx::dx11