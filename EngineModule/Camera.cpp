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

    if (input->IsMouseDown(1)) //우클릭 누르고 있는 동안만 시점 회전
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

//뷰행렬
Matrix4x4 Camera::GetView() const
{
    Vector3 forward = GetForward();
    return Matrix4x4::LookAt(mPosition, mPosition + forward, Vector3::Up);
}

//원근투영
Matrix4x4 Camera::GetProjection() const
{
    return Matrix4x4::Perspective(mFovY, mAspect, mNearZ, mFarZ);
}

Ray Camera::ScreenPointToRay(const Vector2& screenPos, float viewportWidth, float viewportHeight) const
{
    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) return Ray();

    //Screen Space (0~Width, 0~Height) -> NDC Space (-1~1, -1~1) 변환 (Y축 반전)
    float ndcX = (2.0f * screenPos.X / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.Y / viewportHeight);

    //View Space 상의 Ray Direction 계산
    Matrix4x4 invProj = GetProjection().GetInverse();

    //Near Clip Plane 상의 점과 Far Clip Plane 상의 점을 View Space로 변환
    Vector3 rayStartView(
        ndcX * invProj._11,
        ndcY * invProj._22,
        1.0f
    );

    //View Space -> World Space 변환 (View Matrix Inverse 적용)
    Matrix4x4 invView = GetView().GetInverse();

    //Direction을 World Space로 회전/변환
    Vector3 rayDirWorld(
        rayStartView.X * invView._11 + rayStartView.Y * invView._21 + rayStartView.Z * invView._31,
        rayStartView.X * invView._12 + rayStartView.Y * invView._22 + rayStartView.Z * invView._32,
        rayStartView.X * invView._13 + rayStartView.Y * invView._23 + rayStartView.Z * invView._33
    );

    return Ray(mPosition, rayDirWorld.GetNormalize());
}