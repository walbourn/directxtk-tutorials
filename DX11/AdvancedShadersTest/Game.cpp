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
    auto time = static_cast<float>(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationZ(cosf(time) * 2.f);

#if 0
    m_effect->SetFresnelFactor(cosf(time * 2.f));
#endif
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
    auto context = m_deviceResources->GetD3DDeviceContext();

    // TODO: Add your rendering code here.
    m_effect->SetWorld(m_world);
    m_shape->Draw(m_effect.get(), m_inputLayout.Get(), false, false, [=]{
            auto sampler = m_states->LinearWrap();
            context->PSSetSamplers( 1, 1, &sampler );
        });

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

#if 0
    m_effect = std::make_unique<EnvironmentMapEffect>(device);
#elif 0
    m_effect = std::make_unique<NormalMapEffect>(device);
#else
    m_effect = std::make_unique<DebugEffect>(device);
#endif

#if 0
    m_effect->EnableDefaultLighting();
#else
    m_effect->SetHemisphericalAmbientColor(Colors::DarkBlue, Colors::Purple);
#endif

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_shape = GeometricPrimitive::CreateTeapot(context);
    m_shape->CreateInputLayout(m_effect.get(),
        m_inputLayout.ReleaseAndGetAddressOf());

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"wood.dds", nullptr,
        m_texture.ReleaseAndGetAddressOf()));

#if 0
    m_effect->SetTexture(m_texture.Get());
#endif

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"cubemap.dds", nullptr,
        m_cubemap.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"normalMap.dds", nullptr,
            m_normalTexture.ReleaseAndGetAddressOf()));

#if 0
    m_effect->SetEnvironmentMap(m_cubemap.Get());
#elif 0
    m_effect->SetNormalTexture(m_normalTexture.Get());
#endif

    m_world = Matrix::Identity;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f), Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_shape.reset();
    m_effect.reset();
    m_inputLayout.Reset();
    m_texture.Reset();
    m_cubemap.Reset();
    m_normalTexture.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
