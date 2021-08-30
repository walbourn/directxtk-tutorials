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

    //float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    auto time = static_cast<float>(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationY(cosf(time) * 2.f);

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
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
    m_hdrScene->BeginScene(commandList);

    Clear();

    // TODO: Add your rendering code here.
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    m_effect->SetWorld(m_world);
    m_effect->Apply(commandList);
    m_shape->Draw(commandList);

    m_hdrScene->EndScene(commandList);

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Tone-mapping");

    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    m_toneMap->Process(commandList);

    PIXEndEvent(commandList);

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
    auto rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::HDRScene);
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    m_hdrScene->Clear(commandList);
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
    m_shape = GeometricPrimitive::CreateSphere();

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

    m_toneMap = std::make_unique<ToneMapPostProcess>(device,
        rtState,
        ToneMapPostProcess::Reinhard, ToneMapPostProcess::SRGB);

    RenderTargetState hdrState(m_hdrScene->GetFormat(),
        m_deviceResources->GetDepthBufferFormat());

    {
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            hdrState);

#if 1
        m_effect = std::make_unique<PBREffect>(device,
            EffectFlags::Texture | EffectFlags::Emissive, pd);
#else
        m_effect = std::make_unique<PBREffect>(device, EffectFlags::None, pd);
#endif
    }

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_shape->LoadStaticBuffers(device, resourceUpload);

#if 1
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload,
            L"Sphere2Mat_baseColor.png",
            m_albetoMap.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_albetoMap.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::AlbetoMap));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload,
            L"Sphere2Mat_normal.png",
            m_normalMap.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_normalMap.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::NormalMap));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload,
            L"Sphere2Mat_occlusionRoughnessMetallic.png",
            m_rmaMap.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_rmaMap.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::RMAMap));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload,
            L"Sphere2Mat_emissive.png",
            m_emissiveMap.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_emissiveMap.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::EmissiveMap));
#endif

    // Image-based lighting cubemaps.
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            L"SunSubMixer_diffuseIBL.dds",
            m_radiance.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_radiance.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::RadianceIBL), true);

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            L"SunSubMixer_specularIBL.dds",
            m_irradiance.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_irradiance.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::IrradianceIBL), true);

    auto uploadResourcesFinished = resourceUpload.End(
        m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();

    auto desc = m_radiance->GetDesc();

    auto radianceTex = m_resourceDescriptors->GetGpuHandle(Descriptors::RadianceIBL);
    auto irradianceTex = m_resourceDescriptors->GetGpuHandle(Descriptors::IrradianceIBL);

    m_effect->SetIBLTextures(radianceTex, desc.MipLevels, irradianceTex, m_states->AnisotropicClamp());

#if 1
    m_effect->SetSurfaceTextures(
        m_resourceDescriptors->GetGpuHandle(Descriptors::AlbetoMap),
        m_resourceDescriptors->GetGpuHandle(Descriptors::NormalMap),
        m_resourceDescriptors->GetGpuHandle(Descriptors::RMAMap),
        m_states->AnisotropicClamp());

    m_effect->SetEmissiveTexture(
        m_resourceDescriptors->GetGpuHandle(Descriptors::EmissiveMap));
#endif

    m_world = Matrix::Identity;
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
    auto sceneTex = m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex);
    m_toneMap->SetHDRSourceTexture(sceneTex);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_graphicsMemory.reset();
    m_states.reset();
    m_shape.reset();
    m_effect.reset();
    m_radiance.Reset();
    m_irradiance.Reset();
    m_hdrScene->ReleaseDevice();
    m_toneMap.reset();
    m_resourceDescriptors.reset();
    m_renderDescriptors.reset();

    m_albetoMap.Reset();
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
