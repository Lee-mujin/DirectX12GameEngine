#include "pch.h"

void RotatorComponent::Update(float deltaTime)
{
    mAngle += mSpeed * deltaTime;
    GetOwner()->GetTransform().SetRotation(Vector3(0.0f, mAngle, 0.0f));
}