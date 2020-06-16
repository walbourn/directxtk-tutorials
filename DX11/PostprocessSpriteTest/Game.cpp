//
// Game.cpp -
//

#include "pch.h"
#include "Game.h"

#include "ReadData.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
    struct VS_BLOOM_PARAMETERS
    {
        float bloomThreshold;
        float blurAmount;
        float bloomIntensity;
        float baseIntensity;
        float bloomSaturation;
        float baseSaturation;
        uint8_t na[8];
    };

    static_assert(!(sizeof(VS_BLOOM_PARAMETERS) % 16), "VS_BLOOM_PARAMETERS needs to be 16 bytes aligned");

    struct VS_BLUR_PARAMETERS
    {
        static const size_t SAMPLE_COUNT = 15;

        XMFLOAT4 sampleOffsets[SAMPLE_COUNT];
        XMFLOAT4 sampleWeights[SAMPLE_COUNT];

        void SetBlurEffectParameters(float dx, float dy, const VS_BLOOM_PARAMETERS& params)
        {
            sampleWeights[0].x = ComputeGaussian(0, params.blurAmount);
            sampleOffsets[0].x = sampleOffsets[0].y = 0.f;

            float totalWeights = sampleWeights[0].x;

            // Add pairs of additional sample taps, positioned
            // along a line in both directions from the center.
            for (size_t i = 0; i < SAMPLE_COUNT / 2; i++)
            {
                // Store weights for the positive and negative taps.
                float weight = ComputeGaussian( float(i + 1.f), params.blurAmount);

                sampleWeights[i * 2 + 1].x = weight;
                sampleWeights[i * 2 + 2].x = weight;

                totalWeights += weight * 2;

                // To get the maximum amount of blurring from a limited number of
                // pixel shader samples, we take advantage of the bilinear filtering
                // hardware inside the texture fetch unit. If we position our texture
                // coordinates exactly halfway between two texels, the filtering unit
                // will average them for us, giving two samples for the price of one.
                // This allows us to step in units of two texels per sample, rather
                // than just one at a time. The 1.5 offset kicks things off by
                // positioning us nicely in between two texels.
                float sampleOffset = float(i) * 2.f + 1.5f;

                Vector2 delta = Vector2(dx, dy) * sampleOffset;

                // Store texture coordinate offsets for the positive and negative taps.
                sampleOffsets[i * 2 + 1].x = delta.x;
                sampleOffsets[i * 2 + 1].y = delta.y;
                sampleOffsets[i * 2 + 2].x = -delta.x;
                sampleOffsets[i * 2 + 2].y = -delta.y;
            }

            for (size_t i = 0; i < SAMPLE_COUNT; i++)
            {
                sampleWeights[i].x /= totalWeights;
            }
        }

    private:
        float ComputeGaussian(float n, float theta)
        {
            return (float)((1.0 / sqrtf(2 * XM_PI * theta)) * expf(-(n * n) / (2 * theta * theta)));
        }
    };

    static_assert(!(sizeof(VS_BLUR_PARAMETERS) % 16), "VS_BLUR_PARAMETERS needs to be 16 bytes aligned");

    enum BloomPresets
    {
        Default = 0,
        Soft,
        Desaturated,
        Saturated,
        Blurry,
        Subtle,
        None
    };

    BloomPresets g_Bloom = Default;

    static const VS_BLOOM_PARAMETERS g_BloomPresets[] = 
    {
        //Thresh  Blur Bloom  Base  BloomSat BaseSat
        { 0.25f,  4,   1.25f, 1,    1,       1 }, // Default
        { 0,      3,   1,     1,    1,       1 }, // Soft
        { 0.5f,   8,   2,     1,    0,       1 }, // Desaturated
        { 0.25f,  4,   2,     1,    2,       0 }, // Saturated
        { 0,      2,   1,     0.1f, 1,       1 }, // Blurry
        { 0.5f,   2,   1,     1,    1,       1 }, // Subtle
        { 0.25f,  4,   1.25f, 1,    1,       1 }, // None
    };
};

// Constructor.
Game::Game() :
    m_window(0),
    m_featureLevel( D3D_FEATURE_LEVEL_9_1 )
{
    memset(&m_fullscreenRect, 0, sizeof(RECT));
    memset(&m_bloomRect, 0, sizeof(RECT));
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window)
{
    m_window = window;

    CreateDevice();

    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

// Executes basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world
void Game::Update(DX::StepTimer const& timer)
{
    float totalTime = static_cast<float>(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationZ(totalTime / 2.f)
              * Matrix::CreateRotationY(totalTime)
              * Matrix::CreateRotationX(totalTime * 2.f);
}

// Draws the scene
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    Clear();

    // TODO: Add your rendering code here
    m_spriteBatch->Begin();
    m_spriteBatch->Draw(m_background.Get(), m_fullscreenRect);
    m_spriteBatch->End();

    m_shape->Draw(m_world, m_view, m_projection);

#if 1
    PostProcess();
#endif

    Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views
#if 1
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_d3dContext->OMSetRenderTargets(1, m_sceneRT.GetAddressOf(), m_depthStencilView.Get());
#else
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
#endif
}

#if 1
void Game::PostProcess()
{
    ID3D11ShaderResourceView* null[] = { nullptr, nullptr };

    if (g_Bloom == None)
    {
        // Pass-through test
        m_d3dContext->CopyResource(m_backBuffer.Get(), m_sceneTex.Get());
    }
    else
    {
        // scene -> RT1 (downsample)
        m_d3dContext->OMSetRenderTargets(1, m_rt1RT.GetAddressOf(), nullptr);
        m_spriteBatch->Begin(SpriteSortMode_Immediate, nullptr, nullptr, nullptr, nullptr,
            [=](){
                m_d3dContext->PSSetConstantBuffers(0, 1, m_bloomParams.GetAddressOf());
                m_d3dContext->PSSetShader(m_bloomExtractPS.Get(), nullptr, 0);
            });
        m_spriteBatch->Draw(m_sceneSRV.Get(), m_bloomRect);
        m_spriteBatch->End();

        #if 0
        // RT1 (1st) screenshot
        m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_spriteBatch->Begin();
        m_spriteBatch->Draw(m_rt1SRV.Get(), m_bloomRect);
        m_spriteBatch->End();
        #else
        // RT1 -> RT2 (blur horizontal)
        m_d3dContext->OMSetRenderTargets(1, m_rt2RT.GetAddressOf(), nullptr);
        m_spriteBatch->Begin(SpriteSortMode_Immediate, nullptr, nullptr, nullptr, nullptr,
            [=](){
                m_d3dContext->PSSetShader(m_gaussianBlurPS.Get(), nullptr, 0);
                m_d3dContext->PSSetConstantBuffers(0, 1, m_blurParamsWidth.GetAddressOf());
            });
        m_spriteBatch->Draw(m_rt1SRV.Get(), m_bloomRect);
        m_spriteBatch->End();

        m_d3dContext->PSSetShaderResources(0, 2, null);

        #if 0
        // RT2 (1st) screenshot
        m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_spriteBatch->Begin();
        m_spriteBatch->Draw(m_rt2SRV.Get(), m_bloomRect);
        m_spriteBatch->End();
        #else
        // RT2 -> RT1 (blur vertical)
        m_d3dContext->OMSetRenderTargets(1, m_rt1RT.GetAddressOf(), nullptr);
        m_spriteBatch->Begin(SpriteSortMode_Immediate, nullptr, nullptr, nullptr, nullptr,
            [=](){
                m_d3dContext->PSSetShader(m_gaussianBlurPS.Get(), nullptr, 0);
                m_d3dContext->PSSetConstantBuffers(0, 1, m_blurParamsHeight.GetAddressOf());
            });
        m_spriteBatch->Draw(m_rt2SRV.Get(), m_bloomRect);
        m_spriteBatch->End();

        #if 0
        // RT1 (2nd) screenshot
        m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_spriteBatch->Begin();
        m_spriteBatch->Draw(m_rt1SRV.Get(), m_bloomRect);
        m_spriteBatch->End();
        #else
        // RT1 + scene
        m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_spriteBatch->Begin(SpriteSortMode_Immediate, nullptr, nullptr, nullptr, nullptr,
            [=](){
                m_d3dContext->PSSetShader(m_bloomCombinePS.Get(), nullptr, 0);
                m_d3dContext->PSSetShaderResources(1, 1, m_rt1SRV.GetAddressOf());
                m_d3dContext->PSSetConstantBuffers(0, 1, m_bloomParams.GetAddressOf());
            });
        m_spriteBatch->Draw(m_sceneSRV.Get(), m_fullscreenRect);
        m_spriteBatch->End();
        #endif
        #endif
        #endif
    }

    m_d3dContext->PSSetShaderResources(0, 2, null);
}
#endif

// Presents the backbuffer contents to the screen
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
    // TODO: Game is becoming active window
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized)
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize)
}

void Game::OnWindowSizeChanged()
{
    CreateResources();

    // TODO: Game window is being resized
}

void Game::OnKeyPress(int vk)
{
    BloomPresets preset;

    switch (vk)
    {
    case '1':   preset = Default;     break;
    case '2':   preset = Soft;        break;
    case '3':   preset = Desaturated; break;
    case '4':   preset = Saturated;   break;
    case '5':   preset = Blurry;      break;
    case '6':   preset = Subtle;      break;
    case '0':   preset = None;        break;

    default:
        return;
    }

    if (preset == g_Bloom)
        return;

    g_Bloom = preset;

#if 1
    if (m_bloomParams)
    {
        m_d3dContext->UpdateSubresource(m_bloomParams.Get(), 0, nullptr, &g_BloomPresets[preset], sizeof(VS_BLOOM_PARAMETERS), 0);
    }

    if (m_bloomRect.right && m_blurParamsWidth)
    {
        VS_BLUR_PARAMETERS blurData;
        blurData.SetBlurEffectParameters(1.f / m_bloomRect.right, 0, g_BloomPresets[preset]);
        m_d3dContext->UpdateSubresource(m_blurParamsWidth.Get(), 0, nullptr, &blurData, sizeof(VS_BLUR_PARAMETERS), 0);
    }

    if (m_bloomRect.bottom && m_blurParamsHeight)
    {
        VS_BLUR_PARAMETERS blurData;
        blurData.SetBlurEffectParameters(0, 1.f / m_bloomRect.bottom, g_BloomPresets[preset]);
        m_d3dContext->UpdateSubresource(m_blurParamsHeight.Get(), 0, nullptr, &blurData, sizeof(VS_BLUR_PARAMETERS), 0);
    }
#endif
}

// Properties
void Game::GetDefaultSize(size_t& width, size_t& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200)
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    // This flag adds support for surfaces with a different color channel ordering than the API default.
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        // TODO: Modify for supported Direct3D feature levels (see code below related to 11.1 fallback handling)
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
        nullptr,                                // specify null to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,                                // leave as nullptr unless software device
        creationFlags,                          // optionally set debug and Direct2D compatibility flags
        featureLevels,                          // list of feature levels this app can support
        _countof(featureLevels),                // number of entries in above list
        D3D11_SDK_VERSION,                      // always set this to D3D11_SDK_VERSION
        m_d3dDevice.ReleaseAndGetAddressOf(),   // returns the Direct3D device created
        &m_featureLevel,                        // returns feature level of device created
        m_d3dContext.ReleaseAndGetAddressOf()   // returns the device immediate context
        );

    if ( hr == E_INVALIDARG )
    {
        // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
        hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                creationFlags, &featureLevels[1], ARRAYSIZE(featureLevels) - 1,
                                D3D11_SDK_VERSION, m_d3dDevice.ReleaseAndGetAddressOf(),
                                &m_featureLevel, m_d3dContext.ReleaseAndGetAddressOf() );
    }

    DX::ThrowIfFailed(hr);

    // TODO: Initialize device dependent objects here (independent of window size)
    DX::ThrowIfFailed(CreateWICTextureFromFile(m_d3dDevice.Get(), L"sunset.jpg", nullptr, m_background.ReleaseAndGetAddressOf()));

    m_states = std::make_unique<CommonStates>(m_d3dDevice.Get());
    m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());
    m_shape = GeometricPrimitive::CreateTorus(m_d3dContext.Get());

#if 1
    auto blob = DX::ReadData( L"BloomExtract.cso" );
    DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader( &blob.front(), blob.size(), nullptr, m_bloomExtractPS.ReleaseAndGetAddressOf()));

    blob = DX::ReadData( L"BloomCombine.cso" );
    DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader( &blob.front(), blob.size(), nullptr, m_bloomCombinePS.ReleaseAndGetAddressOf()));

    blob = DX::ReadData( L"GaussianBlur.cso" );
    DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader( &blob.front(), blob.size(), nullptr, m_gaussianBlurPS.ReleaseAndGetAddressOf()));

    {
        CD3D11_BUFFER_DESC cbDesc(sizeof(VS_BLOOM_PARAMETERS), D3D11_BIND_CONSTANT_BUFFER);
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &g_BloomPresets[g_Bloom];
        initData.SysMemPitch = sizeof(VS_BLOOM_PARAMETERS);
        DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&cbDesc, &initData, m_bloomParams.ReleaseAndGetAddressOf()));
    }

    {
        CD3D11_BUFFER_DESC cbDesc(sizeof(VS_BLUR_PARAMETERS), D3D11_BIND_CONSTANT_BUFFER);
        DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_blurParamsWidth.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_blurParamsHeight.ReleaseAndGetAddressOf()));
    }
#endif

    m_view = Matrix::CreateLookAt(Vector3(0.f, 3.f, -3.f), Vector3::Zero, Vector3::UnitY);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews [] = { nullptr };
    m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    RECT rc;
    GetWindowRect( m_window, &rc );

    UINT backBufferWidth = std::max<UINT>( rc.right - rc.left, 1 );
    UINT backBufferHeight = std::max<UINT>( rc.bottom - rc.top, 1);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D16_UNORM;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight, backBufferFormat, 0);

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
        // First, retrieve the underlying DXGI Device from the D3D Device
        Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

        // And obtain the factory object that created it.
        Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), &dxgiFactory));

        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory2;
        dxgiFactory.As(&dxgiFactory2);
        if ( dxgiFactory2 )
        {
            // DirectX 11.1 or later
            m_d3dDevice.As( &m_d3dDevice1 );
            m_d3dContext.As( &m_d3dContext1 );

            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
            swapChainDesc.Width = backBufferWidth;
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
            fsSwapChainDesc.Windowed = TRUE;

            // Create a SwapChain from a CoreWindow.
            DX::ThrowIfFailed( dxgiFactory2->CreateSwapChainForHwnd(
                m_d3dDevice.Get(), m_window, &swapChainDesc,
                &fsSwapChainDesc,
                nullptr, m_swapChain1.ReleaseAndGetAddressOf() ) );

            m_swapChain1.As( &m_swapChain );
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
            swapChainDesc.BufferCount = 2;
            swapChainDesc.BufferDesc.Width = backBufferWidth;
            swapChainDesc.BufferDesc.Height = backBufferHeight;
            swapChainDesc.BufferDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.OutputWindow = m_window;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Windowed = TRUE;

            DX::ThrowIfFailed( dxgiFactory->CreateSwapChain( m_d3dDevice.Get(), &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf() ) );
        }

        // This template does not support 'full-screen' mode and prevents the ALT+ENTER shortcut from working
        dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &m_backBuffer));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(m_backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, &m_depthStencilView));

    // Create a viewport descriptor of the full window size.
    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight));

    // Set the current viewport using the descriptor.
    m_d3dContext->RSSetViewports(1, &viewPort);

    // TODO: Initialize windows-size dependent objects here

    // Create render targets for post processing
    m_fullscreenRect.left = 0;
    m_fullscreenRect.top = 0;
    m_fullscreenRect.right = backBufferWidth;
    m_fullscreenRect.bottom = backBufferHeight;

    m_projection = Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4, float(backBufferWidth) / float(backBufferHeight), 0.01f, 100.f);

#if 1
    VS_BLUR_PARAMETERS blurData;
    blurData.SetBlurEffectParameters(1.f / (backBufferWidth / 2), 0, g_BloomPresets[g_Bloom]);
    m_d3dContext->UpdateSubresource(m_blurParamsWidth.Get(), 0, nullptr, &blurData, sizeof(VS_BLUR_PARAMETERS), 0);

    blurData.SetBlurEffectParameters(0, 1.f / (backBufferHeight / 2), g_BloomPresets[g_Bloom]);
    m_d3dContext->UpdateSubresource(m_blurParamsHeight.Get(), 0, nullptr, &blurData, sizeof(VS_BLUR_PARAMETERS), 0);
#endif

#if 1
    // Full-size render target for scene
    CD3D11_TEXTURE2D_DESC sceneDesc(backBufferFormat, backBufferWidth, backBufferHeight, 1, 1,
                                    D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&sceneDesc, nullptr, m_sceneTex.GetAddressOf()));
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(m_sceneTex.Get(), nullptr, m_sceneRT.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView(m_sceneTex.Get(), nullptr, m_sceneSRV.ReleaseAndGetAddressOf()));

    // Half-size blurring render targets
    CD3D11_TEXTURE2D_DESC rtDesc(backBufferFormat, backBufferWidth / 2, backBufferHeight / 2, 1, 1,
                                      D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    //Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture1;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&rtDesc, nullptr, rtTexture1.GetAddressOf()));
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(rtTexture1.Get(), nullptr, m_rt1RT.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView(rtTexture1.Get(), nullptr, m_rt1SRV.ReleaseAndGetAddressOf()));

    //Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture2;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&rtDesc, nullptr, rtTexture2.GetAddressOf()));
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(rtTexture2.Get(), nullptr, m_rt2RT.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView(rtTexture2.Get(), nullptr, m_rt2SRV.ReleaseAndGetAddressOf()));

    m_bloomRect.left = 0;
    m_bloomRect.top = 0;
    m_bloomRect.right = backBufferWidth / 2;
    m_bloomRect.bottom = backBufferHeight / 2;
#endif
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here
    m_states.reset();
    m_spriteBatch.reset();
    m_shape.reset();
    m_background.Reset();

#if 1
    m_bloomExtractPS.Reset();
    m_bloomCombinePS.Reset();
    m_gaussianBlurPS.Reset();

    m_bloomParams.Reset();
    m_blurParamsWidth.Reset();
    m_blurParamsHeight.Reset();
#endif

#if 1
    m_sceneTex.Reset();
    m_sceneSRV.Reset();
    m_sceneRT.Reset();
    m_rt1SRV.Reset();
    m_rt1RT.Reset();
    m_rt2SRV.Reset();
    m_rt2RT.Reset();

    rtTexture1.Reset();
    rtTexture2.Reset();
#endif

    m_backBuffer.Reset();

    m_depthStencil.Reset();
    m_depthStencilView.Reset();
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