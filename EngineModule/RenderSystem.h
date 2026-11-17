#pragma once
#include <vector>
#include <memory>
#include "RenderQueue.h"

class GameObject;
class Camera;

//씬 내 메시렌더러 컴포넌트만 전담으로 RenderItem을 수집하는 시스템
class RenderSystem
{
public:
    // camera가 주어지면 월드 AABB와 프러스텀을 검사해 컬링한다.
    // outCulledCount가 주어지면 컬링된 오브젝트 수를 기록한다(Stats 표시용).
    void BuildRenderQueue(const std::vector<std::shared_ptr<GameObject>>& gameObjects,
        RenderQueue& outQueue, const Camera* camera = nullptr, UINT* outCulledCount = nullptr);
};