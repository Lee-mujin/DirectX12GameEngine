#include "pch.h"
#include "Animation.h"
#include "MathUtil.h"

namespace
{
    template<typename T>
    int FindKeyIndex(const std::vector<float>& times, float time)
    {
        if (times.empty())
        {
            return -1;
        }

        for (size_t i = 0; i < times.size() - 1; ++i)
        {
            if (time < times[i + 1])
            {
                return static_cast<int>(i);
            }
        }

        return static_cast<int>(times.size() - 2 >= 0 ? times.size() - 2 : 0);
    }
}

Vector3 Animation::SamplePosition(const BoneKeyframes& channel, float time) const
{
    if (channel.Positions.empty())
    {
        return Vector3::Zero;
    }

    if (channel.Positions.size() == 1)
    {
        return channel.Positions[0];
    }

    int i = FindKeyIndex<Vector3>(channel.PositionTimes, time);
    if (i < 0)
    {
        return channel.Positions[0];
    }

    float t0 = channel.PositionTimes[i];
    float t1 = channel.PositionTimes[i + 1];
    float alpha = (t1 - t0) > 0.0001f ? (time - t0) / (t1 - t0) : 0.0f;
    alpha = Math::Clamp(alpha, 0.0f, 1.0f); 

    return Math::Lerp(channel.Positions[i], channel.Positions[i + 1], alpha); //position 선형보간
}

Quaternion Animation::SampleRotation(const BoneKeyframes& channel, float time) const
{
    if (channel.Rotations.empty())
    {
        return Quaternion::Identity;
    }

    if (channel.Rotations.size() == 1)
    {
        return channel.Rotations[0];
    }

    int i = FindKeyIndex<Quaternion>(channel.RotationTimes, time);
    if (i < 0)
    {
        return channel.Rotations[0];
    }

    float t0 = channel.RotationTimes[i];
    float t1 = channel.RotationTimes[i + 1];
    float alpha = (t1 - t0) > 0.0001f ? (time - t0) / (t1 - t0) : 0.0f;
    alpha = Math::Clamp(alpha, 0.0f, 1.0f);

	return Quaternion::Slerp(channel.Rotations[i], channel.Rotations[i + 1], alpha); //Rotations 구간보간
}

Vector3 Animation::SampleScale(const BoneKeyframes& channel, float time) const
{
    if (channel.Scales.empty())
    {
        return Vector3::One;
    }

    if (channel.Scales.size() == 1)
    {
        return channel.Scales[0];
    }

    int i = FindKeyIndex<Vector3>(channel.ScaleTimes, time);
    if (i < 0)
    {
        return channel.Scales[0];
    }

    float t0 = channel.ScaleTimes[i];
    float t1 = channel.ScaleTimes[i + 1];
    float alpha = (t1 - t0) > 0.0001f ? (time - t0) / (t1 - t0) : 0.0f;
    alpha = Math::Clamp(alpha, 0.0f, 1.0f);

    return Math::Lerp(channel.Scales[i], channel.Scales[i + 1], alpha); //scale 선형보간
}