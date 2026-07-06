#include "pch.h"

void CameraComponent::Initialize(float aspect)
{
    mCamera.Initialize(aspect);
}

//카메라만 Scene이 직접 불러줌
void CameraComponent::UpdateCamera(Input* input, float deltaTime)
{
    mCamera.Update(input, deltaTime);
}