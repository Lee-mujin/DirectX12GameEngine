#include "pch.h"
#include "Application.h"
#include "Scene.h"
#include "PointLight.h"
#include "SpotLight.h"

bool Application::Initialize(HINSTANCE hInstance)
{
    if (!mWindow.Create(hInstance, 1280, 720)) return false;
    mWindow.SetInput(&mInput);

    if (!mRenderer.Initialize(mWindow.GetHWND(), mWindow.GetWidth(), mWindow.GetHeight())) return false;

    mResourceManager.Initialize(&mRenderer);
    mSceneManager.Initialize();

    float aspect = static_cast<float>(mWindow.GetWidth()) / static_cast<float>(mWindow.GetHeight());

    auto scene = std::make_shared<Scene>();

    auto cameraObject = std::make_shared<GameObject>();
    CameraComponent* cameraComponent = cameraObject->AddComponent<CameraComponent>();
    cameraComponent->Initialize(aspect);
    scene->AddGameObject(cameraObject);
    scene->SetMainCamera(cameraComponent);

    auto cube = std::make_shared<GameObject>();
    MeshRenderer* meshRenderer = cube->AddComponent<MeshRenderer>();
    meshRenderer->SetMesh(mResourceManager.GetCubeMesh());

    auto material = std::make_shared<Material>();
    material->SetTexture(mResourceManager.LoadTexture(L"Textures/Box.jpg"));
    meshRenderer->SetMaterial(material);

    cube->AddComponent<RotatorComponent>(90.0f);
    scene->AddGameObject(cube);
    scene->SetMainLight(DirectionalLight{ Vector3(0.3f, -0.8f, 0.5f), Vector3(1.0f, 1.0f, 1.0f), 1.0f });

    PointLight pointLight;
    pointLight.Position = Vector3(0.0f, 1.0f, -2.0f);
    pointLight.Color = Vector3(1.0f, 0.6f, 0.3f); // 따뜻한 색
    pointLight.Intensity = 2.0f;
    pointLight.Range = 8.0f;
    pointLight.Enabled = true;
    scene->SetPointLight(pointLight);

    SpotLight spotLight;
    spotLight.Position = Vector3(0.0f, 3.0f, -3.0f);
    spotLight.Direction = Vector3(0.0f, -1.0f, 0.5f).GetNormalize();
    spotLight.Color = Vector3(0.4f, 0.6f, 1.0f); // 차가운 색으로 Point Light와 구분
    spotLight.Intensity = 3.0f;
    spotLight.Range = 10.0f;
    spotLight.InnerConeAngle = 12.0f;
    spotLight.OuterConeAngle = 20.0f;
    spotLight.Enabled = true;
    scene->SetSpotLight(spotLight);

    mSceneManager.ChangeScene(scene);
    
    return true;
}
void Application::Run()
{
    while (mWindow.ProcessMessage())
    {
        mTimer.Tick();
        float deltaTime = mTimer.DeltaTime();

        mSceneManager.Update(&mInput, deltaTime);
        mSceneManager.Render(mRenderer);

        mInput.Update();
    }
}

void Application::Shutdown()
{
    mRenderer.Cleanup();
    mWindow.Destroy();
}