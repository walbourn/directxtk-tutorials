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
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // TODO: Add your rendering code here.
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

#if 0
    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_screenPos, nullptr, Colors::White, 0.f, m_origin);

    m_spriteBatch->End();
#endif

#if 0
    float time = float(m_timer.GetTotalSeconds());

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_screenPos, nullptr, Colors::White, cosf(time) * 4.f, m_origin);

    m_spriteBatch->End();
#endif

#if 0
    float time = float(m_timer.GetTotalSeconds());

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_screenPos, nullptr, Colors::White, 0.f, m_origin, cosf(time) + 2.f);

    m_spriteBatch->End();
#endif

#if 0
    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_screenPos, nullptr, Colors::Green, 0.f, m_origin);

    m_spriteBatch->End();
#endif

#if 0
    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_screenPos, &m_tileRect, Colors::White, 0.f, m_origin);

    m_spriteBatch->End();
#endif

#if 1
    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_stretchRect, nullptr, Colors::White);

    m_spriteBatch->End();
#endif

#if 0
    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background),
        GetTextureSize(m_background.Get()),
        m_fullscreenRect);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
        GetTextureSize(m_texture.Get()),
        m_screenPos, nullptr, Colors::White, 0.f, m_origin);

    m_spriteBatch->End();
#endif

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
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

    // Check Shader Model 6 support
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }

    // TODO: Initialize device dependent objects here (independent of window size).
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_states = std::make_unique<CommonStates>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

#if 0
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"cat.png",
            m_texture.ReleaseAndGetAddressOf(), false));
#else
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload, L"cat.dds",
            m_texture.ReleaseAndGetAddressOf()));
#endif

    CreateShaderResourceView(device, m_texture.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Cat));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"sunset.jpg",
            m_background.ReleaseAndGetAddressOf(), false));

    CreateShaderResourceView(device, m_background.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

#if 0
    SpriteBatchPipelineStateDescription pd(rtState);
#endif

#if 0
    SpriteBatchPipelineStateDescription pd(rtState,
        &CommonStates::NonPremultiplied);
#endif

#if 1
    auto sampler = m_states->LinearWrap();
    SpriteBatchPipelineStateDescription pd(rtState,
        nullptr, nullptr, nullptr, &sampler);
#endif

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    XMUINT2 catSize = GetTextureSize(m_texture.Get());

#if 0
    m_origin.x = float(catSize.x / 2);
    m_origin.y = float(catSize.y / 2);
#else
    m_origin.x = float(catSize.x * 2);
    m_origin.y = float(catSize.y * 2);

    m_tileRect.left = catSize.x * 2;
    m_tileRect.right = catSize.x * 6;
    m_tileRect.top = catSize.y * 2;
    m_tileRect.bottom = catSize.y * 6;
#endif

    auto uploadResourcesFinished = resourceUpload.End(
        m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

    auto size = m_deviceResources->GetOutputSize();
    m_screenPos.x = float(size.right) / 2.f;
    m_screenPos.y = float(size.bottom) / 2.f;

    m_fullscreenRect = m_deviceResources->GetOutputSize();

    m_stretchRect.left = size.right / 4;
    m_stretchRect.top = size.bottom / 4;
    m_stretchRect.right = m_stretchRect.left + size.right / 2;
    m_stretchRect.bottom = m_stretchRect.top + size.bottom / 2;
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_spriteBatch.reset();
    m_background.Reset();
    m_texture.Reset();
    m_resourceDescriptors.reset();
    m_states.reset();
    m_graphicsMemory.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
