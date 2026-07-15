#include "pch.h"
#include "AnimatorComponent.h"
#include "Skeleton.h"
#include "Animation.h"
#include "MathUtil.h"

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
    mIsFading = false;

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
    mIsFading = false;
}

void AnimatorComponent::Play(const std::string& clipName)
{
    int index = FindClipIndex(clipName);
    if (index < 0)
    {
        return;
    }

    if (index != mCurrentAnimIndex)
    {
        SetAnimationByIndex(index);
    }

    mIsPlaying = true;
}

void AnimatorComponent::CrossFade(const std::string& clipName, float fadeDuration)
{
    int targetIndex = FindClipIndex(clipName);
    if (targetIndex < 0 || targetIndex == mCurrentAnimIndex)
    {
        return; // 없는 클립이거나 이미 재생 중인 클립이면 무시
    }

    // 지금 재생 중이던 클립을 "이전 클립"으로 넘기고, 그 지점부터 페이드 시작
    mPrevAnimIndex = mCurrentAnimIndex;
    mPrevTime = mCurrentTime;

    mCurrentAnimIndex = targetIndex;
    mCurrentTime = 0.0f;

    mIsFading = (mPrevAnimIndex >= 0 && fadeDuration > 0.0001f);
    mFadeElapsed = 0.0f;
    mFadeDuration = fadeDuration;

    mIsPlaying = true;
}

void AnimatorComponent::Play() { mIsPlaying = true; }
void AnimatorComponent::Resume() { mIsPlaying = true; }
void AnimatorComponent::Pause() { mIsPlaying = false; }

void AnimatorComponent::Stop()
{
    mIsPlaying = false;
    mCurrentTime = 0.0f;
    mIsFading = false;
}

float AnimatorComponent::GetDuration() const
{
    return (mCurrentAnimIndex >= 0) ? mAnimationList[mCurrentAnimIndex]->Duration : 0.0f;
}

const std::string& AnimatorComponent::GetCurrentClipName() const
{
    return (mCurrentAnimIndex >= 0) ? mAnimationList[mCurrentAnimIndex]->Name : kEmptyName;
}

void AnimatorComponent::SampleBoneTRS(Animation* clip, size_t boneIndex, const Bone& bone, float time,
    Vector3& outPos, Quaternion& outRot, Vector3& outScale) const
{
    outPos = bone.BindPosition;
    outRot = bone.BindRotation;
    outScale = bone.BindScale;

    if (!clip || boneIndex >= clip->Channels.size())
    {
        return;
    }

    const BoneKeyframes& channel = clip->Channels[boneIndex];

    if (!channel.Positions.empty())
    {
        outPos = clip->SamplePosition(channel, time);
    }
    if (!channel.Rotations.empty())
    {
        outRot = clip->SampleRotation(channel, time);
    }
    if (!channel.Scales.empty())
    {
        outScale = clip->SampleScale(channel, time);
    }
}

void AnimatorComponent::Update(float deltaTime)
{
    if (!mSkeleton)
    {
        return;
    }

    Animation* currentAnim = (mCurrentAnimIndex >= 0) ? mAnimationList[mCurrentAnimIndex].get() : nullptr;
    Animation* prevAnim = (mIsFading && mPrevAnimIndex >= 0) ? mAnimationList[mPrevAnimIndex].get() : nullptr;

    if (mIsPlaying)
    {
        mCurrentTime += deltaTime * mSpeed;

        if (currentAnim && currentAnim->Duration > 0.0001f)
        {
            if (mLoop)
            {
                mCurrentTime = fmodf(mCurrentTime, currentAnim->Duration);
                if (mCurrentTime < 0.0f) mCurrentTime += currentAnim->Duration;
            }
            else if (mCurrentTime >= currentAnim->Duration)
            {
                mCurrentTime = currentAnim->Duration;
                mIsPlaying = false;
            }
        }

        if (mIsFading)
        {
            mPrevTime += deltaTime * mSpeed;
            if (prevAnim && prevAnim->Duration > 0.0001f)
            {
                mPrevTime = fmodf(mPrevTime, prevAnim->Duration);
                if (mPrevTime < 0.0f) mPrevTime += prevAnim->Duration;
            }

            mFadeElapsed += deltaTime;
            if (mFadeElapsed >= mFadeDuration)
            {
                mIsFading = false;
            }
        }
    }

    float fadeT = mIsFading ? Math::Clamp(mFadeElapsed / mFadeDuration, 0.0f, 1.0f) : 1.0f;

    const size_t boneCount = mSkeleton->Bones.size();
    std::vector<Matrix4x4> localTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        const Bone& bone = mSkeleton->Bones[i];

        Vector3 pos, scale;
        Quaternion rot;
        SampleBoneTRS(currentAnim, i, bone, mCurrentTime, pos, rot, scale);

        if (mIsFading)
        {
            Vector3 prevPos, prevScale;
            Quaternion prevRot;
            SampleBoneTRS(prevAnim, i, bone, mPrevTime, prevPos, prevRot, prevScale);

            pos = Math::Lerp(prevPos, pos, fadeT);
            rot = Quaternion::Slerp(prevRot, rot, fadeT);
            scale = Math::Lerp(prevScale, scale, fadeT);
        }

        localTransforms[i] = Matrix4x4::Scale(scale) * Matrix4x4::Rotate(rot) * Matrix4x4::Translate(pos);
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