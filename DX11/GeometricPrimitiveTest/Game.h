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

    DirectX::SimpleMath::Matrix             m_world;
    DirectX::SimpleMath::Matrix             m_view;
    DirectX::SimpleMath::Matrix             m_proj;

    std::unique_ptr<DirectX::GeometricPrimitive>        m_shape;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_texture;

    std::unique_ptr<DirectX::BasicEffect>               m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_inputLayout;
};
