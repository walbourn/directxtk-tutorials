//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#define USE_REVERB

namespace
{
    constexpr X3DAUDIO_CONE c_listenerCone = {
        X3DAUDIO_PI * 5.0f / 6.0f, X3DAUDIO_PI * 11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f
    };
    constexpr X3DAUDIO_CONE c_emitterCone = {
        0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f
    };

    constexpr X3DAUDIO_DISTANCE_CURVE_POINT c_emitter_LFE_CurvePoints[3] = {
        { 0.0f, 1.0f }, { 0.25f, 0.0f}, { 1.0f, 0.0f }
    };
    constexpr X3DAUDIO_DISTANCE_CURVE c_emitter_LFE_Curve = {
        const_cast<X3DAUDIO_DISTANCE_CURVE_POINT*>(&c_emitter_LFE_CurvePoints[0]), 3
    };

    constexpr X3DAUDIO_DISTANCE_CURVE_POINT c_emitter_Reverb_CurvePoints[3] = {
        { 0.0f, 0.5f}, { 0.75f, 1.0f }, { 1.0f, 0.0f }
    };
    constexpr X3DAUDIO_DISTANCE_CURVE c_emitter_Reverb_Curve = {
        const_cast<X3DAUDIO_DISTANCE_CURVE_POINT*>(&c_emitter_Reverb_CurvePoints[0]), 3
    };
}

Game::Game() noexcept(false) :
    m_position(0, 0, -10.f)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }

    m_soundSource.reset();

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

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags |= AudioEngine_Debug;
#endif

#ifdef USE_REVERB
    eflags |= AudioEngine_EnvironmentalReverb | AudioEngine_ReverbUseFilters | AudioEngine_UseMasteringLimiter;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

#ifdef USE_REVERB
    m_audEngine->SetReverb(Reverb_Hangar);
#endif

    m_retryAudio = false;

    if (!m_audEngine->IsAudioDevicePresent())
    {
        OutputDebugStringA("No default audio device found, running in 'silent mode'\n");
    }
    else
    {
        auto wfx = m_audEngine->GetOutputFormat();

        const char* speakerConfig;
        switch (wfx.dwChannelMask)
        {
        case SPEAKER_MONO:              speakerConfig = "Mono"; break;
        case SPEAKER_STEREO:            speakerConfig = "Stereo"; break;
        case SPEAKER_2POINT1:           speakerConfig = "2.1"; break;
        case SPEAKER_SURROUND:          speakerConfig = "Surround"; break;
        case SPEAKER_QUAD:              speakerConfig = "Quad"; break;
        case SPEAKER_4POINT1:           speakerConfig = "4.1"; break;
        case SPEAKER_5POINT1:           speakerConfig = "5.1"; break;
        case SPEAKER_7POINT1:           speakerConfig = "7.1"; break;
        case SPEAKER_5POINT1_SURROUND:  speakerConfig = "Surround5.1"; break;
        case SPEAKER_7POINT1_SURROUND:  speakerConfig = "Surround7.1"; break;
        default:                        speakerConfig = "(unknown)"; break;
        }

        char buff[128] = {};
        sprintf_s(buff, "Speaker config: %s\n", speakerConfig);
        OutputDebugStringA(buff);
    }

#if 1
    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"heli.wav");
#else
    // Copy tada.wav from windows\media for this test...
    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"tada.wav");
#endif

#if 0
    m_soundSource = m_soundEffect->CreateInstance(SoundEffectInstance_Use3D);
#endif

#if 0
    m_soundSource = m_soundEffect->CreateInstance(SoundEffectInstance_Use3D);
#endif

#ifdef USE_REVERB
    m_soundSource = m_soundEffect->CreateInstance(SoundEffectInstance_Use3D | SoundEffectInstance_ReverbUseFilters);
#else
    m_soundSource = m_soundEffect->CreateInstance(SoundEffectInstance_Use3D);
#endif

    m_soundSource->Play(true);

#ifdef USE_REVERB
    m_listener.pCone = const_cast<X3DAUDIO_CONE*>(&c_listenerCone);

    m_emitter.pLFECurve = const_cast<X3DAUDIO_DISTANCE_CURVE*>(&c_emitter_LFE_Curve);
    m_emitter.pReverbCurve = const_cast<X3DAUDIO_DISTANCE_CURVE*>(&c_emitter_Reverb_Curve);
    m_emitter.CurveDistanceScaler = 14.f;
    m_emitter.pCone = const_cast<X3DAUDIO_CONE*>(&c_emitterCone);
#endif

#if 1
    m_emitter.ChannelCount = m_soundEffect->GetFormat()->nChannels;
#endif
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

    if (m_retryAudio)
    {
        m_retryAudio = false;

        if (m_audEngine->Reset())
        {
            // TODO: restart any looped sounds here
            if (m_soundSource)
                m_soundSource->Play(true);
        }
    }
    else if (!m_audEngine->Update())
    {
        if (m_audEngine->IsCriticalError())
        {
            m_retryAudio = true;
        }
    }

#if 1
    // Move the object around in a circle.
    double speed = timer.GetTotalSeconds() / 2;
    m_position = Vector3(
        static_cast<float>(cos(speed)) * 5.f,
        0,
        static_cast<float>(sin(speed))) * -5.f;
#endif

    m_emitter.Update(m_position, Vector3::Up, static_cast<float>(timer.GetElapsedSeconds()));
    if (m_soundSource)
    {
        m_soundSource->Apply3D(m_listener, m_emitter);
    }

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
    XMMATRIX world = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    m_ballEffect->SetMatrices(world, m_view, m_proj);
    m_ballEffect->SetDiffuseColor(Colors::Yellow);
    m_ballEffect->Apply(commandList);
    m_ball->Draw(commandList);

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
    m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
    m_audEngine->Resume();
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

    m_ball = GeometricPrimitive::CreateSphere();

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    {
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState);

        m_ballEffect = std::make_unique<BasicEffect>(device,
            EffectFlags::Lighting, pd);
        m_ballEffect->EnableDefaultLighting();
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 20.f);

    m_view = Matrix::CreateLookAt(Vector3::Zero,
        Vector3::Forward, Vector3::Up);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_graphicsMemory.reset();
    m_ball.reset();
    m_ballEffect.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
