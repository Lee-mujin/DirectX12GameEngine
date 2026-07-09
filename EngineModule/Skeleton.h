#pragma once
#include <vector>
#include <string>
#include "Matrix4x4.h"

struct Bone
{
    std::string Name;
    int ParentIndex = -1; // 루트 본은 -1
    Matrix4x4 OffsetMatrix; // 바인드 포즈에서 모델 공간 -> 본 로컬 공간 변환
};

class Skeleton
{
public:
    std::vector<Bone> Bones;

    int FindBoneIndex(const std::string& name) const
    {
        for (size_t i = 0; i < Bones.size(); ++i)
        {
            if (Bones[i].Name == name)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};