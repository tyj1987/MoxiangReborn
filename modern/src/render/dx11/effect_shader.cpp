// mxh/render/dx11/effect_shader.cpp
// EffectShaderPalette DX11 implementation.
#include "effect_shader.hpp"
#include "device.hpp"
#include "texture_loader.hpp"

#include "mxh/log/mlog.hpp"

#include <algorithm>
#include <cmath>

namespace mxh::gx::dx11 {

EffectShaderPalette::EffectShaderPalette(Device* dev) : m_dev(dev) {}

EffectShaderPalette::~EffectShaderPalette() { clear(); }

void EffectShaderPalette::clear() {
    m_entries.clear();
}

EffectEntry* EffectShaderPalette::getEffect(std::uint32_t index) {
    if (index >= m_entries.size()) return nullptr;
    return &m_entries[index];
}

bool EffectShaderPalette::buildFromDesc(const CUSTOM_EFFECT_DESC* descs,
                                         std::uint32_t count) {
    if (!descs || count == 0) return false;
    clear();
    m_entries.resize(count);

    for (std::uint32_t i = 0; i < count; ++i) {
        const CUSTOM_EFFECT_DESC& src = descs[i];
        EffectEntry& dst = m_entries[i];
        dst.method         = src.method;
        dst.bDisableSrcTex = src.bDisableSrcTex;
        dst.dwFlag         = src.dwFlag;

        // Load texture from file storage.
        if (src.szTexName[0] != '\0') {
            if (auto* raw = m_dev->createTextureFromFile(src.szTexName)) {
                dst.srv = raw;
                dst.bSuccess = TRUE;
            } else {
                MLOG_WARN("[effect] failed to load texture '%s' for effect %u", src.szTexName, i);
                dst.bSuccess = FALSE;
            }
        } else {
            dst.bSuccess = FALSE;
        }
    }
    MLOG_INFO("[effect] palette built: %u entries", count);
    return true;
}

bool EffectShaderPalette::loadFromFile(const char* fileName) {
    if (!fileName || !m_dev) return false;

    // Binary format: [DWORD version][DWORD count][CUSTOM_EFFECT_DESC[count]].
    // We read raw bytes through the file storage.
    auto* storage = m_dev->fileStorage();
    if (!storage) return false;

    void* fp = storage->FSOpenFile(const_cast<char*>(fileName), FSFILE_ACCESSMODE_BINARY);
    if (!fp) {
        MLOG_WARN("[effect] cannot open file '%s'", fileName);
        return false;
    }

    std::uint32_t version = 0;
    std::uint32_t count   = 0;
    storage->FSRead(fp, &version, sizeof(std::uint32_t));
    storage->FSRead(fp, &count,   sizeof(std::uint32_t));

    if (count > 256) {
        storage->FSCloseFile(fp);
        return false;
    }

    std::vector<CUSTOM_EFFECT_DESC> descs(count);
    storage->FSRead(fp, descs.data(), sizeof(CUSTOM_EFFECT_DESC) * count);
    storage->FSCloseFile(fp);

    return buildFromDesc(descs.data(), count);
}

void EffectShaderPalette::setSphereMapMatrix(MATRIX4* pResultMat,
                                              const MATRIX4* pMatWorld,
                                              const MATRIX4* pMatView) const {
    if (!pResultMat || !pMatWorld || !pMatView) return;

    // matTex = matWorld × matView
    MatrixMultiply2(pResultMat, pMatWorld, pMatView);

    // Scale ×0.5 / -0.5, zero translation, then translate +0.5
    pResultMat->_11 *= 0.5f;   pResultMat->_21 *= 0.5f;   pResultMat->_31 *= 0.5f;
    pResultMat->_12 *= -0.5f;  pResultMat->_22 *= -0.5f;  pResultMat->_32 *= -0.5f;

    pResultMat->_14 = 0.0f;
    pResultMat->_24 = 0.0f;
    pResultMat->_34 = 0.0f;

    pResultMat->_41 = 0.5f;
    pResultMat->_42 = 0.5f;
    pResultMat->_43 = 0.0f;
    pResultMat->_44 = 1.0f;
}

void EffectShaderPalette::setWaveTexMatrix(MATRIX4* pMatResult) const {
    if (!pMatResult) return;

    setIdentityMatrix(pMatResult);

    float fXTrans = std::sin(static_cast<float>(m_tickCount) / 1000.0f);
    pMatResult->_31 = fXTrans; // Row 3, Col 1

    float fYTrans = std::sin(static_cast<float>(m_tickCount) / 1250.0f);
    pMatResult->_32 = fYTrans; // Row 3, Col 2
}

} // namespace mxh::gx::dx11
