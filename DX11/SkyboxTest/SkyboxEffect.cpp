//--------------------------------------------------------------------------------------
// SkyboxEffect.cpp
//
// A sky box effect for DirectX 11.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SkyboxEffect.h"

#include "DirectXHelpers.h"
#include "GraphicsMemory.h"

#include "ReadData.h"

#include <stdexcept>


using namespace DirectX;
using namespace DX;

namespace
{
    constexpr uint32_t DirtyConstantBuffer = 0x1;
    constexpr uint32_t DirtyWVPMatrix = 0x2;
}

SkyboxEffect::SkyboxEffect(_In_ ID3D11Device* device) :
        m_dirtyFlags(uint32_t(-1)),
        m_constantBuffer(device)
{
    static_assert((sizeof(SkyboxEffect::SkyboxEffectConstants) % 16) == 0, "CB size alignment");

    // Get shaders
    m_vsBlob = DX::ReadData(L"SkyboxEffect_VS.cso");

    DX::ThrowIfFailed(
        device->CreateVertexShader(m_vsBlob.data(), m_vsBlob.size(),
            nullptr, m_vs.ReleaseAndGetAddressOf()));

    auto psBlob = DX::ReadData(L"SkyboxEffect_PS.cso");

    DX::ThrowIfFailed(
        device->CreatePixelShader(psBlob.data(), psBlob.size(),
            nullptr, m_ps.ReleaseAndGetAddressOf()));
}


// IEffect methods.
void SkyboxEffect::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    if (m_dirtyFlags & DirtyWVPMatrix)
    {
#if 0
        XMMATRIX worldView = XMMatrixMultiply(m_world, m_view);
        m_worldViewProj = XMMatrixMultiply(worldView, m_proj);
#else
        // Skybox ignores m_world matrix and the translation of m_view
        XMMATRIX view = m_view;
        view.r[3] = g_XMIdentityR3;
        m_worldViewProj = XMMatrixMultiply(view, m_proj);
#endif

        m_dirtyFlags &= ~DirtyWVPMatrix;
        m_dirtyFlags |= DirtyConstantBuffer;
    }

    if (m_dirtyFlags & DirtyConstantBuffer)
    {
        SkyboxEffectConstants constants;
        constants.worldViewProj = XMMatrixTranspose(m_worldViewProj);
        m_constantBuffer.SetData(deviceContext, constants);

        m_dirtyFlags &= ~DirtyConstantBuffer;
    }

    auto cb = m_constantBuffer.GetBuffer();
    deviceContext->VSSetConstantBuffers(0, 1, &cb);
    deviceContext->PSSetShaderResources(0, 1, m_texture.GetAddressOf());

    deviceContext->VSSetShader(m_vs.Get(), nullptr, 0);
    deviceContext->PSSetShader(m_ps.Get(), nullptr, 0);
}

void SkyboxEffect::GetVertexShaderBytecode(
    _Out_ void const** pShaderByteCode,
    _Out_ size_t* pByteCodeLength)
{
    assert(pShaderByteCode != nullptr && pByteCodeLength != nullptr);
    *pShaderByteCode = m_vsBlob.data();
    *pByteCodeLength = m_vsBlob.size();
}


// Camera settings.
void SkyboxEffect::SetWorld(FXMMATRIX /*value*/)
{
#if 0
    m_world = value;
    m_dirtyFlags |= DirtyWVPMatrix;
#endif
}

void SkyboxEffect::SetView(FXMMATRIX value)
{
    m_view = value;
    m_dirtyFlags |= DirtyWVPMatrix;
}

void SkyboxEffect::SetProjection(FXMMATRIX value)
{
    m_proj = value;
    m_dirtyFlags |= DirtyWVPMatrix;
}

void SkyboxEffect::SetMatrices(FXMMATRIX /*world*/, CXMMATRIX view, CXMMATRIX projection)
{
#if 0
    m_world = world;
#endif
    m_view = view;
    m_proj = projection;
    m_dirtyFlags |= DirtyWVPMatrix;
}


// Texture settings.
void SkyboxEffect::SetTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    m_texture = value;
}
