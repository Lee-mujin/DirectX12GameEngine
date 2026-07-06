#pragma once

struct Vector2;
struct Quaternion;

struct Vector3
{
    Vector3();
    Vector3(const float x, const float y, const float z);
    Vector3(const Vector3& other);
    Vector3(const Vector2& other);

    static float Angle(const Vector3& from, const Vector3& to);
    static Vector3 Cross(const Vector3& v1, const Vector3& v2);
    static float Dot(const Vector3& v1, const Vector3& v2);
    static float Distance(const Vector3& v1, const Vector3& v2);
    static Vector3 Min(const Vector3& v1, const Vector3& v2);
    static Vector3 Max(const Vector3& v1, const Vector3& v2);

    float GetSize() const;
    float GetSizeSq() const;
    void Normalize();
    Vector3 GetNormalize() const;
    float GetMin() const;
    float GetMax() const;
    Vector2 ToVector2() const;
    Quaternion ToQuaternion() const;

    operator float* ();
    operator const float* () const;

    Vector3 operator+() const;
    Vector3 operator-() const;

    Vector3 operator*(const float f) const;
    Vector3 operator/(const float f) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator*(const Vector3& other) const;

    Vector3& operator*=(const float f);
    Vector3& operator/=(const float f);
    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);

    bool operator==(const Vector3& other) const;
    bool operator!=(const Vector3& other) const;

    Vector3 operator=(const Vector2& other);

    static const Vector3 Left;
    static const Vector3 Right;
    static const Vector3 Forward;
    static const Vector3 Back;
    static const Vector3 Up;
    static const Vector3 Down;
    static const Vector3 One;
    static const Vector3 Zero;

    float X, Y, Z;
};