//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

//#define ORBIT_STYLE

namespace
{
    const XMVECTORF32 START_POSITION = { 0.f, -1.5f, 0.f, 0.f };
    const XMVECTORF32 ROOM_BOUNDS = { 8.f, 6.f, 12.f, 0.f };
    constexpr float ROTATION_GAIN = 0.004f;
    constexpr float MOVEMENT_GAIN = 0.07f;

    // Orbit-style
    constexpr float c_defaultPhi = XM_2PI / 6.0f;
    constexpr float c_defaultRadius = 3.3f;
    constexpr float c_minRadius = 0.1f;
    constexpr float c_maxRadius = 5.f;
}

Game::Game() noexcept(false) :
    // FPS-style
    m_pitch(0),
    m_yaw(0),
    m_cameraPos(START_POSITION),
    // Orbit-style
    m_theta(0.f),
    m_phi(c_defaultPhi),
    m_radius(c_defaultRadius),
    m_roomColor(Colors::White)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
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

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
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
void Game::Update(DX::StepTimer const&)
{
    // TODO: Add your game logic here.
    auto mouse = m_mouse->GetState();
    m_mouseButtons.Update(mouse);

#ifdef ORBIT_STYLE
    m_radius -= float(mouse.scrollWheelValue) * ROTATION_GAIN;
    m_mouse->ResetScrollWheelValue();
    m_radius = std::max(c_minRadius, std::min(c_maxRadius, m_radius));
#endif

    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f)
            * ROTATION_GAIN;

#ifdef ORBIT_STYLE
        m_phi -= delta.y;
        m_theta -= delta.x;
#else
        m_pitch -= delta.y;
        m_yaw -= delta.x;
#endif
    }

    m_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);

    auto kb = m_keyboard->GetState();

    m_keys.Update(kb);

    if (kb.Escape)
    {
        ExitGame();
    }

    if (kb.Home)
    {
#ifdef ORBIT_STYLE
        m_theta = 0.f;
        m_phi = c_defaultPhi;
        m_radius = c_defaultRadius;
#else
        m_cameraPos = START_POSITION.v;
        m_yaw = m_pitch = 0.f;
#endif
    }

    Vector3 move = Vector3::Zero;

    if (kb.Up || kb.W)
        move.y += 1.f;

    if (kb.Down || kb.S)
        move.y -= 1.f;

    if (kb.Left || kb.A)
        move.x += 1.f;

    if (kb.Right || kb.D)
        move.x -= 1.f;

    if (kb.PageUp || kb.Space)
        move.z += 1.f;

    if (kb.PageDown || kb.X)
        move.z -= 1.f;

#ifdef ORBIT_STYLE
    move *= MOVEMENT_GAIN;

    m_phi -= move.y;
    m_theta -= move.x;

    // Invert the Z to get same 'closer' / 'further' behavior as we had before.
    m_radius -= move.z;
#else
    Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.f);

    move = Vector3::Transform(move, q);

    move *= MOVEMENT_GAIN;

    m_cameraPos += move;

    Vector3 halfBound = (Vector3(ROOM_BOUNDS.v) / Vector3(2.f))
        - Vector3(0.1f, 0.1f, 0.1f);

    m_cameraPos = Vector3::Min(m_cameraPos, halfBound);
    m_cameraPos = Vector3::Max(m_cameraPos, -halfBound);
#endif

#ifdef ORBIT_STYLE
    // limit pitch to straight up or straight down
    constexpr float limit = XM_PIDIV2 - 0.01f;
    m_phi = std::max(1e-2f, std::min(limit, m_phi));

    if (m_theta > XM_PI)
    {
        m_theta -= XM_2PI;
    }
    else if (m_theta < -XM_PI)
    {
        m_theta += XM_2PI;
    }

    XMVECTOR lookFrom = XMVectorSet(
        m_radius * sinf(m_phi) * cosf(m_theta),
        m_radius * cosf(m_phi),
        m_radius * sinf(m_phi) * sinf(m_theta),
        0);

    m_view = XMMatrixLookAtRH(lookFrom, g_XMZero, Vector3::Up);
#else
    // limit pitch to straight up or straight down
    constexpr float limit = XM_PIDIV2 - 0.01f;
    m_pitch = std::max(-limit, m_pitch);
    m_pitch = std::min(+limit, m_pitch);

    // keep longitude in sane range by wrapping
    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_2PI;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_2PI;
    }

    float y = sinf(m_pitch);
    float r = cosf(m_pitch);
    float z = r * cosf(m_yaw);
    float x = r * sinf(m_yaw);

    XMVECTOR lookAt = m_cameraPos + Vector3(x, y, z);

    m_view = XMMatrixLookAtRH(m_cameraPos, lookAt, Vector3::Up);
#endif

    if (m_keys.pressed.Tab || m_mouseButtons.rightButton == Mouse::ButtonStateTracker::PRESSED)
    {
        if (m_roomColor == Colors::Red.v)
            m_roomColor = Colors::Green;
        else if (m_roomColor == Colors::Green.v)
            m_roomColor = Colors::Blue;
        else if (m_roomColor == Colors::Blue.v)
            m_roomColor = Colors::White;
        else
            m_roomColor = Colors::Red;
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
    context;

    // TODO: Add your rendering code here.
    m_room->Draw(Matrix::Identity, m_view, m_proj, m_roomColor, m_roomTex.Get());

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
    m_keys.Reset();
    m_mouseButtons.Reset();
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
    m_keys.Reset();
    m_mouseButtons.Reset();
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

    // TODO: Initialize device dependent objects here (independent of window size).
    device;

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_room = GeometricPrimitive::CreateBox(context,
        XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]),
        false, true);

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"roomtexture.dds",
            nullptr, m_roomTex.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.

    auto size = m_deviceResources->GetOutputSize();
    m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(70.f),
        float(size.right) / float(size.bottom), 0.01f, 100.f);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_room.reset();
    m_roomTex.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
