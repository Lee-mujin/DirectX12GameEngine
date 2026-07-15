#pragma once
#include <vector>
#include <string>
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Quaternion.h"

struct Bone
{
    std::string Name;
    int ParentIndex = -1;
    Matrix4x4 OffsetMatrix;
    Matrix4x4 LocalBindTransform; //정적 폴백용으로보관

    //Cross Fade에서 TRS로 블렌드하기 위한 바인드 포즈 성분
    Vector3 BindPosition{ 0.0f, 0.0f, 0.0f };
    Quaternion BindRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
    Vector3 BindScale{ 1.0f, 1.0f, 1.0f };
};

class Skeleton
{
public:
    std::vector<Bone> Bones;

    int FindBoneIndex(const std::string& name) const
    {
        for (size_t i = 0; i < Bones.size(); ++i)
        {
            if (Bones[i].Name == name) return static_cast<int>(i);
        }
        return -1;
    }
};