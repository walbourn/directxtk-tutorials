//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false) :
    m_leftBackWheelBone(ModelBone::c_Invalid),
    m_rightBackWheelBone(ModelBone::c_Invalid),
    m_leftFrontWheelBone(ModelBone::c_Invalid),
    m_rightFrontWheelBone(ModelBone::c_Invalid),
    m_leftSteerBone(ModelBone::c_Invalid),
    m_rightSteerBone(ModelBone::c_Invalid),
    m_turretBone(ModelBone::c_Invalid),
    m_cannonBone(ModelBone::c_Invalid),
    m_hatchBone(ModelBone::c_Invalid)
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
    // TODO: Add your game logic here.
    float time = float(timer.GetTotalSeconds());

    m_world = XMMatrixRotationY(time * 0.1f);

#if 1
    float wheelRotation = time * 5.f;
    float steerRotation = sinf(time * 0.75f) * 0.5f;
    float turretRotation = sinf(time * 0.333f) * 1.25f;
    float cannonRotation = sinf(time * 0.25f) * 0.333f - 0.333f;
    float hatchRotation = std::min(0.f, std::max(sinf(time * 2.f) * 2.f, -1.f));

    XMMATRIX mat = XMMatrixRotationX(wheelRotation);
    m_animBones[m_leftFrontWheelBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_leftFrontWheelBone]);
    m_animBones[m_rightFrontWheelBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_rightFrontWheelBone]);
    m_animBones[m_leftBackWheelBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_leftBackWheelBone]);
    m_animBones[m_rightBackWheelBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_rightBackWheelBone]);

    mat = XMMatrixRotationX(steerRotation);
    m_animBones[m_leftSteerBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_leftSteerBone]);
    m_animBones[m_rightSteerBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_rightSteerBone]);

    mat = XMMatrixRotationY(turretRotation);
    m_animBones[m_turretBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_turretBone]);

    mat = XMMatrixRotationX(cannonRotation);
    m_animBones[m_cannonBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_cannonBone]);

    mat = XMMatrixRotationX(hatchRotation);
    m_animBones[m_hatchBone] = XMMatrixMultiply(mat,
        m_model->boneMatrices[m_hatchBone]);
#endif
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
#if 0
    m_model->Draw(context, *m_states, m_world, m_view, m_proj);
#else
    size_t nbones = m_model->bones.size();

    m_model->CopyAbsoluteBoneTransforms(nbones,
        m_animBones.get(), m_drawBones.get());

    m_model->Draw(context, *m_states, nbones, m_drawBones.get(),
        m_world, m_view, m_proj);
#endif

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
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

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
    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);

    m_model = Model::CreateFromSDKMESH(device, L"tank.sdkmesh", *m_fxFactory,
        ModelLoader_CounterClockwise | ModelLoader_IncludeBones);

    m_world = Matrix::Identity;

#if 1
    const size_t nbones = m_model->bones.size();

    m_drawBones = ModelBone::MakeArray(nbones);
    m_animBones = ModelBone::MakeArray(nbones);

    m_model->CopyBoneTransformsTo(nbones, m_animBones.get());

#if 1
    uint32_t index = 0;
    for (const auto& it : m_model->bones)
    {
        if (_wcsicmp(it.name.c_str(), L"tank_geo") == 0)
        {
            // Need to recenter the model.
            m_animBones[index] = XMMatrixIdentity();
        }
#if 1
        else if (_wcsicmp(it.name.c_str(), L"l_back_wheel_geo") == 0) { m_leftBackWheelBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"r_back_wheel_geo") == 0) { m_rightBackWheelBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"l_front_wheel_geo") == 0) { m_leftFrontWheelBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"r_front_wheel_geo") == 0) { m_rightFrontWheelBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"l_steer_geo") == 0) { m_leftSteerBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"r_steer_geo") == 0) { m_rightSteerBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"turret_geo") == 0) { m_turretBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"canon_geo") == 0) { m_cannonBone = index; }
        else if (_wcsicmp(it.name.c_str(), L"hatch_geo") == 0) { m_hatchBone = index; }
#endif

        ++index;
    }

#endif
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_view = Matrix::CreateLookAt(Vector3(1000, 500, 0),
        Vector3(0, 150, 0), Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10000.f);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_fxFactory.reset();
    m_model.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
