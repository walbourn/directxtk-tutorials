//
// Game.h
//

#pragma once

#include "pch.h"
#include <stdexcept>
#include "StepTimer.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop
class Game
{
public:

    Game();

    // Initialization and management
    void Initialize(HWND window);

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
    void OnWindowSizeChanged();

    void OnKeyPress(int vk);

    // Properites
    void GetDefaultSize( size_t& width, size_t& height ) const;

private:

    void Update(DX::StepTimer const& timer);
    void PostProcess();

    void CreateDevice();
    void CreateResources();
    
    void OnDeviceLost();


    // Application state
    HWND                                            m_window;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice1;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext1;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain1;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthStencil;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_backBuffer;

    // Game state
    DX::StepTimer                                   m_timer;

    // Tutorial
    std::unique_ptr<DirectX::CommonStates>           m_states;
    std::unique_ptr<DirectX::SpriteBatch>            m_spriteBatch;
    std::unique_ptr<DirectX::GeometricPrimitive>     m_shape;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_background;
    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_projection;
    RECT                                             m_fullscreenRect;

#if 1
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_bloomExtractPS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_bloomCombinePS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_gaussianBlurPS;

    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_bloomParams;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_blurParamsWidth;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_blurParamsHeight;
#endif

#if 1
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_sceneTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_sceneRT;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rt1SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_rt1RT;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rt2SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_rt2RT;

    RECT                                             m_bloomRect;
#endif

// SCREENSHOTS only
Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture1;
Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture2;
};