#pragma once

struct Vector3;

struct Quaternion
{
    Quaternion();
    Quaternion(const Quaternion& other);
    Quaternion(float x, float y, float z, float w);

    static float Dot(const Quaternion& q1, const Quaternion& q2);
    static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, const float t);

    void AngleAxis(const float angle, const Vector3& axis);
    void ToAngleAxis(float* const angle, Vector3* const axis) const;
    void Inverse();
    Quaternion GetInverse() const;
    void Normalize();
    Quaternion GetNormalize() const;
    Vector3 ToEuler() const;

    Vector3 operator*(const Vector3& v) const;
    Quaternion operator*(const Quaternion& other) const;
    Quaternion operator*=(const Quaternion& other);
    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    static const Quaternion Identity;

    float X, Y, Z, W;
};