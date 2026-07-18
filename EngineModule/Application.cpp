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
    if (!mWindow.Create(hInstance, 1280, 720)) return false; //창
    mWindow.SetInput(&mInput); //키/마우스 입력

    if (!mRenderer.Initialize(mWindow.GetHWND(), mWindow.GetWidth(), mWindow.GetHeight())) return false;

    D3D12_CPU_DESCRIPTOR_HANDLE fontCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE fontGpuHandle;
    mRenderer.AllocateSrvSlot(fontCpuHandle, fontGpuHandle); //descriptor heap 슬롯 할당

    mImGuiLayer.Initialize(
        mWindow.GetHWND(), mRenderer.GetDevice(), mRenderer.GetCommandQueue(),
        D3D12Renderer::GetFrameCount(), DXGI_FORMAT_R8G8B8A8_UNORM,
        mRenderer.GetSrvHeap(), fontCpuHandle, fontGpuHandle);

    //mRenderer로부터 핸들과 디바이스 정보를 넘겨 ImGui를 초기화해 DX12에서 UI를 그림
    mRenderer.SetImGuiLayer(&mImGuiLayer);

    mResourceManager.Initialize(&mRenderer); //GUI
    mSceneManager.Initialize();
    ////Scene 생성
    float aspect = static_cast<float>(mWindow.GetWidth()) / static_cast<float>(mWindow.GetHeight());

    auto scene = std::make_shared<Scene>();

	//GameObject에 CameraComponent를 추가해 카메라를 생성하고 Scene에 추가
    auto cameraObject = std::make_shared<GameObject>();
    cameraObject->SetName("Camera");
    CameraComponent* cameraComponent = cameraObject->AddComponent<CameraComponent>();
    cameraComponent->Initialize(aspect);
    scene->AddGameObject(cameraObject);
    scene->SetMainCamera(cameraComponent);

    //MeshRenderer 컴포넌트를 붙여서 큐브를 생성
    auto cube = std::make_shared<GameObject>();
    cube->SetName("Cube");
    MeshRenderer* meshRenderer = cube->AddComponent<MeshRenderer>();
    meshRenderer->SetMesh(mResourceManager.GetCubeMesh());
    //머테리얼로 텍스처를 입힘
    auto material = std::make_shared<Material>();
    material->SetTexture(mResourceManager.LoadTexture(L"Assets/Textures/Box.jpg"));
    meshRenderer->SetMaterial(material);
    //회전추가
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

    //DirectionalLight
    scene->SetMainLight(DirectionalLight{ Vector3(0.3f, -0.8f, 0.5f), Vector3(1.0f, 1.0f, 1.0f), 1.0f });

    //PointLight
    PointLight pointLight;
    pointLight.Position = Vector3(0.0f, 1.0f, -2.0f);
    pointLight.Color = Vector3(1.0f, 0.6f, 0.3f);
    pointLight.Intensity = 2.0f;
    pointLight.Range = 8.0f;
    pointLight.Enabled = true;
    scene->SetPointLight(pointLight);

    //SpotLight
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
    if (!scene || !handle.IsValid())
    {
        return nullptr;
    }

    auto model = mResourceManager.GetOrLoadModel(handle);
    if (!model)
    {
        return nullptr;
    }

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
        mTimer.Tick(); //매 프레임마다 시간 갱신
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
        mInspector.Draw(mEditorState);
        mContentBrowser.Draw();

        mSceneManager.Update(&mInput, deltaTime); //로직 업뎃
        mSceneManager.Render(mRenderer); //렌더러를 이용해 씬

        mInput.Update();
    }
}

void Application::Shutdown()
{
    mImGuiLayer.Shutdown();
    mRenderer.Cleanup();
    mWindow.Destroy();
}