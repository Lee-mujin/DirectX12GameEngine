#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

class Input;

class Camera
{
public:
    void Initialize(float aspect);
    void Update(Input* input, float deltaTime);

    Matrix4x4 GetView() const;
    Matrix4x4 GetProjection() const;
    Vector3 GetPosition() const { return mPosition; }

private:
    Vector3 GetForward() const;
    Vector3 GetRight() const;

private:
    Vector3 mPosition{ 0.0f, 1.5f, -3.0f };
    float mYaw = 0.0f; 
    float mPitch = 0.0f; 

    float mFovY = 0.785398f; //45도
    float mAspect = 1.0f;
    float mNearZ = 0.1f;
    float mFarZ = 100.0f;

    float mMoveSpeed = 3.0f;
    float mLookSpeed = 0.005f;
};