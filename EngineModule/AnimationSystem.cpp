#include "pch.h"
#include "AnimationSystem.h"
#include "GameObject.h"
#include "AnimatorComponent.h"

void AnimationSystem::Update(const std::vector<std::shared_ptr<GameObject>>& gameObjects, float deltaTime)
{
    for (const auto& obj : gameObjects)
    {
        if (!obj) continue;

        if (AnimatorComponent* animator = obj->GetComponent<AnimatorComponent>())
        {
            animator->Update(deltaTime);
        }
    }
}