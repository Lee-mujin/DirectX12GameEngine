#include "pch.h"
#include "AnimatorComponent.h"
#include "Skeleton.h"
#include "Animation.h"
#include <vector>
void AnimatorComponent::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
    mSkeleton = skeleton;

    if (mSkeleton)
    {
        mFinalBoneMatrices.assign(mSkeleton->Bones.size(), Matrix4x4::Identity);
        mBoneWorldTransforms.assign(mSkeleton->Bones.size(), Matrix4x4::Identity);
    }
}

void AnimatorComponent::SetAnimation(std::shared_ptr<Animation> animation)
{
    mAnimation = animation;
    mCurrentTime = 0.0f;
}

void AnimatorComponent::Play()
{
    mIsPlaying = true;
}

void AnimatorComponent::Stop()
{
    mIsPlaying = false;
    mCurrentTime = 0.0f;
}

void AnimatorComponent::Pause()
{
    mIsPlaying = false;
}

void AnimatorComponent::Update(float deltaTime)
{
    if (!mSkeleton)
    {
        return;
    }

    if (mAnimation && mIsPlaying)
    {
        mCurrentTime += deltaTime;
        if (mAnimation->Duration > 0.0001f)
        {
            mCurrentTime = fmodf(mCurrentTime, mAnimation->Duration);
        }
    }

    const size_t boneCount = mSkeleton->Bones.size();
    std::vector<Matrix4x4> localTransforms(boneCount);

    // 1) 각 본의 로컬 트랜스폼만 먼저 전부 계산
    for (size_t i = 0; i < boneCount; ++i)
    {
        const Bone& bone = mSkeleton->Bones[i];
        Matrix4x4 localTransform = bone.LocalBindTransform;

        if (mAnimation && i < mAnimation->Channels.size())
        {
            const BoneKeyframes& channel = mAnimation->Channels[i];
            bool hasAnyKey = !channel.Positions.empty() || !channel.Rotations.empty() || !channel.Scales.empty();

            if (hasAnyKey)
            {
                Vector3 pos = channel.Positions.empty() ? Vector3::Zero : mAnimation->SamplePosition(channel, mCurrentTime);
                Quaternion rot = channel.Rotations.empty() ? Quaternion::Identity : mAnimation->SampleRotation(channel, mCurrentTime);
                Vector3 scale = channel.Scales.empty() ? Vector3::One : mAnimation->SampleScale(channel, mCurrentTime);

                localTransform = Matrix4x4::Scale(scale) * Matrix4x4::Rotate(rot) * Matrix4x4::Translate(pos);
            }
        }

        localTransforms[i] = localTransform;
    }

    // 2) 재귀로 월드 트랜스폼 계산 (배열 순서와 무관하게 항상 부모 먼저 보장)
    std::vector<bool> computed(boneCount, false);
    for (size_t i = 0; i < boneCount; ++i)
    {
        ComputeBoneWorldTransform(static_cast<int>(i), computed, localTransforms);
    }

    // 3) 최종 스키닝 팔레트
    for (size_t i = 0; i < boneCount; ++i)
    {
        mFinalBoneMatrices[i] = mSkeleton->Bones[i].OffsetMatrix * mBoneWorldTransforms[i];
    }
}

void AnimatorComponent::ComputeBoneWorldTransform(int boneIndex, std::vector<bool>& computed, const std::vector<Matrix4x4>& localTransforms)
{
    if (computed[boneIndex])
    {
        return;
    }

    int parentIndex = mSkeleton->Bones[boneIndex].ParentIndex;

    if (parentIndex >= 0)
    {
        ComputeBoneWorldTransform(parentIndex, computed, localTransforms); // 부모부터 먼저 확실히 계산
        mBoneWorldTransforms[boneIndex] = localTransforms[boneIndex] * mBoneWorldTransforms[parentIndex];
    }
    else
    {
        mBoneWorldTransforms[boneIndex] = localTransforms[boneIndex];
    }

    computed[boneIndex] = true;
}