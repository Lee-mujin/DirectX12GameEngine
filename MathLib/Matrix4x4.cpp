#include "pch.h"
#include "MathUtil.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Quaternion.h"

using namespace DirectX;

namespace
{
    inline XMMATRIX ToXM(const Matrix4x4& mat)
    {
        return XMMatrixSet(
            mat._11, mat._12, mat._13, mat._14,
            mat._21, mat._22, mat._23, mat._24,
            mat._31, mat._32, mat._33, mat._34,
            mat._41, mat._42, mat._43, mat._44);
    }

    inline Matrix4x4 FromXM(const XMMATRIX& mat)
    {
        XMFLOAT4X4 f;
        XMStoreFloat4x4(&f, mat);
        return Matrix4x4(
            f._11, f._12, f._13, f._14,
            f._21, f._22, f._23, f._24,
            f._31, f._32, f._33, f._34,
            f._41, f._42, f._43, f._44);
    }
}

const Matrix4x4 Matrix4x4::Identity = Matrix4x4(
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f);

const Matrix4x4 Matrix4x4::Zero = Matrix4x4(
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f);

Matrix4x4::Matrix4x4() : Matrix4x4(Identity) {}

Matrix4x4::Matrix4x4(const Matrix4x4& other)
{
    std::memcpy(this, &other, sizeof(Matrix4x4));
}

Matrix4x4::Matrix4x4(
    float _11, float _12, float _13, float _14,
    float _21, float _22, float _23, float _24,
    float _31, float _32, float _33, float _34,
    float _41, float _42, float _43, float _44)
    : _11(_11), _12(_12), _13(_13), _14(_14)
    , _21(_21), _22(_22), _23(_23), _24(_24)
    , _31(_31), _32(_32), _33(_33), _34(_34)
    , _41(_41), _42(_42), _43(_43), _44(_44)
{}

Matrix4x4 Matrix4x4::LookAt(const Vector3& from, const Vector3& to, const Vector3& up)
{
    XMVECTOR eye = XMVectorSet(from.X, from.Y, from.Z, 0.f);
    XMVECTOR target = XMVectorSet(to.X, to.Y, to.Z, 0.f);
    XMVECTOR upVec = XMVectorSet(up.X, up.Y, up.Z, 0.f);
    return FromXM(XMMatrixLookAtLH(eye, target, upVec));
}

Matrix4x4 Matrix4x4::Perspective(float fov, float aspect, float zn, float zf)
{
    return FromXM(XMMatrixPerspectiveFovLH(fov, aspect, zn, zf));
}

Matrix4x4 Matrix4x4::Ortho(float w, float h, float zn, float zf)
{
    return FromXM(XMMatrixOrthographicLH(w, h, zn, zf));
}

Matrix4x4 Matrix4x4::Translate(const Vector3& v)
{
    return FromXM(XMMatrixTranslation(v.X, v.Y, v.Z));
}

Matrix4x4 Matrix4x4::Rotate(const Vector3& v)
{
    return FromXM(XMMatrixRotationRollPitchYaw(v.X, v.Y, v.Z));
}

Matrix4x4 Matrix4x4::Rotate(const Quaternion& q)
{
    XMVECTOR quat = XMVectorSet(q.X, q.Y, q.Z, q.W);
    return FromXM(XMMatrixRotationQuaternion(quat));
}

Matrix4x4 Matrix4x4::Scale(const Vector3& v)
{
    return FromXM(XMMatrixScaling(v.X, v.Y, v.Z));
}

bool Matrix4x4::IsIdentity() const
{
    return *this == Identity;
}

float Matrix4x4::GetDeterminant() const
{
    XMVECTOR det = XMMatrixDeterminant(ToXM(*this));
    return XMVectorGetX(det);
}

Matrix4x4 Matrix4x4::GetInverse() const
{
    XMVECTOR det;
    XMMATRIX inv = XMMatrixInverse(&det, ToXM(*this));
    return FromXM(inv);
}

Matrix4x4 Matrix4x4::GetTranspose() const
{
    return Matrix4x4(
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]);
}

void Matrix4x4::SetIdentity() { *this = Identity; }
void Matrix4x4::SetInverse() { *this = GetInverse(); }
void Matrix4x4::SetTranspose() { *this = GetTranspose(); }
void Matrix4x4::SetPerspective(float fov, float aspect, float zn, float zf) { *this = Perspective(fov, aspect, zn, zf); }
void Matrix4x4::SetOrtho(float w, float h, float zn, float zf) { *this = Ortho(w, h, zn, zf); }

float& Matrix4x4::operator()(int row, int col) { return m[row][col]; }
float Matrix4x4::operator()(int row, int col) const { return m[row][col]; }

Matrix4x4::operator float* () { return (float*)&_11; }
Matrix4x4::operator const float* () const { return (float*)&_11; }

Matrix4x4& Matrix4x4::operator*=(const float d)
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            m[r][c] *= d;
    return *this;
}

Matrix4x4& Matrix4x4::operator/=(const float d)
{
    float inv = 1.f / d;
    return *this *= inv;
}

Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& other)
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            m[r][c] += other.m[r][c];
    return *this;
}

Matrix4x4& Matrix4x4::operator-=(const Matrix4x4& other)
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            m[r][c] -= other.m[r][c];
    return *this;
}

Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& other)
{
    *this = FromXM(XMMatrixMultiply(ToXM(*this), ToXM(other)));
    return *this;
}

Matrix4x4 Matrix4x4::operator*(float d) const { Matrix4x4 result(*this); result *= d; return result; }
Matrix4x4 Matrix4x4::operator/(float d) const { Matrix4x4 result(*this); result /= d; return result; }
Matrix4x4 Matrix4x4::operator+(const Matrix4x4& other) const { Matrix4x4 result(*this); result += other; return result; }
Matrix4x4 Matrix4x4::operator-(const Matrix4x4& other) const { Matrix4x4 result(*this); result -= other; return result; }

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const
{
    return FromXM(XMMatrixMultiply(ToXM(*this), ToXM(other)));
}

bool Matrix4x4::operator==(const Matrix4x4& other) const
{
    return 0 == std::memcmp(this, &other, sizeof(Matrix4x4));
}

bool Matrix4x4::operator!=(const Matrix4x4& other) const
{
    return 0 != std::memcmp(this, &other, sizeof(Matrix4x4));
}

bool Matrix4x4::Decompose(Vector3& outPosition, Quaternion& outRotation, Vector3& outScale) const
{
    XMVECTOR scale;
    XMVECTOR rotation;
    XMVECTOR translation;

    // DirectXMath의 행렬 분해 연산 수행
    if (XMMatrixDecompose(&scale, &rotation, &translation, ToXM(*this)))
    {
        XMFLOAT3 s, t;
        XMFLOAT4 r;

        XMStoreFloat3(&s, scale);
        XMStoreFloat4(&r, rotation);
        XMStoreFloat3(&t, translation);

        outScale = Vector3(s.x, s.y, s.z);
        outRotation = Quaternion(r.x, r.y, r.z, r.w);
        outPosition = Vector3(t.x, t.y, t.z);

        return true;
    }

    return false;
}