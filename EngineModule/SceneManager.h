#pragma once
#include <memory>

class Scene;
class Input;

class SceneManager
{
public:
    bool Initialize();
    void Update(Input* input, float deltaTime);

    void ChangeScene(std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> GetCurrentScene() const { return mCurrentScene; }

private:
    std::shared_ptr<Scene> mCurrentScene;
};