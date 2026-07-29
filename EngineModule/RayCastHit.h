#pragma once
#include "Vector3.h"
#include <memory>

class GameObject;

struct RayCastHit
{
    std::shared_ptr<GameObject> Object = nullptr; //shared_ptr 사용
    float Distance = 0.0f; //Ray Origin으로부터의 거리
    Vector3 Point = Vector3::Zero; //World Space 충돌 위치
    Vector3 Normal = Vector3::Up; //법선 벡터
};