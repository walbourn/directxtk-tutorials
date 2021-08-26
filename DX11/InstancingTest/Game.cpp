//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false) :
    m_instanceCount(0)
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

    // Update instance transforms.
#if 1
    auto time = static_cast<float>(m_timer.GetTotalSeconds());

    size_t j = 0;
    for (float y = -6.f; y < 6.f; y += 1.5f)
    {
        for (float x = -6.f; x < 6.f; x += 1.5f)
        {
            XMMATRIX m = XMMatrixTranslation(x, y, cos(time + float(x) * XM_PIDIV4) * sin(time + float(y) * XM_PIDIV4) * 2.f);
            XMStoreFloat3x4(&m_instanceTransforms[j], m);
            ++j;
        }
    }

    assert(j == m_instanceCount);
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
#if 1
    {
        MapGuard map(context, m_instancedVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0);
        memcpy(map.pData, m_instanceTransforms.get(), m_instanceCount * sizeof(XMFLOAT3X4));
    }
#endif

    m_shape->DrawInstanced(m_effect.get(), m_instanceLayout.Get(), m_instanceCount, false, false, 0, [=]()
        {
            UINT stride = sizeof(XMFLOAT3X4);
            UINT offset = 0;
            context->IASetVertexBuffers(1, 1, m_instancedVB.GetAddressOf(), &stride, &offset);
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
    auto context = m_deviceResources->GetD3DDeviceContext();
    m_shape = GeometricPrimitive::CreateSphere(context);

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a.DDS",
        nullptr, m_brickDiffuse.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a_normal.DDS",
        nullptr, m_brickNormal.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a_specular.DDS",
        nullptr, m_brickSpecular.ReleaseAndGetAddressOf()));

    m_effect = std::make_unique<NormalMapEffect>(device);
    m_effect->EnableDefaultLighting();
    m_effect->SetTexture(m_brickDiffuse.Get());
    m_effect->SetNormalTexture(m_brickNormal.Get());
    m_effect->SetSpecularTexture(m_brickSpecular.Get());
    m_effect->SetInstancingEnabled(true);

    const D3D11_INPUT_ELEMENT_DESC c_InputElements[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "InstMatrix",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect(device, m_effect.get(),
            c_InputElements, std::size(c_InputElements),
            m_instanceLayout.ReleaseAndGetAddressOf()));

    // Create instance transforms.
    {
        size_t j = 0;
        for (float y = -6.f; y < 6.f; y += 1.5f)
        {
            for (float x = -6.f; x < 6.f; x += 1.5f)
            {
                ++j;
            }
        }
        m_instanceCount = static_cast<UINT>(j);

        m_instanceTransforms = std::make_unique<XMFLOAT3X4[]>(j);

        constexpr XMFLOAT3X4 s_identity = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f };

        j = 0;
        for (float y = -6.f; y < 6.f; y += 1.5f)
        {
            for (float x = -6.f; x < 6.f; x += 1.5f)
            {
                m_instanceTransforms[j] = s_identity;
                m_instanceTransforms[j]._14 = x;
                m_instanceTransforms[j]._24 = y;
                ++j;
            }
        }

        auto desc = CD3D11_BUFFER_DESC(
            static_cast<UINT>(j * sizeof(XMFLOAT3X4)),
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        D3D11_SUBRESOURCE_DATA initData = { m_instanceTransforms.get(), 0, 0 };

        DX::ThrowIfFailed(
            device->CreateBuffer(&desc, &initData, m_instancedVB.ReleaseAndGetAddressOf())
        );
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_view = Matrix::CreateLookAt(Vector3(0.f, 0.f, 12.f),
        Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 25.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_effect.reset();
    m_shape.reset();
    m_instanceLayout.Reset();
    m_instancedVB.Reset();
    m_brickDiffuse.Reset();
    m_brickNormal.Reset();
    m_brickSpecular.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
