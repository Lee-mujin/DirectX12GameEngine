#pragma once
#include "Mesh.h"
#include <memory>

class Skeleton; 

class SkinnedMesh : public Mesh //정점/인덱스 버퍼 생성 로직을 재사용하고 Skeleton 참조만 추가
{
public:
    std::shared_ptr<Skeleton> GetSkeleton() const { return mSkeleton; }
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton) { mSkeleton = skeleton; }

private:
    std::shared_ptr<Skeleton> mSkeleton;
};