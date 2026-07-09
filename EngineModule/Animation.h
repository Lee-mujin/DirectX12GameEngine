#pragma once
#include <vector>
#include <string>
#include "Vector3.h"
#include "Quaternion.h"

struct BoneKeyframes
{
    int BoneIndex = -1;

    std::vector<float> PositionTimes;
    std::vector<Vector3> Positions;

    std::vector<float> RotationTimes;
    std::vector<Quaternion> Rotations;

    std::vector<float> ScaleTimes;
    std::vector<Vector3> Scales;
};

class Animation
{
public:
    std::string Name;
    float Duration = 0.0f;
    std::vector<BoneKeyframes> Channels;

    Vector3 SamplePosition(const BoneKeyframes& channel, float time) const;
    Quaternion SampleRotation(const BoneKeyframes& channel, float time) const;
    Vector3 SampleScale(const BoneKeyframes& channel, float time) const;
};