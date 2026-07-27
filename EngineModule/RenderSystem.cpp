#include "pch.h"
#include "RenderSystem.h"
#include "GameObject.h"
#include "MeshRenderer.h"

void RenderSystem::BuildRenderQueue(const std::vector<std::shared_ptr<GameObject>>& gameObjects, RenderQueue& outQueue)
{
    outQueue.Clear();

    for (const auto& obj : gameObjects)
    {
        if (!obj) continue;

        if (MeshRenderer* meshRenderer = obj->GetComponent<MeshRenderer>())
        {
            meshRenderer->SubmitRenderItem(outQueue);
        }
    }
}