// AnimatorComponent.h
#pragma once
#include <memory>
#include <vector>
#include <string>
#include "Component.h"
#include "Matrix4x4.h"

class SkinnedMesh;
class Skeleton;
class Animation;

class AnimatorComponent : public Component
{
public:
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetAnimationList(std::vector<std::shared_ptr<Animation>> animations);

    void Play(const std::string& clipName); // 이름으로 클립 전환 후 재생
    void Play();                            // 현재 클립 이어서 재생
    void Resume();
    void Pause();
    void Stop();

    void SetSpeed(float speed) { mSpeed = speed; }
    float GetSpeed() const { return mSpeed; }

    void SetLoop(bool loop) { mLoop = loop; }
    bool IsLooping() const { return mLoop; }

    bool IsPlaying() const { return mIsPlaying; }
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

private:
    std::shared_ptr<Skeleton> mSkeleton;
    std::vector<std::shared_ptr<Animation>> mAnimationList;
    int mCurrentAnimIndex = -1;

    std::vector<Matrix4x4> mFinalBoneMatrices; //셰이더에 전달할 최종 스킨닝 팔레트
    std::vector<Matrix4x4> mBoneWorldTransforms; //계층 누적용 임시 버퍼

    float mCurrentTime = 0.0f;
    float mSpeed = 1.0f;
    bool mLoop = true;
    bool mIsPlaying = false;

    static const std::string kEmptyName;
};