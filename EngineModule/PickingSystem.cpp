#include "pch.h"
#include "PickingSystem.h"
#include "GameObject.h"
#include "Scene.h"
#include <limits>

bool PickingSystem::Raycast(Scene* scene, const Ray& ray, RayCastHit& outHit)
{
    if (!scene) return false;
    return Raycast(scene->GetGameObjects(), ray, outHit);
}

bool PickingSystem::Raycast(const std::vector<std::shared_ptr<GameObject>>& objects, const Ray& ray, RayCastHit& outHit)
{
    bool isHit = false;

    //가장 가까운 충돌 지점 추적
    float closestHitDistance = (std::numeric_limits<float>::max)();
    GameObject* closestObject = nullptr;

    for (const auto& gameObject : objects)
    {
        if (!gameObject) continue;

        //카메라 객체 자체는 피킹 대상에서 제외
        if (gameObject->GetName() == "Camera") continue;

        BoundingBox worldBox = gameObject->GetWorldBoundingBox();

        float hitDistance = 0.0f;
        if (worldBox.Intersects(ray, hitDistance))
        {
            //Ray Origin(카메라)에서 더 가까운 충돌점을 가진 오브젝트 우선 선택
            if (hitDistance >= 0.0f && hitDistance < closestHitDistance)
            {
                closestHitDistance = hitDistance;
                closestObject = gameObject.get();
                isHit = true;
            }
        }
    }

    if (isHit && closestObject)
    {
        outHit.Object = closestObject->shared_from_this(); //std::shared_ptr 전달
        outHit.Distance = closestHitDistance;
        outHit.Point = ray.GetPoint(closestHitDistance);
        outHit.Normal = Vector3::Up;
    }

    return isHit;
}