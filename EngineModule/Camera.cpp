#include "pch.h"
#include "Input.h"
#include "MathUtil.h"

void Camera::Initialize(float aspect)
{
    mAspect = aspect;
}

Vector3 Camera::GetForward() const
{
    float cosPitch = std::cosf(mPitch);
    return Vector3(
        cosPitch * std::sinf(mYaw),
        std::sinf(mPitch),
        cosPitch * std::cosf(mYaw));
}

Vector3 Camera::GetRight() const
{
    return Vector3::Cross(Vector3::Up, GetForward()).GetNormalize();
}

void Camera::Update(Input* input, float deltaTime)
{
    if (!input)
    {
        return;
    }

    if (input->IsMouseDown(1)) // 우클릭 누르고 있는 동안만 시점 회전
    {
        mYaw += input->GetMouseDeltaX() * mLookSpeed;
        mPitch -= input->GetMouseDeltaY() * mLookSpeed;

        const float pitchLimit = Math::HalfPI - 0.01f;
        mPitch = Math::Clamp(mPitch, -pitchLimit, pitchLimit);
    }

    Vector3 forward = GetForward();
    Vector3 right = GetRight();
    float moveDist = mMoveSpeed * deltaTime;

    if (input->IsKeyDown('W')) mPosition += forward * moveDist;
    if (input->IsKeyDown('S')) mPosition -= forward * moveDist;
    if (input->IsKeyDown('D')) mPosition += right * moveDist;
    if (input->IsKeyDown('A')) mPosition -= right * moveDist;
    if (input->IsKeyDown('E')) mPosition += Vector3::Up * moveDist;
    if (input->IsKeyDown('Q')) mPosition -= Vector3::Up * moveDist;
}

Matrix4x4 Camera::GetView() const
{
    Vector3 forward = GetForward();
    return Matrix4x4::LookAt(mPosition, mPosition + forward, Vector3::Up);
}

Matrix4x4 Camera::GetProjection() const
{
    return Matrix4x4::Perspective(mFovY, mAspect, mNearZ, mFarZ);
}