//--------------------------------------------------------------------------------------
// File: MSAAHelper.cpp
//
// Helper for managing MSAA render targets (DirectX 11 version)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "MSAAHelper.h"

#include "DirectXHelpers.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <utility>

#include <wrl/client.h>

using namespace DirectX;
using namespace DX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

using Microsoft::WRL::ComPtr;

MSAAHelper::MSAAHelper(DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    unsigned int sampleCount) noexcept(false) :
        m_backBufferFormat(backBufferFormat),
        m_depthBufferFormat(depthBufferFormat),
        m_targetSampleCount(sampleCount),
        m_width(0),
        m_height(0)
{
    if (sampleCount < 2 || sampleCount > D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT)
    {
        throw std::out_of_range("MSAA sample count invalid.");
    }
}


void MSAAHelper::SetDevice(_In_ ID3D11Device* device)
{
    if (device == m_device.Get())
        return;

    if (m_device)
    {
        ReleaseDevice();
    }

    {
        UINT formatSupport = 0;
        if (FAILED(device->CheckFormatSupport(m_backBufferFormat, &formatSupport)))
        {
            throw std::exception();
        }

        UINT32 required = D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET;
        if ((formatSupport & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "MSAAHelper: Device does not support MSAA for requested backbuffer format (%u)!\n", m_backBufferFormat);
            OutputDebugStringA(buff);
#endif
            throw std::exception();
        }
    }

    {
        UINT formatSupport = 0;
        if (FAILED(device->CheckFormatSupport(m_depthBufferFormat, &formatSupport)))
        {
            throw std::exception();
        }

        UINT32 required = D3D11_FORMAT_SUPPORT_DEPTH_STENCIL | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET;
        if ((formatSupport & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "MSAAHelper: Device does not support MSAA for requested depth/stencil format (%u)!\n", m_depthBufferFormat);
            OutputDebugStringA(buff);
#endif
            throw std::exception();
        }
    }

    for (m_sampleCount = m_targetSampleCount; m_sampleCount > 1; m_sampleCount--)
    {
        UINT levels = 0;
        if (FAILED(device->CheckMultisampleQualityLevels(m_backBufferFormat, m_sampleCount, &levels)))
            continue;

        if ( levels > 0)
            break;
    }

    if (m_sampleCount < 2)
    {
#ifdef _DEBUG
        OutputDebugStringA("MSAAHelper: Device does not support MSAA!\n");
#endif
        throw std::exception();
    }

    m_device = device;
}


void MSAAHelper::SizeResources(size_t width, size_t height)
{
    if (width == m_width && height == m_height)
        return;

    if (m_width > UINT32_MAX || m_height > UINT32_MAX)
    {
        throw std::out_of_range("Invalid width/height");
    }

    if (!m_device)
        return;

    m_width = m_height = 0;

    // Create an MSAA render target
    CD3D11_TEXTURE2D_DESC renderTargetDesc(
        m_backBufferFormat,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1, // The render target view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET,
        D3D11_USAGE_DEFAULT,
        0,
        m_sampleCount
    );

    ThrowIfFailed(m_device->CreateTexture2D(
        &renderTargetDesc,
        nullptr,
        m_msaaRenderTarget.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_msaaRenderTarget.Get(), "MSAA Render Target");


    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS, m_backBufferFormat);

    ThrowIfFailed(m_device->CreateRenderTargetView(
        m_msaaRenderTarget.Get(),
        &renderTargetViewDesc,
        m_renderTargetView.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_renderTargetView.Get(), "MSAA Render Target");

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Create an MSAA depth stencil view
        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            m_depthBufferFormat,
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            1, // This depth stencil view has only one texture.
            1, // Use a single mipmap level.
            D3D11_BIND_DEPTH_STENCIL,
            D3D11_USAGE_DEFAULT,
            0,
            m_sampleCount
        );

        ThrowIfFailed(m_device->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_msaaDepthStencil.ReleaseAndGetAddressOf()
        ));

        SetDebugObjectName(m_msaaDepthStencil.Get(), "MSAA Depth/Stencil");

        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS, m_depthBufferFormat);

        ThrowIfFailed(m_device->CreateDepthStencilView(
            m_msaaDepthStencil.Get(),
            &dsvDesc,
            m_depthStencilView.ReleaseAndGetAddressOf()
        ));

        SetDebugObjectName(m_depthStencilView.Get(), "MSAA Depth/Stencil");
    }

    m_width = width;
    m_height = height;
}


void MSAAHelper::ReleaseDevice()
{
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_msaaDepthStencil.Reset();
    m_msaaRenderTarget.Reset();

    m_device.Reset();

    m_width = m_height = 0;
}


void MSAAHelper::Resolve(_In_ ID3D11DeviceContext* context, _In_ ID3D11Texture2D* backBuffer)
{
    context->ResolveSubresource(backBuffer, 0, m_msaaRenderTarget.Get(), 0, m_backBufferFormat);
}


void MSAAHelper::SetWindow(const RECT& output)
{
    // Determine the render target size in pixels.
    auto width = size_t(std::max<LONG>(output.right - output.left, 1));
    auto height = size_t(std::max<LONG>(output.bottom - output.top, 1));

    SizeResources(width, height);
}
