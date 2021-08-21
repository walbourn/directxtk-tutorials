//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    float randf()
    {
        return (float)rand() / (float)RAND_MAX * 10000;
    }
}

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

#pragma region Frame Update
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
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    elapsedTime;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    //auto context = m_deviceResources->GetD3DDeviceContext();

    // TODO: Add your rendering code here.
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

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1024;
    height = 768;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    auto context = m_deviceResources->GetD3DDeviceContext();
    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    m_sprites = std::make_unique<SpriteSheet>();

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"SpriteSheetSample.png", nullptr, m_texture.ReleaseAndGetAddressOf()));
    //  DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"SpriteSheetSample.dds", nullptr, m_texture.ReleaseAndGetAddressOf()));

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
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_spriteBatch.reset();
    m_texture.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
