#pragma once
#include "Vector3.h"

class GameObject;

//Picking 및 Collision 연산 결과 데이터를 담는 공용 구조체
struct RayCastHit
{
    GameObject* Object = nullptr;
    float Distance = 0.0f;     //Ray Origin으로부터의 충돌 거리
    Vector3 Point = Vector3::Zero;   //충돌 지점 (World Space)
    Vector3 Normal = Vector3::Up;    //충돌 지점의 법선 벡터 (World Space)
};