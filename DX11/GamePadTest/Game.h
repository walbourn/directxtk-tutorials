//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game() = default;

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
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_buttons;

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

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_roomTex;

    DirectX::Keyboard::KeyboardStateTracker         m_keys;
    DirectX::Mouse::ButtonStateTracker              m_mouseButtons;
};
