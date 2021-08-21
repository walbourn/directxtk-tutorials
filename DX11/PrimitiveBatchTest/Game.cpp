//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#define MSAA
//#define MSAA_HELPER

#ifdef MSAA
namespace
{
    constexpr UINT MSAA_COUNT = 4;
    constexpr UINT MSAA_QUALITY = 0;
}
#endif

Game::Game() noexcept(false)
{
#ifdef MSAA
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>();
#endif
    m_deviceResources->RegisterDeviceNotify(this);

#if defined(MSAA) && defined(MSAA_HELPER)
    m_msaaHelper = std::make_unique<DX::MSAAHelper>(
        m_deviceResources->GetBackBufferFormat(),
        DXGI_FORMAT_D32_FLOAT,
        MSAA_COUNT);
#endif
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
    // TODO: Add your game logic here
#if 1
    m_world = Matrix::CreateRotationY(cosf(static_cast<float>(timer.GetTotalSeconds())));
#endif

#if 0
    auto time = static_cast<float>(m_timer.GetTotalSeconds());

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    auto quat = Quaternion::CreateFromYawPitchRoll(pitch, yaw, roll);

    auto light = XMVector3Rotate(g_XMOne, quat);

    m_effect->SetLightDirection(0, light);
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
#if 0
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);

#if 1
    context->RSSetState(m_states->CullNone());
#elif 1
    context->RSSetState(m_states->CullCounterClockwise());
#else
    context->RSSetState(m_states->CullClockwise());
#endif

    m_effect->Apply(context);

    auto sampler = m_states->LinearClamp();
    context->PSSetSamplers(0, 1, &sampler);

    context->IASetInputLayout(m_inputLayout.Get());

    m_batch->Begin();

#if 0
    VertexPositionColor v1(Vector3(0.f, 0.5f, 0.5f), Colors::Yellow);
    VertexPositionColor v2(Vector3(0.5f, -0.5f, 0.5f), Colors::Yellow);
    VertexPositionColor v3(Vector3(-0.5f, -0.5f, 0.5f), Colors::Yellow);
#elif 0
    VertexPositionColor v1(Vector3(400.f, 150.f, 0.f), Colors::Yellow);
    VertexPositionColor v2(Vector3(600.f, 450.f, 0.f), Colors::Yellow);
    VertexPositionColor v3(Vector3(200.f, 450.f, 0.f), Colors::Yellow);
#elif 0
    VertexPositionColor v1(Vector3(400.f, 150.f, 0.f), Colors::Red);
    VertexPositionColor v2(Vector3(600.f, 450.f, 0.f), Colors::Green);
    VertexPositionColor v3(Vector3(200.f, 450.f, 0.f), Colors::Blue);
#elif 1
    VertexPositionTexture v1(Vector3(400.f, 150.f, 0.f), Vector2(.5f, 0));
    VertexPositionTexture v2(Vector3(600.f, 450.f, 0.f), Vector2(1, 1));
    VertexPositionTexture v3(Vector3(200.f, 450.f, 0.f), Vector2(0, 1));
#else
    VertexPositionNormalTexture v1(Vector3(400.f, 150.f, 0.f), -Vector3::UnitZ, Vector2(.5f, 0));
    VertexPositionNormalTexture v2(Vector3(600.f, 450.f, 0.f), -Vector3::UnitZ, Vector2(1, 1));
    VertexPositionNormalTexture v3(Vector3(200.f, 450.f, 0.f), -Vector3::UnitZ, Vector2(0, 1));
#endif

    m_batch->DrawTriangle(v1, v2, v3);

    m_batch->End();
#endif

#if 1
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);

#if 0
    context->RSSetState(m_states->CullNone());
#else
    context->RSSetState(m_raster.Get());
#endif

    m_effect->SetWorld(m_world);

    m_effect->Apply(context);

    context->IASetInputLayout(m_inputLayout.Get());

    m_batch->Begin();

    Vector3 xaxis(2.f, 0.f, 0.f);
    Vector3 yaxis(0.f, 0.f, 2.f);
    Vector3 origin = Vector3::Zero;

    constexpr size_t divisions = 20;

    for (size_t i = 0; i <= divisions; ++i)
    {
        float fPercent = float(i) / float(divisions);
        fPercent = (fPercent * 2.0f) - 1.0f;

        Vector3 scale = xaxis * fPercent + origin;

        VertexPositionColor v1(scale - yaxis, Colors::White);
        VertexPositionColor v2(scale + yaxis, Colors::White);
        m_batch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= divisions; i++)
    {
        float fPercent = float(i) / float(divisions);
        fPercent = (fPercent * 2.0f) - 1.0f;

        Vector3 scale = yaxis * fPercent + origin;

        VertexPositionColor v1(scale - xaxis, Colors::White);
        VertexPositionColor v2(scale + xaxis, Colors::White);
        m_batch->DrawLine(v1, v2);
    }

    m_batch->End();
#endif

#ifdef MSAA
#ifdef MSAA_HELPER
    m_msaaHelper->Resolve(context, m_deviceResources->GetRenderTarget());
#else
    context->ResolveSubresource(m_deviceResources->GetRenderTarget(), 0,
        m_offscreenRenderTarget.Get(), 0,
        m_deviceResources->GetBackBufferFormat());
#endif
#endif

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

#ifdef MSAA
#ifdef MSAA_HELPER
    auto renderTarget = m_msaaHelper->GetMSAARenderTargetView();
    auto depthStencil = m_msaaHelper->GetMSAADepthStencilView();
#else
    auto renderTarget = m_offscreenRenderTargetSRV.Get();
    auto depthStencil = m_depthStencilSRV.Get();
#endif
#else
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();
#endif

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

    // TODO: Initialize device dependent objects here (independent of window size)
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"rocks.jpg", nullptr,
            m_texture.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"rocks_normalmap.dds", nullptr,
            m_normalMap.ReleaseAndGetAddressOf()));

    m_states = std::make_unique<CommonStates>(device);

#if 1
    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetVertexColorEnabled(true);
#elif 1
    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetTextureEnabled(true);
    m_effect->SetTexture(m_texture.Get());
#else
    m_effect = std::make_unique<NormalMapEffect>(device);
    m_effect->SetTexture(m_texture.Get());
    m_effect->SetNormalTexture(m_normalMap.Get());
    m_effect->EnableDefaultLighting();
    m_effect->SetLightDiffuseColor(0, Colors::Gray);
#endif

#if 1
    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexType>(device, m_effect.get(), m_inputLayout.ReleaseAndGetAddressOf())
    );
#else
    void const* shaderByteCode;
    size_t byteCodeLength;

    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    DX::ThrowIfFailed(
        device->CreateInputLayout(VertexType::InputElements, VertexType::InputElementCount,
                                       shaderByteCode, byteCodeLength, m_inputLayout.ReleaseAndGetAddressOf()));
#endif

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_batch = std::make_unique<PrimitiveBatch<VertexType>>(context);

    m_world = Matrix::Identity;

#if 1

#if 0
    CD3D11_RASTERIZER_DESC rastDesc(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE,
        D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE,
        TRUE, TRUE);
#else
    CD3D11_RASTERIZER_DESC rastDesc(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE,
        D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE,
        TRUE, FALSE);
#endif

    DX::ThrowIfFailed(device->CreateRasterizerState(&rastDesc, m_raster.ReleaseAndGetAddressOf()));
#endif

#if defined(MSAA) && defined(MSAA_HELPER)
    m_msaaHelper->SetDevice(device);
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.

    auto size = m_deviceResources->GetOutputSize();
#if 0
    Matrix proj = Matrix::CreateOrthographicOffCenter(0.f, float(size.right), float(size.bottom), 0.f, 0.f, 1.f);

//    Matrix proj = Matrix::CreateScale( 2.f/float(backBufferWidth), -2.f/float(backBufferHeight), 1.f)
//             * Matrix::CreateTranslation( -1.f, 1.f, 0.f );
    m_effect->SetProjection(proj);
#endif

#if 1
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f), Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f, float(size.right) / float(size.bottom), 0.1f, 10.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
#endif

#ifdef MSAA
#ifdef MSAA_HELPER
    m_msaaHelper->SetWindow(size);
#else
    auto device = m_deviceResources->GetD3DDevice();
    auto width = static_cast<UINT>(size.right);
    auto height = static_cast<UINT>(size.bottom);

    CD3D11_TEXTURE2D_DESC rtDesc(m_deviceResources->GetBackBufferFormat(),
        width, height, 1, 1,
        D3D11_BIND_RENDER_TARGET, D3D11_USAGE_DEFAULT, 0,
        MSAA_COUNT, MSAA_QUALITY);

    DX::ThrowIfFailed(
        device->CreateTexture2D(&rtDesc, nullptr,
            m_offscreenRenderTarget.ReleaseAndGetAddressOf()));

    CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS);

    DX::ThrowIfFailed(
        device->CreateRenderTargetView(m_offscreenRenderTarget.Get(),
            &rtvDesc,
            m_offscreenRenderTargetSRV.ReleaseAndGetAddressOf()));

    CD3D11_TEXTURE2D_DESC dsDesc(DXGI_FORMAT_D32_FLOAT,
        width, height, 1, 1,
        D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0,
        MSAA_COUNT, MSAA_QUALITY);

    ComPtr<ID3D11Texture2D> depthBuffer;
    DX::ThrowIfFailed(
        device->CreateTexture2D(&dsDesc, nullptr, depthBuffer.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS);

    DX::ThrowIfFailed(
        device->CreateDepthStencilView(depthBuffer.Get(),
            &dsvDesc,
            m_depthStencilSRV.ReleaseAndGetAddressOf()));
#endif
#endif
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_effect.reset();
    m_batch.reset();
    m_inputLayout.Reset();
    m_raster.Reset();
    m_texture.Reset();
    m_normalMap.Reset();

#if defined(MSAA) && defined(MSAA_HELPER)
    m_msaaHelper->ReleaseDevice();
#else
    m_offscreenRenderTarget.Reset();
    m_offscreenRenderTargetSRV.Reset();
    m_depthStencilSRV.Reset();
#endif
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
