#pragma once
#include <memory>
#include <vector>
#include <functional>
#include "Component.h"
#include "Matrix4x4.h"

class SkinnedMesh;
class Skeleton;
class Animation;

class AnimatorComponent : public Component
{
public:
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetAnimation(std::shared_ptr<Animation> animation);

    void Play();
    void Stop();
    void Pause();

    void Update(float deltaTime) override;
    void ComputeBoneWorldTransform(int boneIndex, std::vector<bool>& computed, const std::vector<Matrix4x4>& localTransforms);
    bool IsPlaying() const { return mIsPlaying; }
    void SetCurrentTime(float time) { mCurrentTime = time; }
    float GetCurrentTime() const { return mCurrentTime; }
    float GetDuration() const;
    void SetAnimationList(std::vector<std::shared_ptr<Animation>> animations);
    void SetAnimationByIndex(int index);
    int GetCurrentAnimationIndex() const { return mCurrentAnimIndex; }
    const std::vector<std::shared_ptr<Animation>>& GetAnimationList() const { return mAnimationList; }
    const std::vector<Matrix4x4>& GetFinalBoneMatrices() const { return mFinalBoneMatrices; }

private:
    std::shared_ptr<Skeleton> mSkeleton;
    std::shared_ptr<Animation> mAnimation;

    std::vector<Matrix4x4> mFinalBoneMatrices; //셰이더로 넘길 최종 스키닝 팔레트
    std::vector<Matrix4x4> mBoneWorldTransforms; //계층 누적용 임시 버퍼

    std::vector<std::shared_ptr<Animation>> mAnimationList;
    int mCurrentAnimIndex = -1;

    float mCurrentTime = 0.0f;
    bool mIsPlaying = false;
};