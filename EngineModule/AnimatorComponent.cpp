// AnimatorComponent.cpp
#include "pch.h"
#include "AnimatorComponent.h"
#include "Skeleton.h"
#include "Animation.h"

const std::string AnimatorComponent::kEmptyName = "";

void AnimatorComponent::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
    mSkeleton = skeleton;
    if (mSkeleton)
    {
        mFinalBoneMatrices.assign(mSkeleton->Bones.size(), Matrix4x4::Identity);
        mBoneWorldTransforms.assign(mSkeleton->Bones.size(), Matrix4x4::Identity);
    }
}

void AnimatorComponent::SetAnimationList(std::vector<std::shared_ptr<Animation>> animations)
{
    mAnimationList = std::move(animations);
    mCurrentAnimIndex = -1;
    mCurrentTime = 0.0f;

    if (!mAnimationList.empty())
    {
        SetAnimationByIndex(0);
    }
}

int AnimatorComponent::FindClipIndex(const std::string& name) const
{
    for (size_t i = 0; i < mAnimationList.size(); ++i)
    {
        if (mAnimationList[i]->Name == name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void AnimatorComponent::SetAnimationByIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(mAnimationList.size()))
    {
        return;
    }

    mCurrentAnimIndex = index;
    mCurrentTime = 0.0f;
}

void AnimatorComponent::Play(const std::string& clipName)
{
    int index = FindClipIndex(clipName);
    if (index < 0)
    {
        return; //없는 애니메이션 이름이면 무시
    }

    if (index != mCurrentAnimIndex)
    {
        SetAnimationByIndex(index);
    }

    mIsPlaying = true;
}

void AnimatorComponent::Play() { mIsPlaying = true; }
void AnimatorComponent::Resume() { mIsPlaying = true; }
void AnimatorComponent::Pause() { mIsPlaying = false; }

void AnimatorComponent::Stop()
{
    mIsPlaying = false;
    mCurrentTime = 0.0f;
}

float AnimatorComponent::GetDuration() const
{
    return (mCurrentAnimIndex >= 0) ? mAnimationList[mCurrentAnimIndex]->Duration : 0.0f;
}

const std::string& AnimatorComponent::GetCurrentClipName() const
{
    return (mCurrentAnimIndex >= 0) ? mAnimationList[mCurrentAnimIndex]->Name : kEmptyName;
}

void AnimatorComponent::Update(float deltaTime)
{
    if (!mSkeleton)
    {
        return;
    }

    Animation* currentAnim = (mCurrentAnimIndex >= 0) ? mAnimationList[mCurrentAnimIndex].get() : nullptr;

    if (currentAnim && mIsPlaying)
    {
        mCurrentTime += deltaTime * mSpeed;

        if (currentAnim->Duration > 0.0001f)
        {
            if (mLoop)
            {
                mCurrentTime = fmodf(mCurrentTime, currentAnim->Duration);
                if (mCurrentTime < 0.0f)
                {
                    mCurrentTime += currentAnim->Duration;
                }
            }
            else if (mCurrentTime >= currentAnim->Duration)
            {
                mCurrentTime = currentAnim->Duration;
                mIsPlaying = false; //논루프는 끝나면 자동 정지
            }
        }
    }

    const size_t boneCount = mSkeleton->Bones.size();
    std::vector<Matrix4x4> localTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        const Bone& bone = mSkeleton->Bones[i];
        Matrix4x4 localTransform = bone.LocalBindTransform;

        if (currentAnim && i < currentAnim->Channels.size())
        {
            const BoneKeyframes& channel = currentAnim->Channels[i];
            bool hasAnyKey = !channel.Positions.empty() || !channel.Rotations.empty() || !channel.Scales.empty();

            if (hasAnyKey)
            {
                Vector3 pos = channel.Positions.empty() ? Vector3::Zero : currentAnim->SamplePosition(channel, mCurrentTime);
                Quaternion rot = channel.Rotations.empty() ? Quaternion::Identity : currentAnim->SampleRotation(channel, mCurrentTime);
                Vector3 scale = channel.Scales.empty() ? Vector3::One : currentAnim->SampleScale(channel, mCurrentTime);

                localTransform = Matrix4x4::Scale(scale) * Matrix4x4::Rotate(rot) * Matrix4x4::Translate(pos);
            }
        }

        localTransforms[i] = localTransform;
    }

    std::vector<bool> computed(boneCount, false);
    for (size_t i = 0; i < boneCount; ++i)
    {
        ComputeBoneWorldTransform(static_cast<int>(i), computed, localTransforms);
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        mFinalBoneMatrices[i] = mSkeleton->Bones[i].OffsetMatrix * mBoneWorldTransforms[i];
    }
}

//재귀 계산
void AnimatorComponent::ComputeBoneWorldTransform(int boneIndex, std::vector<bool>& computed, const std::vector<Matrix4x4>& localTransforms)
{
    if (computed[boneIndex])
    {
        return;
    }

    int parentIndex = mSkeleton->Bones[boneIndex].ParentIndex;

    if (parentIndex >= 0)
    {
        ComputeBoneWorldTransform(parentIndex, computed, localTransforms);
        mBoneWorldTransforms[boneIndex] = localTransforms[boneIndex] * mBoneWorldTransforms[parentIndex];
    }
    else
    {
        mBoneWorldTransforms[boneIndex] = localTransforms[boneIndex];
    }

    computed[boneIndex] = true;
}