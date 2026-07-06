#pragma once

struct Vector3;
struct Quaternion;

struct Matrix4x4
{
public:
    Matrix4x4();
    Matrix4x4(const Matrix4x4& other);
    Matrix4x4(
        float _11, float _12, float _13, float _14,
        float _21, float _22, float _23, float _24,
        float _31, float _32, float _33, float _34,
        float _41, float _42, float _43, float _44);

    static Matrix4x4 LookAt(const Vector3& from, const Vector3& to, const Vector3& up);
    static Matrix4x4 Perspective(float fov, float aspect, float zn, float zf);
    static Matrix4x4 Ortho(float w, float h, float zn, float zf);
    static Matrix4x4 Translate(const Vector3& v);
    static Matrix4x4 Rotate(const Vector3& v); // v: 라디안 (pitch=X, yaw=Y, roll=Z)
    static Matrix4x4 Rotate(const Quaternion& q);
    static Matrix4x4 Scale(const Vector3& v);

    bool IsIdentity();
    float GetDeterminant() const;
    Matrix4x4 GetInverse() const;
    Matrix4x4 GetTranspose() const;

    void SetIdentity();
    void SetInverse();
    void SetTranspose();
    void SetPerspective(float fov, float aspect, float zn, float zf);
    void SetOrtho(float w, float h, float zn, float zf);

    float& operator()(int row, int col);
    float operator()(int row, int col) const;

    operator float* ();
    operator const float* () const;

    Matrix4x4& operator*=(const float d);
    Matrix4x4& operator/=(const float d);
    Matrix4x4& operator+=(const Matrix4x4& other);
    Matrix4x4& operator-=(const Matrix4x4& other);
    Matrix4x4& operator*=(const Matrix4x4& other);

    Matrix4x4 operator*(float d);
    Matrix4x4 operator/(float d);
    Matrix4x4 operator+(const Matrix4x4& other);
    Matrix4x4 operator-(const Matrix4x4& other);
    Matrix4x4 operator*(const Matrix4x4& other);

    bool operator==(const Matrix4x4& other);
    bool operator!=(const Matrix4x4& other);

    static const Matrix4x4 Identity;
    static const Matrix4x4 Zero;

    union
    {
        struct
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};