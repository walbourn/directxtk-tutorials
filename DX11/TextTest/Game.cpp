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

    m_spriteBatch->Begin();

#if 1
    const wchar_t* output = L"Hello World";

    Vector2 origin = m_font->MeasureString(output) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos, Colors::White, 0.f, origin);
#endif

#if 0
    const wchar_t* output = L"Hello World";

    Vector2 origin = m_font->MeasureString(output) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos + Vector2(1.f, 1.f), Colors::Black, 0.f, origin);
    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos + Vector2(-1.f, 1.f), Colors::Black, 0.f, origin);

    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos, Colors::White, 0.f, origin);
#endif

#if 0
    const wchar_t* output = L"Hello World";

    Vector2 origin = m_font->MeasureString(output) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos + Vector2(1.f, 1.f), Colors::Black, 0.f, origin);
    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos + Vector2(-1.f, 1.f), Colors::Black, 0.f, origin);
    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos + Vector2(-1.f, -1.f), Colors::Black, 0.f, origin);
    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos + Vector2(1.f, -1.f), Colors::Black, 0.f, origin);

    m_font->DrawString(m_spriteBatch.get(), output, m_fontPos, Colors::White, 0.f, origin);
#endif

#if 0
    std::wstring output = std::wstring(L"Hello") + std::wstring(L" World");

    Vector2 origin = m_font->MeasureString(output.c_str()) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), output.c_str(), m_fontPos, Colors::White, 0.f, origin);
#endif

#if 0
    const char *ascii = "Hello World";

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring output = converter.from_bytes(ascii);

    Vector2 origin = m_font->MeasureString(output.c_str()) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), output.c_str(), m_fontPos, Colors::White, 0.f, origin);

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
    m_font = std::make_unique<SpriteFont>(device, L"myfile.spritefont");
    auto context = m_deviceResources->GetD3DDeviceContext();
    m_spriteBatch = std::make_unique<SpriteBatch>(context);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_fontPos.x = float(size.right) / 2.f;
    m_fontPos.y = float(size.bottom) / 2.f;
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_font.reset();
    m_spriteBatch.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
