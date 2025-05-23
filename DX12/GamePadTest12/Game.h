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

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;


    std::unique_ptr<DirectX::GamePad>               m_gamePad;

    std::unique_ptr<DirectX::GeometricPrimitive>    m_room;
    DirectX::SimpleMath::Matrix                     m_view;
    DirectX::SimpleMath::Matrix                     m_proj;

    // FPS-style
    float                                           m_pitch;
    float                                           m_yaw;
    DirectX::SimpleMath::Vector3                    m_cameraPos;

    // Orbit-style
    float                                           m_theta;
    float                                           m_phi;
    float                                           m_radius;

    DirectX::SimpleMath::Color                      m_roomColor;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_roomTex;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptors;
    std::unique_ptr<DirectX::CommonStates>          m_states;
    std::unique_ptr<DirectX::BasicEffect>           m_roomEffect;

    DirectX::GamePad::ButtonStateTracker            m_buttons;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;
};
