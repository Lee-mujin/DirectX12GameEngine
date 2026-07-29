#pragma once
#include "Ray.h"
#include "RayCastHit.h"
#include <vector>
#include <memory>

class GameObject;
class Scene;

class PickingSystem
{
public:
    //Scene 내의 모든 GameObject를 대상으로 Raycast 연산 수행
    static bool Raycast(Scene* scene, const Ray& ray, RayCastHit& outHit);

    //전달받은 GameObject 목록을 대상으로 Raycast 연산 수행
    static bool Raycast(const std::vector<std::shared_ptr<GameObject>>& objects, const Ray& ray, RayCastHit& outHit);
};