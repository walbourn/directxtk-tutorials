//
// Game.cpp -
//

#include "pch.h"
#include "Game.h"

using namespace DirectX;

float randf()
{
    return (float)rand() / (float)RAND_MAX * 10000;
}

// Constructor.
Game::Game() :
    m_window(0),
    m_featureLevel( D3D_FEATURE_LEVEL_9_1 )
{
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
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here
    elapsedTime;
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

    // TODO -
    //auto frame = m_sprites->Find(L"glow1");
    //auto frame = m_sprites->Find(L"glow7");
    auto frame = m_sprites->Find(L"cat");
    assert(frame != 0);

    float time = float(60 * m_timer.GetTotalSeconds());

    // Moving
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(900,384.f + sinf(time/60.f)*384.f), Colors::White, 0.f, 1, SpriteEffects_None, 0);

    // Spinning.
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(200, 150), Colors::White, time / 100, 1, SpriteEffects_None, 0);

    // Differently scaled.
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(100, 500), Colors::White, 0, 0.5);

    RECT dest1 = { 100, 200, 100 + 256, 200 + 64 };
    RECT dest2 = { 100, 400, 100 + 64, 400 + 256 };

    m_sprites->Draw(m_spriteBatch.get(), *frame, dest1);
    m_sprites->Draw(m_spriteBatch.get(), *frame, dest2);

    // Mirroring.
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(200, 300), Colors::White, 0, 0.3f, SpriteEffects_None);
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(400, 300), Colors::White, 0, 0.3f, SpriteEffects_FlipHorizontally);
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(600, 300), Colors::White, 0, 0.3f, SpriteEffects_FlipVertically);
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(800, 300), Colors::White, 0, 0.3f, SpriteEffects_FlipBoth);

    // Draw overloads specifying position and scale as XMFLOAT2.
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(-40, 320), Colors::Red);
        
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(200, 320), Colors::Lime, time / 500, 0.5f, SpriteEffects_None, 0.5f);
        
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMFLOAT2(350, 320), Colors::Blue, time / 500, XMFLOAT2(0.25f, 0.5f), SpriteEffects_None, 0.5f);

    // Draw overloads specifying position, origin and scale via the first two components of an XMVECTOR.
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMVectorSet(0, 450, randf(), randf()), Colors::Pink);
        
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMVectorSet(200, 450, randf(), randf()), Colors::Lime, time / 500, 0.5f, SpriteEffects_None, 0.5f);
        
    m_sprites->Draw(m_spriteBatch.get(), *frame, XMVectorSet(350, 450, randf(), randf()), Colors::Blue, time / 500, XMVectorSet(0.25f, 0.5f, randf(), randf()), SpriteEffects_None, 0.5f);

    // Draw overloads specifying position as a RECT.
    RECT rc1 = { 500, 320, 600, 420 };
    RECT rc2 = { 550, 450, 650, 550 };

    m_sprites->Draw(m_spriteBatch.get(), *frame, rc1, Colors::Gray);
        
    m_sprites->Draw(m_spriteBatch.get(), *frame, rc2, Colors::LightSeaGreen, time / 300, SpriteEffects_None, 0.5f);

    m_spriteBatch->End();

    Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
}

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

// Properties
void Game::GetDefaultSize(size_t& width, size_t& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200)
    width = 1024;
    height = 768;
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
    m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());

    m_sprites = std::make_unique<SpriteSheet>();

    DX::ThrowIfFailed(CreateWICTextureFromFile(m_d3dDevice.Get(), L"SpriteSheetSample.png", nullptr, m_texture.ReleaseAndGetAddressOf()));
    //  DX::ThrowIfFailed(CreateDDSTextureFromFile(m_d3dDevice.Get(), L"SpriteSheetSample.dds", nullptr, m_texture.ReleaseAndGetAddressOf()));

#if 0
    {
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        m_texture->GetResource( resource.GetAddressOf() );

        D3D11_RESOURCE_DIMENSION dim;
        resource->GetType( &dim );

        if ( dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D )
            throw std::exception( "AnimatedTexture expects a Texture2D" );

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
        resource.As( &tex2D );

        D3D11_TEXTURE2D_DESC desc;
        tex2D->GetDesc( &desc );


    }
#endif

    m_sprites->Load(m_texture.Get(), L"SpriteSheetSample.txt");
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
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

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
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here
    m_spriteBatch.reset();
    m_texture.Reset();

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