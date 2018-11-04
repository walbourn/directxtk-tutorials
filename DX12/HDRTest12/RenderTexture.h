//--------------------------------------------------------------------------------------
// File: RenderTexture.h
//
// Helper for managing offscreen render targets
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace DX
{
    class RenderTexture
    {
    public:
        RenderTexture(DXGI_FORMAT format);

        RenderTexture(RenderTexture&&) = default;
        RenderTexture& operator= (RenderTexture&&) = default;

        RenderTexture(RenderTexture const&) = delete;
        RenderTexture& operator= (RenderTexture const&) = delete;

        void SetDevice(_In_ ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice();

        void TransitionTo(_In_ ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES afterState);

        void BeginScene(_In_ ID3D12GraphicsCommandList* commandList)
        {
            TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        void EndScene(_In_ ID3D12GraphicsCommandList* commandList)
        {
            TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        void SetClearColor(DirectX::FXMVECTOR color)
        {
            DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(m_clearColor), color);
        }

        void SetWindow(const RECT& rect);

        DXGI_FORMAT GetFormat() const { return m_format; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Device>                m_device;
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_resource;
        D3D12_RESOURCE_STATES                               m_state;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_srvDescriptor;
        D3D12_CPU_DESCRIPTOR_HANDLE                         m_rtvDescriptor;
        float                                               m_clearColor[4];

        DXGI_FORMAT                                         m_format;

        size_t                                              m_width;
        size_t                                              m_height;
    };
}