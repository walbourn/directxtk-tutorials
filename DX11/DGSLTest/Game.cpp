//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

#include "ReadData.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
#include "dgslsphere.inc"
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
    auto time = static_cast<float>(timer.GetTotalSeconds());

    m_effect->SetTime(time);

    m_world = Matrix::CreateRotationZ(cosf(time) * 2.f);

    m_effect->SetWorld(m_world);
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
    m_effect->Apply(context);

    auto sampler = m_states->LinearWrap();
    context->PSSetSamplers(0, 1, &sampler);

    context->RSSetState(m_states->CullClockwise());

    context->IASetIndexBuffer(m_shapeIB.Get(), DXGI_FORMAT_R16_UINT, 0);
    
    context->IASetInputLayout(m_inputLayout.Get());

    UINT stride = sizeof(VertexPositionNormalTangentColorTexture);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_shapeVB.GetAddressOf(), &stride, &offset);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(static_cast<UINT>(std::size(g_sphereIB)), 0, 0);

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

    // Create DGSL Effect
    auto blob = DX::ReadData(L"MyDGSLShader.cso");
    DX::ThrowIfFailed(device->CreatePixelShader(&blob.front(), blob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf()));

    m_effect = std::make_unique<DGSLEffect>(device, m_pixelShader.Get());
    m_effect->SetTextureEnabled(true);
    m_effect->SetVertexColorEnabled(true);

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"billard15.dds", nullptr,
        m_texture.ReleaseAndGetAddressOf()));

    m_effect->SetTexture(m_texture.Get());

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"envmap.dds", nullptr,
        m_texture2.ReleaseAndGetAddressOf()));

    m_effect->SetTexture(1, m_texture2.Get());
    m_effect->EnableDefaultLighting();

    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexPositionNormalTangentColorTexture>(device,
            m_effect.get(), m_inputLayout.ReleaseAndGetAddressOf()));

    // Create sphere geometry with DGSL vertex data
    DX::ThrowIfFailed(
        CreateStaticBuffer(device, g_sphereVB, std::size(g_sphereVB),
            D3D11_BIND_VERTEX_BUFFER,
            m_shapeVB.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateStaticBuffer(device, g_sphereIB, std::size(g_sphereIB),
            D3D11_BIND_INDEX_BUFFER,
            m_shapeIB.ReleaseAndGetAddressOf()));

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

    m_effect->SetViewport( float(size.right), float(size.bottom) );

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_effect.reset();
    m_shapeVB.Reset();
    m_shapeIB.Reset();
    m_inputLayout.Reset();
    m_texture.Reset();
    m_texture2.Reset();
    m_pixelShader.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
