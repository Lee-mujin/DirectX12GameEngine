#include "pch.h"
#include "Model.h"
#include "SkinnedMesh.h"

std::shared_ptr<Skeleton> Model::GetSkeleton() const
{
    return mSkinnedMesh ? mSkinnedMesh->GetSkeleton() : nullptr;
}