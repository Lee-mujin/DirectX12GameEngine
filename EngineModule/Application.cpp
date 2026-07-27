#include "pch.h"
#include "Application.h"
#include "Scene.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "imgui.h"
#include "SkinnedMesh.h"
#include "Model.h"
#include "SceneSerializer.h"
#include "ModelUtility.h"
#include "ShadowPass.h"
#include "GeometryPass.h"
#include "UIPass.h"

bool Application::Initialize(HINSTANCE hInstance)
{
    if (!mWindow.Create(hInstance, 1280, 720)) return false;
    mWindow.SetInput(&mInput);

    if (!InitializeRenderer()) return false;
    if (!InitializeUI()) return false;

    InitializeResources();
    CreateDefaultScene();
    InitializeEditor();

    // 렌더 파이프라인 패스 구성 (ShadowPass -> GeometryPass -> UIPass)
    SetupRenderPipeline();

    mSceneManager.Initialize();

    return true;
}

// RenderPipeline에 파이프라인 패스 등록
void Application::SetupRenderPipeline()
{
    mRenderPipeline.ClearPasses();
    mRenderPipeline.AddPass<ShadowPass>();
    mRenderPipeline.AddPass<GeometryPass>();
    mRenderPipeline.AddPass<UIPass>();
}

// 1. D3D12 렌더러 및 GPU 업로드 컨텍스트 초기화
bool Application::InitializeRenderer()
{
    if (!mRenderer.Initialize(mWindow.GetHWND(), mWindow.GetWidth(), mWindow.GetHeight())) return false;

    // 렌더러 CommandQueue 주입 제거, Device만 전달하여 내부 전용 CopyQueue 생성
    if (!mUploadContext.Initialize(mRenderer.GetDevice())) return false;

    return true;
}

// 2. ImGui UI 레이어 세팅
bool Application::InitializeUI()
{
    // DescriptorHandle 단일 객체 반환 받기
    DescriptorHandle fontHandle = mRenderer.AllocateSrvSlot();

    mImGuiLayer.Initialize(
        mWindow.GetHWND(), mRenderer.GetDevice(), mRenderer.GetCommandQueue(),
        D3D12Renderer::GetFrameCount(), DXGI_FORMAT_R8G8B8A8_UNORM,
        mRenderer.GetSrvAllocator().GetHeap(), fontHandle.CpuHandle, fontHandle.GpuHandle);

    mRenderer.SetImGuiLayer(&mImGuiLayer);
    return true;
}

// 3. TextureLoader, ResourceManager, 기본 화이트 텍스처 초기화
void Application::InitializeResources()
{
    mTextureLoader.Initialize(mRenderer.GetDevice(), mRenderer.GetSrvAllocatorPtr(), &mUploadContext);

    // ResourceManager에 세 개 모두 전달
    mResourceManager.Initialize(&mRenderer, &mTextureLoader, &mUploadContext);

    // 렌더러에 기본 흰색 텍스처 주입
    mRenderer.SetDefaultWhiteTexture(mResourceManager.GetDefaultWhiteTexture());
}

// 4. 초기 기본 카메라, 큐브 객체, 라이트 세팅
void Application::CreateDefaultScene()
{
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
    // AssetHandle과 함께 텍스처 바인딩
    AssetHandle boxTexHandle = mResourceManager.GetOrCreateHandle("Assets/Textures/Box.jpg");
    material->SetBaseColorTexture(mResourceManager.GetOrLoadTexture(boxTexHandle), boxTexHandle);
    meshRenderer->SetMaterial(material);

    // CPU를 멈추는 Flush 대신, Copy Queue 작업 완결 신호를 Graphics Queue에 연결해 GPU 간 동기화
    mUploadContext.SyncWithGraphicsQueue(mRenderer.GetCommandQueue());

    cube->AddComponent<RotatorComponent>(90.0f);
    scene->AddGameObject(cube);

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
    mRenderer.WaitForGpu();
}

// 5. ContentBrowser 및 Hierarchy 콜백 초기화
void Application::InitializeEditor()
{
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

    // ModelUtility를 사용해 Mesh, Material 초기화, SourceHandle, Skinned/Static 구분을 한 번에 처리
    ModelUtility::ReplaceModel(*obj, model, handle);

    // 스키니드 모델 기본 스케일/회전 보정
    if (model->IsSkinned())
    {
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
        }
        mInspector.Draw(mEditorState, mResourceManager);
        mContentBrowser.Draw();

        float newWidth = 0.0f, newHeight = 0.0f, posX = 0.0f, posY = 0.0f;
        mViewport.Draw(mEditorState, mRenderer.GetViewportSrvHandle(), newWidth, newHeight, posX, posY);

        // 뷰포트 변경 체크 시 float <-> UINT 명시적 형변환으로 C4244 / C4267 경고 완벽 보정
        UINT uNewWidth = static_cast<UINT>(newWidth);
        UINT uNewHeight = static_cast<UINT>(newHeight);

        float fCurrentWidth = static_cast<float>(mViewportWidth);
        float fCurrentHeight = static_cast<float>(mViewportHeight);

        if (uNewWidth > 0 && uNewHeight > 0 &&
            (newWidth != fCurrentWidth || newHeight != fCurrentHeight))
        {
            mViewportWidth = static_cast<float>(uNewWidth);
            mViewportHeight = static_cast<float>(uNewHeight);
            mRenderer.ResizeViewport(uNewWidth, uNewHeight);
        }
        mViewportPosX = posX;
        mViewportPosY = posY;

        // Gizmo
        if (scene)
        {
            if (CameraComponent* camComp = scene->GetMainCamera())
            {
                mGizmoPanel.Draw(mEditorState, camComp->GetCamera(), mViewportPosX, mViewportPosY, mViewportWidth, mViewportHeight);
            }
        }

        mSceneManager.Update(&mInput, deltaTime);

        // RenderPipeline 실행부
        if (scene && scene->GetMainCamera())
        {
            // RenderQueue 수집
            RenderQueue queue;
            scene->BuildRenderQueue(queue);

            // 패스 실행용 Context 구성
            RenderContext context;
            context.Renderer = &mRenderer;
            context.Queue = &queue;
            context.MainCamera = &scene->GetMainCamera()->GetCamera();
            context.DirLight = &scene->GetMainLight();
            context.PointLight = &scene->GetPointLight();
            context.SpotLight = &scene->GetSpotLight();

            // RenderPipeline 중앙 제어를 통한 일괄 실행
            mRenderPipeline.Execute(context);
        }
        else
        {
            // Scene이 없을 때 UI만 렌더링 및 프레임 종료
            RenderContext context;
            context.Renderer = &mRenderer;
            UIPass uiPass;
            uiPass.Execute(context);
        }

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