#pragma once
#include <vector>
#include <memory>
#include "RenderQueue.h"

class GameObject;

//씬 내 메시렌더러 컴포넌트만 전담으로 RenderItem을 수집하는 시스템
class RenderSystem
{
public:
    void BuildRenderQueue(const std::vector<std::shared_ptr<GameObject>>& gameObjects, RenderQueue& outQueue);
};