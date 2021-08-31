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
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0);
    m_deviceResources->RegisterDeviceNotify(this);

    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
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
    //float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    auto time = static_cast<float>(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationY(cosf(time) * 2.f);
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
    m_shape->Draw(m_effect.get(), m_inputLayout.Get(), false, false, [=] {
        ID3D11SamplerState* samplers[] =
        {
            m_states->AnisotropicClamp(),
            m_states->LinearWrap(),
        };
        context->PSSetSamplers(0, 2, samplers);
        });

    // Tonemap
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    m_toneMap->Process(context);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);
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
    auto renderTarget = m_hdrScene->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
    context->ClearRenderTargetView(renderTarget, color);

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
    m_effect = std::make_unique<PBREffect>(device);

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_shape = GeometricPrimitive::CreateSphere(context);
    m_shape->CreateInputLayout(m_effect.get(),
        m_inputLayout.ReleaseAndGetAddressOf());

    // Image-based lighting cubemaps.
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"SunSubMixer_diffuseIBL.dds",
            nullptr,
            m_radiance.ReleaseAndGetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    m_radiance->GetDesc(&desc);

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"SunSubMixer_specularIBL.dds",
            nullptr,
            m_irradiance.ReleaseAndGetAddressOf()));

    m_effect->SetIBLTextures(m_radiance.Get(),
        static_cast<int>(desc.TextureCube.MipLevels),
        m_irradiance.Get());

    m_hdrScene->SetDevice(device);

    m_toneMap = std::make_unique<ToneMapPostProcess>(device);
    m_toneMap->SetOperator(ToneMapPostProcess::Reinhard);
    m_toneMap->SetTransferFunction(ToneMapPostProcess::SRGB);

    m_world = Matrix::Identity;

#if 1
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"Sphere2Mat_baseColor.png",
            nullptr,
            m_albedoMap.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"Sphere2Mat_normal.png",
            nullptr,
            m_normalMap.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"Sphere2Mat_occlusionRoughnessMetallic.png",
            nullptr,
            m_rmaMap.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"Sphere2Mat_emissive.png",
            nullptr,
            m_emissiveMap.ReleaseAndGetAddressOf()));

    m_effect->SetSurfaceTextures(m_albedoMap.Get(), m_normalMap.Get(), m_rmaMap.Get());
    m_effect->SetEmissiveTexture(m_emissiveMap.Get());
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f),
        Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);

    m_hdrScene->SetWindow(size);
    m_toneMap->SetHDRSourceTexture(m_hdrScene->GetShaderResourceView());
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_shape.reset();
    m_effect.reset();
    m_inputLayout.Reset();
    m_radiance.Reset();
    m_irradiance.Reset();
    m_hdrScene->ReleaseDevice();
    m_toneMap.reset();

    m_albedoMap.Reset();
    m_normalMap.Reset();
    m_rmaMap.Reset();
    m_emissiveMap.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
