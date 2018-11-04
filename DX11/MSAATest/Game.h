//
// Game.h
//

#pragma once

#include "StepTimer.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
{
public:

    Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();
    void Render();

    // Rendering helpers
    void Clear();
    void Present();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

    // Device resources.
    HWND                                            m_window;
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice1;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext1;

    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain1;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_renderTarget;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

    // Render scene.
    DirectX::SimpleMath::Matrix                     m_world;
    DirectX::SimpleMath::Matrix                     m_view;
    DirectX::SimpleMath::Matrix                     m_proj;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_shape;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_shape2;
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterState;
    RECT                                            m_outputSize;
    
    // MSAA
    DXGI_FORMAT                                     m_resolveFormat;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_resovleMSAA;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_resolveMSAASRV;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_renderTargetMSAA;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetViewMSAA;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilViewMSAA;
};