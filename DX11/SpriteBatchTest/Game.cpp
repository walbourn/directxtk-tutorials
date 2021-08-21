//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

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

#if 1
    m_spriteBatch->Begin();
    //m_spriteBatch->Begin( SpriteSortMode_Deferred, m_states->NonPremultiplied() );
    //m_spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, m_states->LinearWrap());
    //m_spriteBatch->Begin(SpriteSortMode_Deferred, m_states->AlphaBlend(), m_states->PointClamp());

#if 1
    m_spriteBatch->Draw(m_background.Get(), m_fullscreenRect);
#endif


    m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr, Colors::White, 0.f, m_origin);
    //m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr, Colors::White, cosf(time) * 4.f, m_origin);

    //float time = float(m_timer.GetTotalSeconds());
    //m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr, Colors::White, 0.f, m_origin, cosf(time) + 2.f);

    //m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr, Colors::Green, 0.f, m_origin);
    //m_spriteBatch->Draw(m_texture.Get(), m_screenPos, &m_tileRect, Colors::White, 0.f, m_origin);
    //m_spriteBatch->Draw(m_texture.Get(), m_stretchRect, nullptr, Colors::White);
#else
    m_spriteBatch->Begin(SpriteSortMode_BackToFront, m_states->NonPremultiplied());
    
    float time = float(m_timer.GetTotalSeconds());

    float rot = cosf(time) * 2.f;
    float scale = ( cosf(time) + 2.f )*1.5f;

    m_spriteBatch->Draw(m_texture.Get(), XMFLOAT2(400, 400), nullptr, Colors::White, rot, XMFLOAT2(50, 50), scale, SpriteEffects_None, 0.f);
    m_spriteBatch->Draw(m_texture.Get(), XMFLOAT2(400, 400), nullptr, Colors::Black, rot, XMFLOAT2(100, 100), scale, SpriteEffects_None, 1.f);
    m_spriteBatch->Draw(m_texture.Get(), XMFLOAT2(400, 300), nullptr, Colors::Blue, rot, XMFLOAT2(50, 50), scale, SpriteEffects_None, 1.f);
    // TODO: Add your rendering code here
#endif
    
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
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    m_states = std::make_unique<CommonStates>(device);

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    ComPtr<ID3D11Resource> resource;
#if 1
    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"cat.png", resource.GetAddressOf(), m_texture.ReleaseAndGetAddressOf()));
#else
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"cat.dds", resource.GetAddressOf(), m_texture.ReleaseAndGetAddressOf()));
#endif

    ComPtr<ID3D11Texture2D> cat;
    DX::ThrowIfFailed(resource.As(&cat));

    CD3D11_TEXTURE2D_DESC catDesc;
    cat->GetDesc(&catDesc);

#if 1
    m_origin.x = float(catDesc.Width / 2);
    m_origin.y = float(catDesc.Height / 2);
#else
    m_origin.x = float(catDesc.Width * 2);
    m_origin.y = float(catDesc.Height * 2);

    m_tileRect.left = catDesc.Width * 2;
    m_tileRect.right = catDesc.Width * 6;
    m_tileRect.top = catDesc.Height * 2;
    m_tileRect.bottom = catDesc.Height * 6;
#endif

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"sunset.jpg", nullptr, m_background.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_screenPos.x = float(size.right) / 2.f;
    m_screenPos.y = float(size.bottom) / 2.f;

    m_fullscreenRect = size;

#if 0
    m_stretchRect.left = size.right / 4;
    m_stretchRect.top   = size.bottom / 4;
    m_stretchRect.right = m_stretchRect.left  + size.right / 2;
    m_stretchRect.bottom = m_stretchRect.top + size.bottom / 2;
#else
    m_stretchRect.left = size.right / 16;
    m_stretchRect.top = size.bottom / 16;
    m_stretchRect.right = size.right - m_stretchRect.left;
    m_stretchRect.bottom = size.bottom - m_stretchRect.top;
#endif
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_spriteBatch.reset();
    m_texture.Reset();
    m_background.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
