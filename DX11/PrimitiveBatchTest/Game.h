//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "MSAAHelper.h"


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
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

#if 1
    using VertexType = DirectX::VertexPositionColor;
#elif 0
    using VertexType = DirectX::VertexPositionTexture;
#else
    using VertexType = DirectX::VertexPositionNormalTexture;
#endif

    std::unique_ptr<DirectX::CommonStates>          m_states;

#if 1
    std::unique_ptr<DirectX::BasicEffect>           m_effect;
#else
    std::unique_ptr<DirectX::NormalMapEffect>       m_effect;
#endif

    std::unique_ptr<DirectX::PrimitiveBatch<VertexType>>    m_batch;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>           m_raster;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>               m_inputLayout;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMap;

    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_proj;

    // MSAA
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_offscreenRenderTarget;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_offscreenRenderTargetSRV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilSRV;

    std::unique_ptr<DX::MSAAHelper> m_msaaHelper;
};