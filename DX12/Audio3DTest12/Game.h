//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    void OnNewAudioDevice() noexcept { m_retryAudio = true; }

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;

    std::unique_ptr<DirectX::AudioEngine>   m_audEngine;
    bool m_retryAudio;

    std::unique_ptr<DirectX::SoundEffect>   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance> m_soundSource;

    std::unique_ptr<DirectX::GeometricPrimitive> m_ball;
    std::unique_ptr<DirectX::BasicEffect> m_ballEffect;

    DirectX::SimpleMath::Matrix m_proj;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Vector3 m_position;

    DirectX::AudioListener m_listener;
    DirectX::AudioEmitter  m_emitter;
};
