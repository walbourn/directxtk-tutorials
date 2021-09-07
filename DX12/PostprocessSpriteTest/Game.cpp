//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

#include "ReadData.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    struct VS_BLOOM_PARAMETERS
    {
        float bloomThreshold;
        float blurAmount;
        float bloomIntensity;
        float baseIntensity;
        float bloomSaturation;
        float baseSaturation;
        uint8_t na[8];
    };

    static_assert(!(sizeof(VS_BLOOM_PARAMETERS) % 16),
        "VS_BLOOM_PARAMETERS needs to be 16 bytes aligned");

    struct VS_BLUR_PARAMETERS
    {
        static constexpr size_t SAMPLE_COUNT = 15;

        XMFLOAT4 sampleOffsets[SAMPLE_COUNT];
        XMFLOAT4 sampleWeights[SAMPLE_COUNT];

        void SetBlurEffectParameters(float dx, float dy,
            const VS_BLOOM_PARAMETERS& params)
        {
            sampleWeights[0].x = ComputeGaussian(0, params.blurAmount);
            sampleOffsets[0].x = sampleOffsets[0].y = 0.f;

            float totalWeights = sampleWeights[0].x;

            // Add pairs of additional sample taps, positioned
            // along a line in both directions from the center.
            for (size_t i = 0; i < SAMPLE_COUNT / 2; i++)
            {
                // Store weights for the positive and negative taps.
                float weight = ComputeGaussian(float(i + 1.f), params.blurAmount);

                sampleWeights[i * 2 + 1].x = weight;
                sampleWeights[i * 2 + 2].x = weight;

                totalWeights += weight * 2;

                // To get the maximum amount of blurring from a limited number of
                // pixel shader samples, we take advantage of the bilinear filtering
                // hardware inside the texture fetch unit. If we position our texture
                // coordinates exactly halfway between two texels, the filtering unit
                // will average them for us, giving two samples for the price of one.
                // This allows us to step in units of two texels per sample, rather
                // than just one at a time. The 1.5 offset kicks things off by
                // positioning us nicely in between two texels.
                float sampleOffset = float(i) * 2.f + 1.5f;

                Vector2 delta = Vector2(dx, dy) * sampleOffset;

                // Store texture coordinate offsets for the positive and negative taps.
                sampleOffsets[i * 2 + 1].x = delta.x;
                sampleOffsets[i * 2 + 1].y = delta.y;
                sampleOffsets[i * 2 + 2].x = -delta.x;
                sampleOffsets[i * 2 + 2].y = -delta.y;
            }

            for (size_t i = 0; i < SAMPLE_COUNT; i++)
            {
                sampleWeights[i].x /= totalWeights;
            }
        }

    private:
        float ComputeGaussian(float n, float theta)
        {
            return (float)((1.0 / sqrtf(2 * XM_PI * theta))
                * expf(-(n * n) / (2 * theta * theta)));
        }
    };

    static_assert(!(sizeof(VS_BLUR_PARAMETERS) % 16),
        "VS_BLUR_PARAMETERS needs to be 16 bytes aligned");

    enum BloomPresets
    {
        Default = 0,
        Soft,
        Desaturated,
        Saturated,
        Blurry,
        Subtle,
        None
    };

    BloomPresets g_Bloom = Default;

    static const VS_BLOOM_PARAMETERS g_BloomPresets[] =
    {
        //Thresh  Blur Bloom  Base  BloomSat BaseSat
        { 0.25f,  4,   1.25f, 1,    1,       1 }, // Default
        { 0,      3,   1,     1,    1,       1 }, // Soft
        { 0.5f,   8,   2,     1,    0,       1 }, // Desaturated
        { 0.25f,  4,   2,     1,    2,       0 }, // Saturated
        { 0,      2,   1,     0.1f, 1,       1 }, // Blurry
        { 0.5f,   2,   1,     1,    1,       1 }, // Subtle
        { 0.25f,  4,   1.25f, 1,    1,       1 }, // None
    };
}

Game::Game() noexcept(false) :
    m_fullscreenRect{},
    m_bloomRect{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

#if 2
    const auto format = m_deviceResources->GetBackBufferFormat();
    m_offscreenTexture = std::make_unique<DX::RenderTexture>(format);
    m_renderTarget1 = std::make_unique<DX::RenderTexture>(format);
    m_renderTarget2 = std::make_unique<DX::RenderTexture>(format);
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

    //float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    auto totalTime = static_cast<float>(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationZ(totalTime / 2.f)
        * Matrix::CreateRotationY(totalTime)
        * Matrix::CreateRotationX(totalTime * 2.f);

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
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // TODO: Add your rendering code here.
    m_offscreenTexture->BeginScene(commandList);

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    m_spriteBatch->Begin(commandList);
    m_spriteBatch->Draw(
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background),
        GetTextureSize(m_background.Get()),
        m_fullscreenRect);
    m_spriteBatch->End();

    m_effect->SetMatrices(m_world, m_view, m_projection);
    m_effect->Apply(commandList);
    m_shape->Draw(commandList);

    m_offscreenTexture->EndScene(commandList);

    PostProcess(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present(D3D12_RESOURCE_STATE_PRESENT);
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
#if 2
    auto rtvDescriptor = m_renderDescriptors->GetCpuHandle(OffscreenRT);
#else
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
#endif
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    //commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Game::PostProcess(_In_ ID3D12GraphicsCommandList* commandList)
{
    if (g_Bloom == None)
    {
        auto renderTarget = m_deviceResources->GetRenderTarget();
        auto offscreenTarget = m_offscreenTexture->GetResource();

        ScopedBarrier barriers(commandList,
            {
                CD3DX12_RESOURCE_BARRIER::Transition(renderTarget,
                    D3D12_RESOURCE_STATE_PRESENT,
                    D3D12_RESOURCE_STATE_COPY_DEST, 0),
                CD3DX12_RESOURCE_BARRIER::Transition(offscreenTarget,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_COPY_SOURCE, 0)
            });

        // Pass-through test
        commandList->CopyResource(renderTarget, offscreenTarget);
    }
    else
    {
        enum RootParameterIndex
        {
            TextureSRV,
            ConstantBuffer,
            Texture2SRV,
            MyConstantBuffer,
        };

        // scene -> RT1 (downsample)
        m_renderTarget1->BeginScene(commandList);

        auto rtvDescriptor = m_renderDescriptors->GetCpuHandle(Blur1RT);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        auto vp = m_deviceResources->GetScreenViewport();

        Viewport halfvp(vp);
        halfvp.height /= 2.;
        halfvp.width /= 2.;
        commandList->RSSetViewports(1, halfvp.Get12());

        m_bloomExtract->Begin(commandList, SpriteSortMode_Immediate);
        commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::MyConstantBuffer,
            m_bloomParams.GpuAddress());
        auto sceneTex = m_resourceDescriptors->GetGpuHandle(SceneTex);
        m_bloomExtract->Draw(sceneTex,
            GetTextureSize(m_offscreenTexture->GetResource()),
            m_bloomRect);
        m_bloomExtract->End();

        m_renderTarget1->EndScene(commandList);

#if 0
        // RT1 (1st) screenshot
        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        commandList->RSSetViewports(1, &vp);

        m_spriteBatch->Begin(commandList);
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(BlurTex1), GetTextureSize(m_renderTarget1->GetResource()), m_bloomRect);
        m_spriteBatch->End();

        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
#else
        // RT1 -> RT2 (blur horizontal)
        m_renderTarget2->BeginScene(commandList);

        rtvDescriptor = m_renderDescriptors->GetCpuHandle(Blur2RT);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        m_gaussianBlur->Begin(commandList, SpriteSortMode_Immediate);
        commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::MyConstantBuffer,
            m_blurParamsWidth.GpuAddress());
        auto blur1Tex = m_resourceDescriptors->GetGpuHandle(BlurTex1);
        m_gaussianBlur->Draw(blur1Tex,
            GetTextureSize(m_renderTarget1->GetResource()),
            m_bloomRect);
        m_gaussianBlur->End();

        m_renderTarget2->EndScene(commandList);

#if 0
        // RT2 (1st) screenshot
        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        commandList->RSSetViewports(1, &vp);

        m_spriteBatch->Begin(commandList);
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(BlurTex2), GetTextureSize(m_renderTarget2->GetResource()), m_bloomRect);
        m_spriteBatch->End();

        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
#else
        // RT2 -> RT1 (blur vertical)
        m_renderTarget1->BeginScene(commandList);

        rtvDescriptor = m_renderDescriptors->GetCpuHandle(Blur1RT);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        m_gaussianBlur->Begin(commandList, SpriteSortMode_Immediate);
        commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::MyConstantBuffer,
            m_blurParamsHeight.GpuAddress());
        auto blur2Tex = m_resourceDescriptors->GetGpuHandle(BlurTex2);
        m_gaussianBlur->Draw(blur2Tex,
            GetTextureSize(m_renderTarget2->GetResource()),
            m_bloomRect);
        m_gaussianBlur->End();

        m_renderTarget1->EndScene(commandList);

#if 0
        // RT1 (2nd) screenshot
        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        commandList->RSSetViewports(1, &vp);

        m_spriteBatch->Begin(commandList);
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(BlurTex2), GetTextureSize(m_renderTarget2->GetResource()), m_bloomRect);
        m_spriteBatch->End();

        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
#else
        // RT1 + scene
        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        commandList->RSSetViewports(1, &vp);

        m_bloomCombine->Begin(commandList, SpriteSortMode_Immediate);
        commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::MyConstantBuffer,
            m_bloomParams.GpuAddress());
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::Texture2SRV, blur1Tex);
        m_bloomCombine->Draw(sceneTex,
            GetTextureSize(m_offscreenTexture->GetResource()),
            m_fullscreenRect);
        m_bloomCombine->End();

        TransitionResource(commandList, m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
#endif
#endif
#endif
    }
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
    m_shape = GeometricPrimitive::CreateTorus();

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

#if 2
    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);
#endif

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_shape->LoadStaticBuffers(device, resourceUpload);

    RenderTargetState sceneState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    {
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            sceneState);

        m_effect = std::make_unique<BasicEffect>(device, EffectFlags::Lighting, pd);
        m_effect->EnableDefaultLighting();
    }

    {
        SpriteBatchPipelineStateDescription pd(sceneState);
        m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
    }

#if 1
    {
        RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
            DXGI_FORMAT_UNKNOWN);

        SpriteBatchPipelineStateDescription pd(rtState);

        auto vsBlob = DX::ReadData(L"SpriteVertexShader.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(),
                IID_PPV_ARGS(m_rootSig.ReleaseAndGetAddressOf())));        

        pd.customRootSignature = m_rootSig.Get();
        pd.customVertexShader = { vsBlob.data(), vsBlob.size() };

        auto blob = DX::ReadData(L"BloomExtract.cso");
        pd.customPixelShader = { blob.data(), blob.size() };
        m_bloomExtract = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

        blob = DX::ReadData(L"BloomCombine.cso");
        pd.customPixelShader = { blob.data(), blob.size() };
        m_bloomCombine = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

        blob = DX::ReadData(L"GaussianBlur.cso");
        pd.customPixelShader = { blob.data(), blob.size() };
        m_gaussianBlur = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
    }
#endif

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, resourceUpload,
        L"sunset.jpg",
        m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    auto uploadResourcesFinished = resourceUpload.End(
        m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();

    m_view = Matrix::CreateLookAt(Vector3(0.f, 3.f, -3.f),
        Vector3::Zero, Vector3::UnitY);

#if 2
    m_offscreenTexture->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(SceneTex),
        m_renderDescriptors->GetCpuHandle(OffscreenRT));

    m_renderTarget1->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(BlurTex1),
        m_renderDescriptors->GetCpuHandle(Blur1RT));

    m_renderTarget2->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(BlurTex2),
        m_renderDescriptors->GetCpuHandle(Blur2RT));
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_fullscreenRect = size;

    auto vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);

    m_projection = Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4,
        float(size.right) / float(size.bottom), 0.01f, 100.f);

#if 1
    VS_BLUR_PARAMETERS blurData = {};
    blurData.SetBlurEffectParameters(1.f / (float(size.right) / 2), 0,
        g_BloomPresets[g_Bloom]);
    m_blurParamsWidth = m_graphicsMemory->AllocateConstant(blurData);

    blurData.SetBlurEffectParameters(0, 1.f / (float(size.bottom) / 2),
        g_BloomPresets[g_Bloom]);
    m_blurParamsHeight = m_graphicsMemory->AllocateConstant(blurData);

    m_bloomParams = m_graphicsMemory->AllocateConstant(g_BloomPresets[g_Bloom]);
    m_bloomCombine->SetViewport(vp);

    auto halfvp = vp;
    halfvp.Height /= 2.;
    halfvp.Width /= 2.;
    m_bloomExtract->SetViewport(halfvp);
    m_gaussianBlur->SetViewport(halfvp);
#endif

#if 2
    m_offscreenTexture->SetWindow(size);

    // Half-size blurring render targets
    m_bloomRect = { 0, 0, size.right / 2, size.bottom / 2 };

    m_renderTarget1->SetWindow(m_bloomRect);
    m_renderTarget2->SetWindow(m_bloomRect);
#endif
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_blurParamsWidth.Reset();
    m_blurParamsHeight.Reset();
    m_bloomParams.Reset();
    m_graphicsMemory.reset();

    m_states.reset();
    m_shape.reset();
    m_effect.reset();
    m_spriteBatch.reset();
    m_background.Reset();
    m_renderDescriptors.reset();

    m_bloomExtract.reset();
    m_bloomCombine.reset();
    m_gaussianBlur.reset();
    m_rootSig.Reset();

    m_blurParamsWidth.Reset();
    m_blurParamsHeight.Reset();

    m_offscreenTexture->ReleaseDevice();
    m_renderTarget1->ReleaseDevice();
    m_renderTarget2->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
