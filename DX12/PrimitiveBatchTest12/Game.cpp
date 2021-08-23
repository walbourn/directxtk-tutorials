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
#define MSAA_HELPER

#ifdef MSAA
namespace
{
    constexpr UINT MSAA_COUNT = 4;
    constexpr UINT MSAA_QUALITY = 0;
    constexpr DXGI_FORMAT MSAA_DEPTH_FORMAT = DXGI_FORMAT_D32_FLOAT;
}
#endif

Game::Game() noexcept(false)
{
#ifdef MSAA
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>();
#endif
    m_deviceResources->RegisterDeviceNotify(this);

#if defined(MSAA) && defined(MSAA_HELPER)
    m_msaaHelper = std::make_unique<DX::MSAAHelper>(
        m_deviceResources->GetBackBufferFormat(),
        DXGI_FORMAT_D32_FLOAT,
        MSAA_COUNT);

    m_msaaHelper->SetClearColor(Colors::CornflowerBlue);
#endif
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

    auto time = static_cast<float>(timer.GetTotalSeconds());

#if 1
    m_world = Matrix::CreateRotationY(cosf(time));
#elif 0

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    auto quat = Quaternion::CreateFromYawPitchRoll(pitch, yaw, roll);

    auto light = XMVector3Rotate(g_XMOne, quat);

    m_effect->SetLightDirection(0, light);
#endif

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
#if defined(MSAA) && !defined(MSAA_HELPER)
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST);
#else
    m_deviceResources->Prepare();
#endif

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

#ifdef MSAA 
#ifdef MSAA_HELPER
    m_msaaHelper->Prepare(commandList);
#else
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenRenderTarget.Get(),
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);
#endif
#endif

    Clear();

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // TODO: Add your rendering code here.
#if 1
    m_effect->SetWorld(m_world);
#endif

    m_effect->Apply(commandList);

    m_batch->Begin(commandList);

#if 0
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
#elif 0
    VertexPositionTexture v1(Vector3(400.f, 150.f, 0.f), Vector2(.5f, 0));
    VertexPositionTexture v2(Vector3(600.f, 450.f, 0.f), Vector2(1, 1));
    VertexPositionTexture v3(Vector3(200.f, 450.f, 0.f), Vector2(0, 1));
#else
    VertexPositionNormalTexture v1(Vector3(400.f, 150.f, 0.f), -Vector3::UnitZ, Vector2(.5f, 0));
    VertexPositionNormalTexture v2(Vector3(600.f, 450.f, 0.f), -Vector3::UnitZ, Vector2(1, 1));
    VertexPositionNormalTexture v3(Vector3(200.f, 450.f, 0.f), -Vector3::UnitZ, Vector2(0, 1));
#endif

    m_batch->DrawTriangle(v1, v2, v3);
#endif

#if 1
    Vector3 xaxis(2.f, 0.f, 0.f);
    Vector3 yaxis(0.f, 0.f, 2.f);
    Vector3 origin = Vector3::Zero;

    size_t divisions = 20;

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
#endif

    m_batch->End();

#ifdef MSAA
#ifdef MSAA_HELPER
    m_msaaHelper->Resolve(commandList, m_deviceResources->GetRenderTarget());
#else
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_offscreenRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    commandList->ResourceBarrier(1, &barrier);

    commandList->ResolveSubresource(m_deviceResources->GetRenderTarget(),
        0, m_offscreenRenderTarget.Get(), 0, m_deviceResources->GetBackBufferFormat());
#endif
#endif

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
#ifdef MSAA
#ifdef MSAA_HELPER
    m_deviceResources->Present(D3D12_RESOURCE_STATE_PRESENT);
#else
    m_deviceResources->Present(D3D12_RESOURCE_STATE_RESOLVE_DEST);
#endif
#else
    m_deviceResources->Present();
#endif
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
#ifdef MSAA
#ifdef MSAA_HELPER
    auto rtvDescriptor = m_msaaHelper->GetMSAARenderTargetView();
    auto dsvDescriptor = m_msaaHelper->GetMSAADepthStencilView();
#else
    auto rtvDescriptor = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto dsvDescriptor = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
#endif
#else
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();
#endif

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
#ifdef MSAA
#ifdef MSAA_HELPER
    m_msaaHelper->SetDevice(device);
#else
    // Create descriptor heaps for MSAA.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = 1;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    DX::ThrowIfFailed(device->CreateDescriptorHeap(
        &rtvDescriptorHeapDesc,
        IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(device->CreateDescriptorHeap(
        &dsvDescriptorHeapDesc,
        IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));
#endif
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_states = std::make_unique<CommonStates>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, resourceUpload, L"rocks.jpg",
            m_texture.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_texture.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Rocks));

#if 0
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload, L"rocks_normalmap.dds",
            m_normalMap.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_normalMap.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::NormalMap));
#endif

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();

    m_batch = std::make_unique<PrimitiveBatch<VertexType>>(device);

#ifdef MSAA
#ifdef MSAA_HELPER
    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_msaaHelper->GetDepthBufferFormat());
    rtState.sampleDesc.Count = m_msaaHelper->GetSampleCount();
#else
    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), MSAA_DEPTH_FORMAT);
    rtState.sampleDesc.Count = MSAA_COUNT;
    rtState.sampleDesc.Quality = MSAA_QUALITY;
#endif
#else
    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());
#endif
    
#if 0
    EffectPipelineStateDescription pd(
        &VertexType::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
#if 0
        CommonStates::CullNone,
#endif
#if 0
        CommonStates::CullClockwise,
#endif
#if 1
        CommonStates::CullCounterClockwise,
#endif
        rtState);
#endif

#if 0
    EffectPipelineStateDescription pd(
        &VertexType::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullNone,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
#endif

#if 1
#if 0
    // AA lines
    CD3DX12_RASTERIZER_DESC rastDesc(D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_NONE, FALSE,
        D3D12_DEFAULT_DEPTH_BIAS, D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE, TRUE,
        0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
#else
    // MSAA
    CD3DX12_RASTERIZER_DESC rastDesc(D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_NONE, FALSE,
        D3D12_DEFAULT_DEPTH_BIAS, D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, TRUE, FALSE,
        0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
#endif

    EffectPipelineStateDescription pd(
        &VertexType::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        rastDesc,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
#endif

#if 1
    m_effect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
#elif 0
    m_effect = std::make_unique<BasicEffect>(device, EffectFlags::Texture, pd);
    m_effect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::Rocks), m_states->LinearClamp());
#else
    m_effect = std::make_unique<NormalMapEffect>(device, EffectFlags::None, pd);
    m_effect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::Rocks), m_states->LinearClamp());
    m_effect->SetNormalTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::NormalMap));
    m_effect->EnableDefaultLighting();
    m_effect->SetLightDiffuseColor(0, Colors::Gray);
#endif

    m_world = Matrix::Identity;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();

#ifdef MSAA
#ifdef MSAA_HELPER
    m_msaaHelper->SetWindow(size);
#else
    auto device = m_deviceResources->GetD3DDevice();

    // Create the MSAA depth/stencil buffer.
    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    auto depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        MSAA_DEPTH_FORMAT,
        static_cast<UINT>(size.right),
        static_cast<UINT>(size.bottom),
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level
        MSAA_COUNT,
        MSAA_QUALITY
    );
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = MSAA_DEPTH_FORMAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
        ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = MSAA_DEPTH_FORMAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

    device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // Create the MSAA render target.
    auto msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        m_deviceResources->GetBackBufferFormat(),
        static_cast<UINT>(size.right),
        static_cast<UINT>(size.bottom),
        1, // This render target view has only one texture.
        1, // Use a single mipmap level
        MSAA_COUNT,
        MSAA_QUALITY
    );
    msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE msaaOptimizedClearValue = {};
    msaaOptimizedClearValue.Format = m_deviceResources->GetBackBufferFormat();
    memcpy(msaaOptimizedClearValue.Color, Colors::CornflowerBlue, sizeof(float) * 4);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &msaaRTDesc,
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        &msaaOptimizedClearValue,
        IID_PPV_ARGS(m_offscreenRenderTarget.ReleaseAndGetAddressOf())
    ));

    device->CreateRenderTargetView(m_offscreenRenderTarget.Get(), nullptr, m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
#endif
#endif

#if 0
    Matrix proj = Matrix::CreateScale(2.f / float(size.right),
        -2.f / float(size.bottom), 1.f)
        * Matrix::CreateTranslation(-1.f, 1.f, 0.f);
    m_effect->SetProjection(proj);
#endif

#if 1
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f),
        Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
#endif
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_graphicsMemory.reset();
    m_effect.reset();
    m_batch.reset();
    m_texture.Reset();
    m_normalMap.Reset();
    m_resourceDescriptors.reset();

    m_rtvDescriptorHeap.Reset();
    m_dsvDescriptorHeap.Reset();
    m_depthStencil.Reset();
    m_offscreenRenderTarget.Reset();

#if defined(MSAA) && defined(MSAA_HELPER)
    m_msaaHelper->ReleaseDevice();
#endif
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion