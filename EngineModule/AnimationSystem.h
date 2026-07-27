#pragma once
#include <vector>
#include <memory>

class GameObject;

//씬 내 애니메이터 컴포넌트만 전담으로 일괄 처리하는 시스템
class AnimationSystem
{
public:
    void Update(const std::vector<std::shared_ptr<GameObject>>& gameObjects, float deltaTime);
};