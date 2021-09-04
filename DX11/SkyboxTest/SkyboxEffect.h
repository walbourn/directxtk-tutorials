//--------------------------------------------------------------------------------------
// SkyboxEffect.h
//
// A sky box effect for DirectX 11.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <BufferHelpers.h>
#include <Effects.h>
#include <SimpleMath.h>

#include <vector>


namespace DX
{
    class SkyboxEffect : public DirectX::IEffect, public DirectX::IEffectMatrices
    {
    public:
        explicit SkyboxEffect(_In_ ID3D11Device* device);

        SkyboxEffect(SkyboxEffect&&) = default;
        SkyboxEffect& operator= (SkyboxEffect&&) = default;

        SkyboxEffect(SkyboxEffect const&) = delete;
        SkyboxEffect& operator= (SkyboxEffect const&) = delete;

        // IEffect methods.
        virtual void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;
        virtual void __cdecl GetVertexShaderBytecode(
            _Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        // Texture settings.
        void __cdecl SetTexture(_In_opt_ ID3D11ShaderResourceView* value);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_ps;

        uint32_t m_dirtyFlags;

#if 0
        DirectX::SimpleMath::Matrix m_world;
#endif
        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_worldViewProj;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_texture;
        std::vector<uint8_t>                                m_vsBlob;

        struct __declspec(align(16)) SkyboxEffectConstants
        {
            DirectX::XMMATRIX worldViewProj;
        };

        DirectX::ConstantBuffer<SkyboxEffectConstants>      m_constantBuffer;
    };
}
