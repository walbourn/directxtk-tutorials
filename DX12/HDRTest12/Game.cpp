//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
#if 0
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R10G10B10A2_UNORM);
#endif

#if 0
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_11_0,
        DX::DeviceResources::c_EnableHDR);
#endif

#if 1
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_11_0,
        DX::DeviceResources::c_EnableHDR);
#endif

    m_deviceResources->RegisterDeviceNotify(this);

    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);

    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
    m_hdrScene->SetClearColor(color);
}

Game::~Game()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    elapsedTime;

    float time = float(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationZ(cosf(time) * 2.f);

    float colorScale = 1.f + cosf(time);
    m_effect->SetDiffuseColor(XMVectorSetW(Colors::White * colorScale, 1.f));

    PIXEndEvent();
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

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    
    auto commandList = m_deviceResources->GetCommandList();
    m_hdrScene->BeginScene(commandList);

    Clear();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // TODO: Add your rendering code here.
    m_effect->SetMatrices(m_world, m_view, m_proj);

    m_effect->Apply(commandList);

    m_shape->Draw(commandList);

    m_hdrScene->EndScene(commandList);

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Tonemap");

    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

#if 0
    m_toneMap->Process(commandList);
#endif

#if 1
    switch (m_deviceResources->GetColorSpace())
    {
    default:
        m_toneMap->Process(commandList);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        m_toneMapHDR10->Process(commandList);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        // Required if R16G16B16A16_FLOAT is used as display format
        // (otherwise you can omit this case)
        m_toneMapLinear->Process(commandList);
        break;
    }
#endif

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(m_deviceResources->GetCommandQueue(), PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent(m_deviceResources->GetCommandQueue());
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::HDRScene);
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
    commandList->ClearRenderTargetView(rtvDescriptor, color, 0, nullptr);

    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
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
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);

    m_hdrScene->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::SceneTex),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::HDRScene));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        DXGI_FORMAT_UNKNOWN);

#if 0
    // Set tone-mapper as 'pass-through' for now...
    m_toneMap = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::None, ToneMapPostProcess::SRGB);
#endif

#if 0
    // Set the tone-mapper to use Reinhard
    m_toneMap = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::Reinhard, ToneMapPostProcess::SRGB);
#endif

#if 0
    // Set the tone-mapper to ACES Filmic
    m_toneMap = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::ACESFilmic, ToneMapPostProcess::SRGB);
#endif

#if 1
    m_toneMap = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::ACESFilmic,
        (m_deviceResources->GetBackBufferFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT)
        ? ToneMapPostProcess::Linear : ToneMapPostProcess::SRGB);

    m_toneMapHDR10 = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::None, ToneMapPostProcess::ST2084);

    m_toneMapLinear = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::None, ToneMapPostProcess::Linear);
#endif

    RenderTargetState hdrState(m_hdrScene->GetFormat(), m_deviceResources->GetDepthBufferFormat());

    EffectPipelineStateDescription pd(
        &GeometricPrimitive::VertexType::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullNone,
        hdrState);

    m_effect = std::make_unique<BasicEffect>(device, EffectFlags::Lighting, pd);
    m_effect->EnableDefaultLighting();

    m_shape = GeometricPrimitive::CreateTeapot();

    m_world = Matrix::Identity;

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.

    auto size = m_deviceResources->GetOutputSize();
    m_hdrScene->SetWindow(size);

    auto sceneTex = m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex);
    m_toneMap->SetHDRSourceTexture(sceneTex);
    m_toneMapHDR10->SetHDRSourceTexture(sceneTex);
    m_toneMapLinear->SetHDRSourceTexture(sceneTex);

    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f),
        Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10.f);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_hdrScene->ReleaseDevice();
    m_toneMap.reset();
    m_resourceDescriptors.reset();
    m_renderDescriptors.reset();
    m_shape.reset();
    m_effect.reset();

    m_toneMapHDR10.reset();
    m_toneMapLinear.reset();

    m_graphicsMemory.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
