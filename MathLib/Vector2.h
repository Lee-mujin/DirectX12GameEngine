#pragma once

struct Vector3;

struct Vector2
{
    Vector2();
    Vector2(const float x, const float y);
    Vector2(const Vector2& other);
    Vector2(const Vector3& other);

    static float Angle(const Vector2& from, const Vector2& to);
    static float Dot(const Vector2& v1, const Vector2& v2);
    static float Distance(const Vector2& v1, const Vector2& v2);
    static Vector2 Min(const Vector2& v1, const Vector2& v2);
    static Vector2 Max(const Vector2& v1, const Vector2& v2);

    float GetSize() const;
    float GetSizeSq() const;
    void Normalize();
    Vector2 GetNormalize() const;
    float GetMin() const;
    float GetMax() const;
    Vector3 ToVector3() const;

    operator float* ();
    operator const float* () const;

    Vector2 operator+() const;
    Vector2 operator-() const;

    Vector2 operator*(const float f) const;
    Vector2 operator/(const float f) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2 operator+(const Vector2& other) const;
    Vector2 operator*(const Vector2& other) const;

    Vector2& operator*=(const float f);
    Vector2& operator/=(const float f);
    Vector2& operator+=(const Vector2& other);
    Vector2& operator-=(const Vector2& other);

    bool operator==(const Vector2& other) const;
    bool operator!=(const Vector2& other) const;

    Vector2 operator=(const Vector3& other);

    static const Vector2 Left;
    static const Vector2 Right;
    static const Vector2 Up;
    static const Vector2 Down;
    static const Vector2 One;
    static const Vector2 Zero;

    float X, Y;
};