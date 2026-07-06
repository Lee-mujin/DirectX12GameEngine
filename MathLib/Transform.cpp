#include "pch.h"
#include "MathUtil.h"
#include "Matrix4x4.h"
#include "Transform.h"

Transform::Transform() : mPosition(Vector3::Zero), mEulerAngles(Vector3::Zero), mScale(Vector3::One) {}
Transform::Transform(const Vector3& position) : mPosition(position), mEulerAngles(Vector3::Zero), mScale(Vector3::One) {}
Transform::Transform(const Vector3& position, const Quaternion& rotation) : mPosition(position), mEulerAngles(rotation.ToEuler()), mScale(Vector3::One) {}
Transform::Transform(const Vector3& position, const Vector3& euler) : mPosition(position), mEulerAngles(euler), mScale(Vector3::One) {}
Transform::Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) : mPosition(position), mEulerAngles(rotation.ToEuler()), mScale(scale) {}
Transform::Transform(const Vector3& position, const Vector3& euler, const Vector3& scale) : mPosition(position), mEulerAngles(euler), mScale(scale) {}

Vector3 Transform::GetPosition() const { return mPosition; }
Quaternion Transform::GetRotation() const { return mEulerAngles.ToQuaternion(); }
Vector3 Transform::GetEulerAngles() const { return mEulerAngles; }
Vector3 Transform::GetScale() const { return mScale; }

Vector3 Transform::GetAxisX() const
{
    Quaternion rotation = mEulerAngles.ToQuaternion();
    rotation.Normalize();
    return rotation * Vector3::Right;
}

Vector3 Transform::GetAxisY() const
{
    Quaternion rotation = mEulerAngles.ToQuaternion();
    rotation.Normalize();
    return rotation * Vector3::Up;
}

Vector3 Transform::GetAxisZ() const
{
    Quaternion rotation = mEulerAngles.ToQuaternion();
    rotation.Normalize();
    return rotation * Vector3::Forward;
}

Matrix4x4 Transform::GetMatrix() const
{
    Matrix4x4 matPos = Matrix4x4::Translate(mPosition);
    Vector3 radianEuler = mEulerAngles * (Math::PI / 180.f);
    Matrix4x4 matRot = Matrix4x4::Rotate(radianEuler);
    Matrix4x4 matScale = Matrix4x4::Scale(mScale);

    return matScale * matRot * matPos;
}

void Transform::SetPosition(const Vector3& position) { mPosition = position; }
void Transform::SetRotation(const Quaternion& rotation) { mEulerAngles = rotation.ToEuler(); }
void Transform::SetRotation(const Vector3& euler) { mEulerAngles = euler; }
void Transform::SetScale(const Vector3& scale) { mScale = scale; }

void Transform::AddPosition(const Vector3& position) { mPosition += position; }
void Transform::AddRotation(const Quaternion& rotation) { mEulerAngles += rotation.ToEuler(); }
void Transform::AddRotation(const Vector3& euler) { mEulerAngles += euler; }
void Transform::AddScale(const Vector3& scale) { mScale += scale; }
void Transform::AddRotationX(const float degree) { mEulerAngles.X += degree; }
void Transform::AddRotationY(const float degree) { mEulerAngles.Y += degree; }
void Transform::AddRotationZ(const float degree) { mEulerAngles.Z += degree; }

void Transform::Translate(const Vector3& translation)
{
    Vector3 posX = GetAxisX() * translation.X;
    Vector3 posY = GetAxisY() * translation.Y;
    Vector3 posZ = GetAxisZ() * translation.Z;

    mPosition += posX + posY + posZ;
}

void Transform::LookAt(const Transform& target)
{
    Matrix4x4 lookMat = Matrix4x4::LookAt(mPosition, target.mPosition, Vector3::Up);
    Quaternion q = Matrix4x4::Rotate(Quaternion::Identity) == lookMat ? Quaternion::Identity : Quaternion(); // 아래 참고
    // 회전 추출은 축별 각도로 직접 계산
    Vector3 forward = (target.mPosition - mPosition).GetNormalize();
    float yaw = Math::Rad2Deg(std::atan2f(forward.X, forward.Z));
    float pitch = Math::Rad2Deg(std::asinf(-forward.Y));
    mEulerAngles = Vector3(pitch, yaw, 0.f);
}

Transform Transform::GetInverse() const
{
    Vector3 reciprocalScale = Vector3::Zero;
    if (!Math::EqualsInTolerance(mScale.X, 0.f)) reciprocalScale.X = 1.f / mScale.X;
    if (!Math::EqualsInTolerance(mScale.Y, 0.f)) reciprocalScale.Y = 1.f / mScale.Y;
    if (!Math::EqualsInTolerance(mScale.Z, 0.f)) reciprocalScale.Z = 1.f / mScale.Z;

    Transform result;
    result.SetScale(reciprocalScale);
    result.SetRotation(GetRotation().GetInverse());
    result.SetPosition(result.GetScale() * (result.GetRotation() * -mPosition));
    return result;
}

Transform Transform::GetLocalToWorld(const Transform& parentWorldTransform) const
{
    Transform result;
    result.SetScale(parentWorldTransform.GetScale() * GetScale());
    result.SetRotation(parentWorldTransform.GetRotation() * GetRotation());
    result.SetPosition(parentWorldTransform.GetPosition() + parentWorldTransform.GetRotation() * (parentWorldTransform.GetScale() * GetPosition()));
    return result;
}

Transform Transform::GetWorldToLocal(const Transform& parentWorldTransform) const
{
    Transform invParent = parentWorldTransform.GetInverse();
    Transform result;
    result.SetScale(invParent.GetScale() * GetScale());
    result.SetRotation(invParent.GetRotation() * GetRotation());
    result.SetPosition(invParent.GetPosition() + (invParent.GetScale() * (invParent.GetRotation() * GetPosition())));
    return result;
}

void Transform::clampEuler(Vector3& euler)
{
    euler.X = getAxisClampedValue(euler.X);
    euler.Y = getAxisClampedValue(euler.Y);
    euler.Z = getAxisClampedValue(euler.Z);
}

float Transform::getAxisClampedValue(float rotationValue)
{
    float angle = Math::FMod(rotationValue, 360.f);
    if (angle < 0.f) angle += 360.f;
    return angle;
}