//--------------------------------------------------------------------------------------
// File: MSAAHelper.h
//
// Helper for managing MSAA render targets (DirectX 11 version)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <wrl/client.h>

namespace DX
{
    class MSAAHelper
    {
    public:
        explicit MSAAHelper(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
            unsigned int sampleCount = 4) noexcept(false);

        MSAAHelper(MSAAHelper&&) = default;
        MSAAHelper& operator= (MSAAHelper&&) = default;

        MSAAHelper(MSAAHelper const&) = delete;
        MSAAHelper& operator= (MSAAHelper const&) = delete;

        void SetDevice(_In_ ID3D11Device* device);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice();

        void Resolve(_In_ ID3D11DeviceContext* context, _In_ ID3D11Texture2D* backBuffer);

        void SetWindow(const RECT& rect);

        ID3D11Texture2D* GetMSAARenderTarget() const noexcept { return m_msaaRenderTarget.Get(); }
        ID3D11Texture2D* GetMSAADepthStencil() const noexcept { return m_msaaDepthStencil.Get(); }

        ID3D11RenderTargetView* GetMSAARenderTargetView() const noexcept { return m_renderTargetView.Get(); }
        ID3D11DepthStencilView* GetMSAADepthStencilView() const noexcept { return m_depthStencilView.Get(); }

        DXGI_FORMAT GetBackBufferFormat() const noexcept { return m_backBufferFormat; }
        DXGI_FORMAT GetDepthBufferFormat() const noexcept { return m_depthBufferFormat; }
        unsigned int GetSampleCount() const noexcept { return m_sampleCount; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Device>                m_device;

        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_msaaRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_msaaDepthStencil;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      m_depthStencilView;

        DXGI_FORMAT                                         m_backBufferFormat;
        DXGI_FORMAT                                         m_depthBufferFormat;
        unsigned int                                        m_sampleCount;
        unsigned int                                        m_targetSampleCount;

        size_t                                              m_width;
        size_t                                              m_height;
    };
}
