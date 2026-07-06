#pragma once
#include <memory>

class Scene;
class D3D12Renderer;
class Input;

class SceneManager
{
public:
    bool Initialize();
    void Update(Input* input, float deltaTime);
    void Render(D3D12Renderer& renderer);

    void ChangeScene(std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> GetCurrentScene() const { return mCurrentScene; }

private:
    std::shared_ptr<Scene> mCurrentScene;
};