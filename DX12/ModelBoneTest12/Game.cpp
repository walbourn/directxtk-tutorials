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

Game::~Game()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float time = float(timer.GetTotalSeconds());

    float wheelRotation = time * 5.f;
    float steerRotation = sinf(time * 0.75f) * 0.5f;
    float turretRotation = sinf(time * 0.333f) * 1.25f;
    float cannonRotation = sinf(time * 0.25f) * 0.333f - 0.333f;
    float hatchRotation = std::min(0.f, std::max(sinf(time * 2.f) * 2.f, -1.f));

    XMMATRIX mat = XMMatrixRotationX(wheelRotation);
    m_animBones[m_leftFrontWheelBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_leftFrontWheelBone]);
    m_animBones[m_rightFrontWheelBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_rightFrontWheelBone]);
    m_animBones[m_leftBackWheelBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_leftBackWheelBone]);
    m_animBones[m_rightBackWheelBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_rightBackWheelBone]);

    mat = XMMatrixRotationX(steerRotation);
    m_animBones[m_leftSteerBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_leftSteerBone]);
    m_animBones[m_rightSteerBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_rightSteerBone]);

    mat = XMMatrixRotationY(turretRotation);
    m_animBones[m_turretBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_turretBone]);

    mat = XMMatrixRotationX(cannonRotation);
    m_animBones[m_cannonBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_cannonBone]);

    mat = XMMatrixRotationX(hatchRotation);
    m_animBones[m_hatchBone] = XMMatrixMultiply(mat, m_model->boneMatrices[m_hatchBone]);

    m_world = XMMatrixRotationY(time * 0.1f);

    PIXEndEvent();
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

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // TODO: Add your rendering code here.
    size_t nbones = m_model->bones.size();

    m_model->CopyAbsoluteBoneTransforms(nbones, m_animBones.get(), m_drawBones.get());

    ID3D12DescriptorHeap* heaps[] = { m_modelResources->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    Model::UpdateEffectMatrices(m_modelNormal, m_world, m_view, m_proj);

    m_model->Draw(commandList, nbones, m_drawBones.get(), m_world, m_modelNormal.cbegin());

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
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

    // Check Shader Model 6 support
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }

    // TODO: Initialize device dependent objects here (independent of window size).
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_states = std::make_unique<CommonStates>(device);

    m_model = Model::CreateFromSDKMESH(device, L"tank.sdkmesh", ModelLoader_IncludeBones);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_modelResources = m_model->LoadTextures(device, resourceUpload);

    m_fxFactory = std::make_unique<EffectFactory>(m_modelResources->Heap(), m_states->Heap());

    auto uploadResourcesFinished = resourceUpload.End(
        m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    EffectPipelineStateDescription pd(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        rtState);

    m_modelNormal = m_model->CreateEffects(*m_fxFactory, pd, pd);

    m_world = Matrix::Identity;

    const size_t nbones = m_model->bones.size();

    m_drawBones = ModelBone::MakeArray(nbones);
    m_animBones = ModelBone::MakeArray(nbones);

    m_model->CopyBoneTransformsTo(nbones, m_animBones.get());

    uint32_t index = 0;
    for (const auto& it : m_model->bones)
    {
        if (_wcsicmp(it.name.c_str(), L"tank_geo") == 0)
        {
            // Need to recenter the model.
            m_animBones[index] = XMMatrixIdentity();
        }
        else if (_wcsicmp(it.name.c_str(), L"l_back_wheel_geo") == 0)
        {
            m_leftBackWheelBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"r_back_wheel_geo") == 0)
        {
            m_rightBackWheelBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"l_front_wheel_geo") == 0)
        {
            m_leftFrontWheelBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"r_front_wheel_geo") == 0)
        {
            m_rightFrontWheelBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"l_steer_geo") == 0)
        {
            m_leftSteerBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"r_steer_geo") == 0)
        {
            m_rightSteerBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"turret_geo") == 0)
        {
            m_turretBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"canon_geo") == 0)
        {
            m_cannonBone = index;
        }
        else if (_wcsicmp(it.name.c_str(), L"hatch_geo") == 0)
        {
            m_hatchBone = index;
        }

        ++index;
    }
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
    m_modelResources.reset();
    m_model.reset();
    m_modelNormal.clear();
    m_drawBones.reset();
    m_animBones.reset();

    m_graphicsMemory.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
