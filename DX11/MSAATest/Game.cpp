//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() :
    m_window(0),
    m_outputWidth(800),
    m_outputHeight(600),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1),
    m_outputSize{}
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateDevice();

    CreateResources();
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float time = float(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationZ(cosf(time/2.f));
}

// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    auto customState = [=]()
    {
        m_d3dContext->RSSetState(m_rasterState.Get());
    };

    m_shape->Draw(m_world, m_view, m_proj, Colors::White, nullptr, true, customState);
    m_shape2->Draw(m_world, m_view, m_proj, Colors::White, nullptr, true, customState);

#if 1
    // --- MSAA ---
    m_d3dContext->ResolveSubresource(m_resovleMSAA.Get(), 0, m_renderTargetMSAA.Get(), 0, m_resolveFormat);

#if 0
    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);

    m_batch->SetViewport(viewport);

    m_batch->Begin();
    m_batch->Draw(m_resolveMSAASRV.Get(), m_outputSize);
    m_batch->End();
#else
    m_d3dContext->CopyResource(m_renderTarget.Get(), m_resovleMSAA.Get());
#endif

#endif

    Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
#if 1
    XMVECTORF32 clr;
    clr.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
    m_d3dContext->ClearRenderTargetView(m_renderTargetViewMSAA.Get(), clr);
    m_d3dContext->ClearDepthStencilView(m_depthStencilViewMSAA.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetViewMSAA.GetAddressOf(), m_depthStencilViewMSAA.Get());
#else
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
#endif

    // Set the viewport.
#if 0
    UINT msaaBackBufferWidth = 640;
    UINT msaaBackBufferHeight = 480;

    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(msaaBackBufferWidth), static_cast<float>(msaaBackBufferHeight));
    m_d3dContext->RSSetViewports(1, &viewport);

#else
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
#endif
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    HRESULT hr = D3D11CreateDevice(
        nullptr,                                // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        m_d3dDevice.ReleaseAndGetAddressOf(),   // returns the Direct3D device created
        &m_featureLevel,                        // returns feature level of device created
        m_d3dContext.ReleaseAndGetAddressOf()   // returns the device immediate context
        );

    if (hr == E_INVALIDARG)
    {
        // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it.
        hr = D3D11CreateDevice(nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            &featureLevels[1],
            _countof(featureLevels) - 1,
            D3D11_SDK_VERSION,
            m_d3dDevice.ReleaseAndGetAddressOf(),
            &m_featureLevel,
            m_d3dContext.ReleaseAndGetAddressOf()
            );
    }

    DX::ThrowIfFailed(hr);

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(m_d3dDevice.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
            };
            D3D11_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    // DirectX 11.1 if present
    if (SUCCEEDED(m_d3dDevice.As(&m_d3dDevice1)))
        (void)m_d3dContext.As(&m_d3dContext1);

    m_shape = GeometricPrimitive::CreateTorus(m_d3dContext.Get(), 2.f, 1.f, 64);
    m_shape2 = GeometricPrimitive::CreateSphere(m_d3dContext.Get(), 2.f, 32);

    m_batch = std::make_unique<SpriteBatch>(m_d3dContext.Get());

    m_world = Matrix::Identity;

    CD3D11_RASTERIZER_DESC rastDesc(D3D11_FILL_WIREFRAME, D3D11_CULL_NONE, FALSE, D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP, D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE, TRUE, FALSE);
    DX::ThrowIfFailed(m_d3dDevice->CreateRasterizerState(&rastDesc, m_rasterState.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews [] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_renderTarget.Reset();
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();

#if 1
    // --- MSAA ---
    m_resovleMSAA.Reset();
    m_renderTargetViewMSAA.Reset();
    m_depthStencilViewMSAA.Reset();
#endif

    m_d3dContext->Flush();

    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory1> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        ComPtr<IDXGIFactory2> dxgiFactory2;
        if (SUCCEEDED(dxgiFactory.As(&dxgiFactory2)))
        {
            // DirectX 11.1 or later

            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
            swapChainDesc.Width = backBufferWidth;
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = backBufferCount;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
            fsSwapChainDesc.Windowed = TRUE;

            // Create a SwapChain from a Win32 window.
            DX::ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(
                m_d3dDevice.Get(),
                m_window,
                &swapChainDesc,
                &fsSwapChainDesc,
                nullptr,
                m_swapChain1.ReleaseAndGetAddressOf()
                ));

            DX::ThrowIfFailed(m_swapChain1.As(&m_swapChain));
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
            swapChainDesc.BufferCount = backBufferCount;
            swapChainDesc.BufferDesc.Width = backBufferWidth;
            swapChainDesc.BufferDesc.Height = backBufferHeight;
            swapChainDesc.BufferDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.OutputWindow = m_window;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Windowed = TRUE;

            DX::ThrowIfFailed(dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf()));
        }

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(m_renderTarget.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));


#if 1
    // --- MSAA ---
#if 1
    DXGI_FORMAT msaaBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    DXGI_FORMAT msaaBackBufferFormat = backBufferFormat;
#endif

#if 0
    UINT msaaBackBufferWidth = 640;
    UINT msaaBackBufferHeight = 480;
#else
    UINT msaaBackBufferWidth = backBufferWidth;
    UINT msaaBackBufferHeight = backBufferHeight;
#endif


    UINT msaaCount = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
    for (; msaaCount > 1; --msaaCount)
    {
        UINT numQualityLevels = 0;
        DX::ThrowIfFailed(m_d3dDevice->CheckMultisampleQualityLevels(msaaBackBufferFormat, msaaCount, &numQualityLevels));
        if (numQualityLevels > 0)
            break;
    }

    if (msaaCount <= 1)
    {
        throw std::exception("Device doesn't support MSAA");
    }

    UINT msaaQuality = 0;

    CD3D11_TEXTURE2D_DESC msaaRenderTargetDesc(msaaBackBufferFormat, msaaBackBufferWidth, msaaBackBufferHeight, 1, 1, D3D11_BIND_RENDER_TARGET, D3D11_USAGE_DEFAULT, 0, msaaCount, msaaQuality);
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&msaaRenderTargetDesc, nullptr, m_renderTargetMSAA.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(m_renderTargetMSAA.Get(), nullptr, m_renderTargetViewMSAA.ReleaseAndGetAddressOf()));

    m_resolveFormat = msaaBackBufferFormat;
    msaaRenderTargetDesc.Format = m_resolveFormat;
    msaaRenderTargetDesc.SampleDesc.Count = 1;
    msaaRenderTargetDesc.SampleDesc.Quality = 0;
    msaaRenderTargetDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&msaaRenderTargetDesc, nullptr, m_resovleMSAA.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView(m_resovleMSAA.Get(), nullptr, m_resolveMSAASRV.ReleaseAndGetAddressOf()));

    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, msaaBackBufferWidth, msaaBackBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0, msaaCount, msaaQuality);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilViewMSAA.ReleaseAndGetAddressOf()));

    m_outputSize.left = m_outputSize.top = 0;
    m_outputSize.right = backBufferWidth;
    m_outputSize.bottom = backBufferHeight;
#else
    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));
#endif

    m_view = Matrix::CreateLookAt(Vector3(0.f, 0.f, 2.f), Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(msaaBackBufferWidth) / float(msaaBackBufferHeight), 0.1f, 10.f);

}

void Game::OnDeviceLost()
{
    // --- MSAA ---
    m_resovleMSAA.Reset();
    m_resolveMSAASRV.Reset();
    m_renderTargetMSAA.Reset();
    m_renderTargetViewMSAA.Reset();
    m_depthStencilViewMSAA.Reset();

    m_shape.reset();
    m_shape2.reset();
    m_batch.reset();
    m_rasterState.Reset();

    m_depthStencilView.Reset();
    m_renderTarget.Reset();
    m_renderTargetView.Reset();
    m_swapChain1.Reset();
    m_swapChain.Reset();
    m_d3dContext1.Reset();
    m_d3dContext.Reset();
    m_d3dDevice1.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}