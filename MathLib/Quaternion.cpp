#include "pch.h"
#include "MathUtil.h"
#include "Vector3.h"
#include "Quaternion.h"

const Quaternion Quaternion::Identity(0.f, 0.f, 0.f, 1.f);

Quaternion::Quaternion() : X(0.f), Y(0.f), Z(0.f), W(1.f) {}
Quaternion::Quaternion(const Quaternion& other) : X(other.X), Y(other.Y), Z(other.Z), W(other.W) {}
Quaternion::Quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

float Quaternion::Dot(const Quaternion& q1, const Quaternion& q2)
{
    return q1.X * q2.X + q1.Y * q2.Y + q1.Z * q2.Z + q1.W * q2.W;
}

Quaternion Quaternion::Slerp(const Quaternion& q1, const Quaternion& q2, const float t)
{
    float cosOmega = Dot(q1, q2);

    Quaternion b = q2;
    if (cosOmega < 0.f)
    {
        cosOmega = -cosOmega;
        b = Quaternion(-q2.X, -q2.Y, -q2.Z, -q2.W);
    }

    float k0, k1;
    if (cosOmega > 0.9999f)
    {
        k0 = 1.f - t;
        k1 = t;
    }
    else
    {
        float sinOmega = std::sqrtf(1.f - cosOmega * cosOmega);
        float omega = std::atan2f(sinOmega, cosOmega);
        float invSin = 1.f / sinOmega;
        k0 = std::sinf((1.f - t) * omega) * invSin;
        k1 = std::sinf(t * omega) * invSin;
    }

    return Quaternion(
        q1.X * k0 + b.X * k1,
        q1.Y * k0 + b.Y * k1,
        q1.Z * k0 + b.Z * k1,
        q1.W * k0 + b.W * k1);
}

void Quaternion::AngleAxis(const float angle, const Vector3& axis)
{
    Vector3 n = axis.GetNormalize();
    float half = angle * 0.5f;
    float s = std::sinf(half);

    X = n.X * s;
    Y = n.Y * s;
    Z = n.Z * s;
    W = std::cosf(half);
}

void Quaternion::ToAngleAxis(float* const angle, Vector3* const axis) const
{
    assert(angle);
    assert(axis);

    Quaternion q = GetNormalize();
    *angle = 2.f * std::acosf(q.W);

    float s = std::sqrtf(1.f - q.W * q.W);
    if (s < Math::SMALL_NUMBER)
    {
        *axis = Vector3(1.f, 0.f, 0.f);
    }
    else
    {
        *axis = Vector3(q.X / s, q.Y / s, q.Z / s);
    }
}

void Quaternion::Inverse()
{
    X = -X;
    Y = -Y;
    Z = -Z;
}

Quaternion Quaternion::GetInverse() const
{
    return Quaternion(-X, -Y, -Z, W);
}

void Quaternion::Normalize()
{
    const float squareSum = X * X + Y * Y + Z * Z + W * W;

    if (squareSum >= Math::SMALL_NUMBER)
    {
        const float scale = 1.f / std::sqrtf(squareSum);
        X *= scale;
        Y *= scale;
        Z *= scale;
        W *= scale;
    }
    else
    {
        *this = Quaternion::Identity;
    }
}

Quaternion Quaternion::GetNormalize() const
{
    Quaternion result(*this);
    result.Normalize();
    return result;
}

Vector3 Quaternion::ToEuler() const
{
    Vector3 result;

    float sinrCosp = 2 * (W * Z + X * Y);
    float cosrCosp = 1 - 2 * (Z * Z + X * X);
    result.Z = Math::Rad2Deg(std::atan2f(sinrCosp, cosrCosp));

    float pitchTest = W * X - Y * Z;
    float asinThreshold = 0.4999995f;
    if (pitchTest < -asinThreshold)
    {
        result.X = -90.f;
    }
    else if (pitchTest > asinThreshold)
    {
        result.X = 90.f;
    }
    else
    {
        float sinp = 2 * pitchTest;
        result.X = Math::Rad2Deg(std::asinf(sinp));
    }

    float sinyCosp = 2 * (W * Y + X * Z);
    float cosyCosp = 1.f - 2 * (X * X + Y * Y);
    result.Y = Math::Rad2Deg(std::atan2f(sinyCosp, cosyCosp));

    return result;
}

Vector3 Quaternion::operator*(const Vector3& v) const
{
    Quaternion q1 = GetInverse();
    Quaternion q2 = q1 * Quaternion(v.X, v.Y, v.Z, 1.f) * *this;
    return Vector3(q2.X, q2.Y, q2.Z);
}

Quaternion Quaternion::operator*(const Quaternion& other) const
{
    return Quaternion(
        W * other.X + X * other.W + Y * other.Z - Z * other.Y,
        W * other.Y - X * other.Z + Y * other.W + Z * other.X,
        W * other.Z + X * other.Y - Y * other.X + Z * other.W,
        W * other.W - X * other.X - Y * other.Y - Z * other.Z);
}

Quaternion Quaternion::operator*=(const Quaternion& other)
{
    *this = *this * other;
    return *this;
}

bool Quaternion::operator==(const Quaternion& other) const
{
    return std::fabsf(X - other.X) <= Math::SMALL_NUMBER
        && std::fabsf(Y - other.Y) <= Math::SMALL_NUMBER
        && std::fabsf(Z - other.Z) <= Math::SMALL_NUMBER
        && std::fabsf(W - other.W) <= Math::SMALL_NUMBER;
}

bool Quaternion::operator!=(const Quaternion& other) const { return !(*this == other); }