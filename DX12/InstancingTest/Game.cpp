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

#if 1
    auto time = static_cast<float>(m_timer.GetTotalSeconds());

    size_t j = 0;
    for (float y = -6.f; y < 6.f; y += 1.5f)
    {
        for (float x = -6.f; x < 6.f; x += 1.5f)
        {
            XMMATRIX m = XMMatrixTranslation(x,
                y,
                cos(time + float(x) * XM_PIDIV4)
                * sin(time + float(y) * XM_PIDIV4)
                * 2.f);
            XMStoreFloat3x4(&m_instanceTransforms[j], m);
            ++j;
        }
    }

    assert(j == m_instanceCount);
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
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // TODO: Add your rendering code here.
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    const size_t instBytes = m_instanceCount * sizeof(XMFLOAT3X4);
    GraphicsResource inst = m_graphicsMemory->Allocate(instBytes);
    memcpy(inst.Memory(), m_instanceTransforms.get(), instBytes);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferInst = {};
    vertexBufferInst.BufferLocation = inst.GpuAddress();
    vertexBufferInst.SizeInBytes = static_cast<UINT>(instBytes);
    vertexBufferInst.StrideInBytes = sizeof(XMFLOAT3X4);
    commandList->IASetVertexBuffers(1, 1, &vertexBufferInst);

    m_effect->Apply(commandList);

    m_shape->DrawInstanced(commandList, m_instanceCount);

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

    m_shape = GeometricPrimitive::CreateSphere();

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    const D3D12_INPUT_ELEMENT_DESC c_InputElements[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
        { "InstMatrix",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };

    const D3D12_INPUT_LAYOUT_DESC layout = { c_InputElements, static_cast<UINT>(std::size(c_InputElements)) };

    EffectPipelineStateDescription pd(
        &layout,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullNone,
        rtState);

    m_effect = std::make_unique<NormalMapEffect>(device,
        EffectFlags::Specular | EffectFlags::Instancing, pd);
    m_effect->EnableDefaultLighting();

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

    m_states = std::make_unique<CommonStates>(device);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_shape->LoadStaticBuffers(device, resourceUpload);

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"spnza_bricks_a.DDS",
        m_brickDiffuse.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_brickDiffuse.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::BrickDiffuse));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"spnza_bricks_a_normal.DDS",
        m_brickNormal.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_brickNormal.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::BrickNormal));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"spnza_bricks_a_specular.DDS",
        m_brickSpecular.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_brickSpecular.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::BrickSpecular));

    auto uploadResourcesFinished = resourceUpload.End(
        m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();

    m_effect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::BrickDiffuse),
        m_states->LinearClamp());
    m_effect->SetNormalTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::BrickNormal));
    m_effect->SetSpecularTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::BrickSpecular));

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
    m_graphicsMemory.reset();
    m_resourceDescriptors.reset();
    m_states.reset();
    m_effect.reset();
    m_shape.reset();
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
