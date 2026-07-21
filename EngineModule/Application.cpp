#include "pch.h"
#include "Application.h"
#include "Scene.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "imgui.h"
#include "SkinnedMesh.h"
#include "Model.h"
#include "SceneSerializer.h"

bool Application::Initialize(HINSTANCE hInstance)
{
    if (!mWindow.Create(hInstance, 1280, 720)) return false;
    mWindow.SetInput(&mInput);

    if (!mRenderer.Initialize(mWindow.GetHWND(), mWindow.GetWidth(), mWindow.GetHeight())) return false;

    //렌더러 CommandQueue 주입 제거 Device만 전달하여 내부 전용 CopyQueue 생성
    if (!mUploadContext.Initialize(mRenderer.GetDevice())) return false;

    mTextureLoader.Initialize(mRenderer.GetDevice(), mRenderer.GetSrvAllocatorPtr(), &mUploadContext);

    //DescriptorHandle 단일 객체 반환 받기
    DescriptorHandle fontHandle = mRenderer.AllocateSrvSlot();

    mImGuiLayer.Initialize(
        mWindow.GetHWND(), mRenderer.GetDevice(), mRenderer.GetCommandQueue(),
        D3D12Renderer::GetFrameCount(), DXGI_FORMAT_R8G8B8A8_UNORM,
        mRenderer.GetSrvAllocator().GetHeap(), fontHandle.CpuHandle, fontHandle.GpuHandle);

    mRenderer.SetImGuiLayer(&mImGuiLayer);

    //ResourceManager에 세개 전부 전달
    mResourceManager.Initialize(&mRenderer, &mTextureLoader, &mUploadContext);
    mSceneManager.Initialize();

    float aspect = static_cast<float>(mWindow.GetWidth()) / static_cast<float>(mWindow.GetHeight());
    auto scene = std::make_shared<Scene>();

    auto cameraObject = std::make_shared<GameObject>();
    cameraObject->SetName("Camera");
    CameraComponent* cameraComponent = cameraObject->AddComponent<CameraComponent>();
    cameraComponent->Initialize(aspect);
    scene->AddGameObject(cameraObject);
    scene->SetMainCamera(cameraComponent);

    auto cube = std::make_shared<GameObject>();
    cube->SetName("Cube");
    MeshRenderer* meshRenderer = cube->AddComponent<MeshRenderer>();
    meshRenderer->SetMesh(mResourceManager.GetCubeMesh());

    auto material = std::make_shared<Material>();
    //mTextureLoader 직접 호출 제거 -> mResourceManager를 경유
    material->SetTexture(mResourceManager.LoadTexture(L"Assets/Textures/Box.jpg"));
    meshRenderer->SetMaterial(material);

    //CPU를 멈추는 Flush 대신, Copy Queue 작업 완결 신호를 Graphics Queue에 연결해 GPU 간 동기화
    mUploadContext.SyncWithGraphicsQueue(mRenderer.GetCommandQueue());

    cube->AddComponent<RotatorComponent>(90.0f);
    scene->AddGameObject(cube);

    mContentBrowser.Initialize("Assets", [this](const std::filesystem::path& path)
        {
            AssetHandle handle = mResourceManager.GetOrCreateHandle(path.string());
            SpawnAssetIntoScene(handle);
        });

    mHierarchy.SetOnAssetDropped([this](const std::filesystem::path& path)
        {
            AssetHandle handle = mResourceManager.GetOrCreateHandle(path.string());
            SpawnAssetIntoScene(handle);
        });

    scene->SetMainLight(DirectionalLight{ Vector3(0.3f, -0.8f, 0.5f), Vector3(1.0f, 1.0f, 1.0f), 1.0f });

    PointLight pointLight;
    pointLight.Position = Vector3(0.0f, 1.0f, -2.0f);
    pointLight.Color = Vector3(1.0f, 0.6f, 0.3f);
    pointLight.Intensity = 2.0f;
    pointLight.Range = 8.0f;
    pointLight.Enabled = true;
    scene->SetPointLight(pointLight);

    SpotLight spotLight;
    spotLight.Position = Vector3(0.0f, 3.0f, -3.0f);
    spotLight.Direction = Vector3(0.0f, -1.0f, 0.5f).GetNormalize();
    spotLight.Color = Vector3(0.4f, 0.6f, 1.0f);
    spotLight.Intensity = 3.0f;
    spotLight.Range = 10.0f;
    spotLight.InnerConeAngle = 12.0f;
    spotLight.OuterConeAngle = 20.0f;
    spotLight.Enabled = true;
    scene->SetSpotLight(spotLight);

    mSceneManager.ChangeScene(scene);

    return true;
}

std::shared_ptr<GameObject> Application::SpawnAssetIntoScene(AssetHandle handle)
{
    auto scene = mSceneManager.GetCurrentScene();
    if (!scene || !handle.IsValid()) return nullptr;

    auto model = mResourceManager.GetOrLoadModel(handle);
    if (!model) return nullptr;

    const std::string& path = mResourceManager.GetPath(handle);
    std::filesystem::path fsPath(path);

    auto obj = std::make_shared<GameObject>();
    obj->SetName(fsPath.stem().string());
    obj->SetSourceAssetHandle(handle);

    MeshRenderer* renderer = obj->AddComponent<MeshRenderer>();
    renderer->SetMesh(model->GetMesh());

    auto material = std::make_shared<Material>();
    material->SetTexture(mResourceManager.GetDefaultWhiteTexture());
    renderer->SetMaterial(material);

    if (model->IsSkinned())
    {
        AnimatorComponent* animator = obj->AddComponent<AnimatorComponent>();
        animator->SetSkeleton(model->GetSkeleton());
        animator->SetAnimationList(model->GetAnimations());
        animator->Play();

        obj->GetTransform().SetScale(Vector3(0.01f, 0.01f, 0.01f));
        obj->GetTransform().SetRotation(Vector3(90.0f, 0.0f, 0.0f));
    }

    scene->AddGameObject(obj);
    return obj;
}

void Application::Run()
{
    while (mWindow.ProcessMessage())
    {
        mTimer.Tick();
        float deltaTime = mTimer.DeltaTime();

        mImGuiLayer.NewFrame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save Scene"))
                {
                    if (auto scene = mSceneManager.GetCurrentScene())
                    {
                        SceneSerializer::Save(*scene, mResourceManager, "Assets/Scenes/scene.txt");
                    }
                }
                if (ImGui::MenuItem("Load Scene"))
                {
                    if (auto scene = mSceneManager.GetCurrentScene())
                    {
                        SceneSerializer::Load(*scene, *this, mResourceManager, "Assets/Scenes/scene.txt");
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        auto scene = mSceneManager.GetCurrentScene();
        if (scene)
        {
            mHierarchy.Draw(*scene, mEditorState);
            mLight.Draw(*scene);

            if (CameraComponent* camComp = scene->GetMainCamera())
            {
                mGizmoPanel.Draw(mEditorState, camComp->GetCamera());
            }
        }
        mInspector.Draw(mEditorState, mResourceManager);
        mContentBrowser.Draw();

        float newWidth, newHeight, posX, posY;
        mViewport.Draw(mRenderer.GetViewportSrvHandle(), newWidth, newHeight, posX, posY);

        if (newWidth > 0 && newHeight > 0 &&
            (static_cast<UINT>(newWidth) != static_cast<UINT>(mViewportWidth) ||
                static_cast<UINT>(newHeight) != static_cast<UINT>(mViewportHeight)))
        {
            mViewportWidth = newWidth;
            mViewportHeight = newHeight;
            mRenderer.ResizeViewport(static_cast<UINT>(newWidth), static_cast<UINT>(newHeight));
        }
        mViewportPosX = posX;
        mViewportPosY = posY;

        mSceneManager.Update(&mInput, deltaTime);

        if (scene)
        {
            if (CameraComponent* camComp = scene->GetMainCamera())
            {
                mRenderer.BeginFrame(camComp->GetCamera(), scene->GetMainLight(), scene->GetPointLight(), scene->GetSpotLight());

                for (auto& obj : scene->GetGameObjects())
                {
                    if (MeshRenderer* mr = obj->GetComponent<MeshRenderer>())
                    {
                        mr->Render(mRenderer);
                    }
                }

                mRenderer.TransitionToSwapChain();
            }
        }

        mRenderer.EndFrame();
        mInput.Update();
    }
}

void Application::Shutdown()
{
    mImGuiLayer.Shutdown();
    mUploadContext.Shutdown();
    mRenderer.Cleanup();
    mWindow.Destroy();
}