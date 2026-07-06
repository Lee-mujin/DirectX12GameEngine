#include "pch.h"
#include "MathUtil.h"
#include "Vector2.h"
#include "Vector3.h"

const Vector2 Vector2::Left(-1.f, 0.f);
const Vector2 Vector2::Right(1.f, 0.f);
const Vector2 Vector2::Up(0.f, 1.f);
const Vector2 Vector2::Down(0.f, -1.f);
const Vector2 Vector2::One(1.f, 1.f);
const Vector2 Vector2::Zero(0.f, 0.f);

Vector2::Vector2() : X(0.f), Y(0.f) {}
Vector2::Vector2(const float x, const float y) : X(x), Y(y) {}
Vector2::Vector2(const Vector2& other) : X(other.X), Y(other.Y) {}
Vector2::Vector2(const Vector3& other) : X(other.X), Y(other.Y) {}

float Vector2::Angle(const Vector2& from, const Vector2& to)
{
    float size = std::sqrtf(from.GetSizeSq() * to.GetSizeSq());
    float dot = Dot(from, to);
    float cosAngle = std::acosf(dot / size);
    cosAngle = Math::Rad2Deg(cosAngle);
    return (from.X * to.Y - from.Y * to.X > 0.f) ? cosAngle : -cosAngle;
}

float Vector2::Dot(const Vector2& v1, const Vector2& v2)
{
    return v1.X * v2.X + v1.Y * v2.Y;
}

float Vector2::Distance(const Vector2& v1, const Vector2& v2)
{
    float xDist = v1.X - v2.X;
    float yDist = v1.Y - v2.Y;
    return std::sqrtf((xDist * xDist) + (yDist * yDist));
}

Vector2 Vector2::Min(const Vector2& v1, const Vector2& v2)
{
    return Vector2(std::fminf(v1.X, v2.X), std::fminf(v1.Y, v2.Y));
}

Vector2 Vector2::Max(const Vector2& v1, const Vector2& v2)
{
    return Vector2(std::fmaxf(v1.X, v2.X), std::fmaxf(v1.Y, v2.Y));
}

float Vector2::GetSize() const { return std::sqrtf(X * X + Y * Y); }
float Vector2::GetSizeSq() const { return X * X + Y * Y; }

void Vector2::Normalize()
{
    float size = GetSize();
    X /= size;
    Y /= size;
}

Vector2 Vector2::GetNormalize() const
{
    Vector2 result(*this);
    result.Normalize();
    return result;
}

float Vector2::GetMin() const { return std::fminf(X, Y); }
float Vector2::GetMax() const { return std::fmaxf(X, Y); }
Vector3 Vector2::ToVector3() const { return Vector3(X, Y, 0.f); }

Vector2::operator float* () { return (float*)&X; }
Vector2::operator const float* () const { return (const float*)&X; }

Vector2 Vector2::operator+() const { return Vector2(+X, +Y); }
Vector2 Vector2::operator-() const { return Vector2(-X, -Y); }
Vector2 Vector2::operator*(const float f) const { return Vector2(X * f, Y * f); }
Vector2 Vector2::operator/(const float f) const { float inv = 1.f / f; return Vector2(X * inv, Y * inv); }
Vector2 Vector2::operator+(const Vector2& other) const { return Vector2(X + other.X, Y + other.Y); }
Vector2 Vector2::operator-(const Vector2& other) const { return Vector2(X - other.X, Y - other.Y); }
Vector2 Vector2::operator*(const Vector2& other) const { return Vector2(X * other.X, Y * other.Y); }

Vector2& Vector2::operator+=(const Vector2& other) { X += other.X; Y += other.Y; return *this; }
Vector2& Vector2::operator-=(const Vector2& other) { X -= other.X; Y -= other.Y; return *this; }
Vector2& Vector2::operator*=(const float f) { X *= f; Y *= f; return *this; }
Vector2& Vector2::operator/=(const float f) { float inv = 1.f / f; X *= inv; Y *= inv; return *this; }

bool Vector2::operator==(const Vector2& other) const
{
    return std::fabsf(X - other.X) <= Math::SMALL_NUMBER && std::fabsf(Y - other.Y) <= Math::SMALL_NUMBER;
}

bool Vector2::operator!=(const Vector2& other) const { return !(*this == other); }

Vector2 Vector2::operator=(const Vector3& other)
{
    X = other.X;
    Y = other.Y;
    return *this;
}