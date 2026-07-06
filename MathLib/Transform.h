#pragma once

#include "Vector3.h"
#include "Quaternion.h"

struct Matrix4x4;

struct Transform
{
public:
    Transform();
    Transform(const Vector3& position);
    Transform(const Vector3& position, const Quaternion& rotation);
    Transform(const Vector3& position, const Vector3& euler);
    Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);
    Transform(const Vector3& position, const Vector3& euler, const Vector3& scale);

public:
    Vector3 GetPosition() const;
    Quaternion GetRotation() const;
    Vector3 GetEulerAngles() const;
    Vector3 GetScale() const;
    Vector3 GetAxisX() const;
    Vector3 GetAxisY() const;
    Vector3 GetAxisZ() const;
    Matrix4x4 GetMatrix() const;

    void SetPosition(const Vector3& position);
    void SetRotation(const Quaternion& rotation);
    void SetRotation(const Vector3& euler);
    void SetScale(const Vector3& scale);

    void AddPosition(const Vector3& position);
    void AddRotation(const Quaternion& rotation);
    void AddRotation(const Vector3& euler);
    void AddScale(const Vector3& scale);
    void AddRotationX(const float degree);
    void AddRotationY(const float degree);
    void AddRotationZ(const float degree);

    void Translate(const Vector3& translation);
    void LookAt(const Transform& target);

    Transform GetInverse() const;
    Transform GetLocalToWorld(const Transform& parentWorldTransform) const;
    Transform GetWorldToLocal(const Transform& parentWorldTransform) const;

private:
    void clampEuler(Vector3& euler);
    float getAxisClampedValue(float rotationValue);

private:
    Vector3 mPosition;
    Vector3 mEulerAngles; // 도(degree) 단위로 저장
    Vector3 mScale;
};