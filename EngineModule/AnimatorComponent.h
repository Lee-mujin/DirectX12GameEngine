#pragma once
#include <memory>
#include <vector>
#include <string>
#include "Component.h"
#include "Matrix4x4.h"
#include "Skeleton.h" 

class SkinnedMesh;
class Animation;
struct BoneKeyframes;

class AnimatorComponent : public Component
{
public:
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetAnimationList(std::vector<std::shared_ptr<Animation>> animations);

    void Play(const std::string& clipName); //즉시 전환
    void CrossFade(const std::string& clipName, float fadeDuration); //부드럽게 전환
    void Play();
    void Resume();
    void Pause();
    void Stop();

    void SetSpeed(float speed) { mSpeed = speed; }
    float GetSpeed() const { return mSpeed; }

    void SetLoop(bool loop) { mLoop = loop; }
    bool IsLooping() const { return mLoop; }

    bool IsPlaying() const { return mIsPlaying; }
    bool IsFading() const { return mIsFading; }
    float GetCurrentTime() const { return mCurrentTime; }
    void SetCurrentTime(float time) { mCurrentTime = time; }
    float GetDuration() const;
    const std::string& GetCurrentClipName() const;

    const std::vector<std::shared_ptr<Animation>>& GetAnimationList() const { return mAnimationList; }

    void Update(float deltaTime) override;
    void ComputeBoneWorldTransform(int boneIndex, std::vector<bool>& computed, const std::vector<Matrix4x4>& localTransforms);
    const std::vector<Matrix4x4>& GetFinalBoneMatrices() const { return mFinalBoneMatrices; }

private:
    int FindClipIndex(const std::string& name) const;
    void SetAnimationByIndex(int index);
    void SampleBoneTRS(Animation* clip, size_t boneIndex, const Bone& bone, float time,
        Vector3& outPos, Quaternion& outRot, Vector3& outScale) const;

private:
    std::shared_ptr<Skeleton> mSkeleton;
    std::vector<std::shared_ptr<Animation>> mAnimationList;

    int mCurrentAnimIndex = -1; //지금 재생 중인 클립
    float mCurrentTime = 0.0f;

    //Cross Fade용
    bool mIsFading = false;
    int mPrevAnimIndex = -1; //페이드 아웃되는 이전 클립
    float mPrevTime = 0.0f;
    float mFadeElapsed = 0.0f;
    float mFadeDuration = 0.0f;

    std::vector<Matrix4x4> mFinalBoneMatrices;
    std::vector<Matrix4x4> mBoneWorldTransforms;

    float mSpeed = 1.0f;
    bool mLoop = true;
    bool mIsPlaying = false;

    static const std::string kEmptyName;
};