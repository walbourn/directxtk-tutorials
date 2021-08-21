//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
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

    m_nightLoop.reset();
#if 0
    m_stream.reset();
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

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif
    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_retryAudio = false;

    if (!m_audEngine->IsAudioDevicePresent())
    {
    }

    m_explode = std::make_unique<SoundEffect>(m_audEngine.get(), L"explo1.wav");
    m_ambient = std::make_unique<SoundEffect>(m_audEngine.get(), L"NightAmbienceSimple_02.wav");
    m_sounds = std::make_unique<WaveBank>(m_audEngine.get(), L"sounds.xwb");

    std::random_device rd;
    m_random = std::make_unique<std::mt19937>(rd());
    explodeDelay = 2.f;

    //m_nightLoop = m_ambient->CreateInstance();
    m_nightLoop = m_sounds->CreateInstance("NightAmbienceSimple_02");
    if (m_nightLoop)
        m_nightLoop->Play(true);

    nightVolume = 1.f;
    nightSlide = -0.1f;

#if 0
    m_music = std::make_unique<WaveBank>(m_audEngine.get(), L"music.xwb");

    m_stream = m_music->CreateStreamInstance(0u);
    if (m_stream)
    {
        m_stream->SetVolume(0.5f);
        m_stream->Play(true);
    }
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
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
#if 1
    explodeDelay -= elapsedTime;
    if (explodeDelay < 0.f)
    {
        std::uniform_int_distribution<unsigned int> dist2(0, 3);
        m_sounds->Play( dist2(*m_random) );

        std::uniform_real_distribution<float> dist(1.f, 10.f);
        explodeDelay = dist(*m_random);
    }
#else
    explodeDelay -= elapsedTime;
    if (explodeDelay < 0.f)
    {
        m_explode->Play();

        std::uniform_real_distribution<float> dist(1.f, 10.f);

        explodeDelay = dist(*m_random);
    }
#endif

    nightVolume += elapsedTime * nightSlide;
    if (nightVolume < 0.f)
    {
        nightVolume = 0.f;
        nightSlide = -nightSlide;
    }
    else if (nightVolume > 1.f)
    {
        nightVolume = 1.f;
        nightSlide = -nightSlide;
    }
    m_nightLoop->SetVolume( nightVolume );

    if (m_retryAudio)
    {
        m_retryAudio = false;

        if (m_audEngine->Reset())
        {
            // TODO: restart any looped sounds here
            if (m_nightLoop)
                m_nightLoop->Play(true);
#if 0
            if (m_stream)
                m_stream->Play(true);
#endif
        }
    }
    else if (!m_audEngine->Update())
    {
        if (m_audEngine->IsCriticalError())
        {
            m_retryAudio = true;
        }
    }
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
    context;

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
    m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
    m_audEngine->Resume();
    explodeDelay = 2.f;
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
    // TODO: Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
