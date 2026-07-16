#pragma once
#include <memory>
#include <vector>
#include <string>
#include "Mesh.h"
#include "SkinnedMesh.h"

class Skeleton;
class Animation;

class Model
{
public:
    bool IsSkinned() const { return mSkinnedMesh != nullptr; }

    std::shared_ptr<Mesh> GetMesh() const { return mSkinnedMesh ? mSkinnedMesh : mStaticMesh; }
    std::shared_ptr<SkinnedMesh> GetSkinnedMesh() const { return mSkinnedMesh; }
    std::shared_ptr<Skeleton> GetSkeleton() const;
    const std::vector<std::shared_ptr<Animation>>& GetAnimations() const { return mAnimations; }

    void SetStaticMesh(std::shared_ptr<Mesh> mesh) { mStaticMesh = mesh; }
    void SetSkinnedMesh(std::shared_ptr<SkinnedMesh> mesh) { mSkinnedMesh = mesh; }
    void SetAnimations(std::vector<std::shared_ptr<Animation>> animations) { mAnimations = std::move(animations); }

    const std::string& GetSourcePath() const { return mSourcePath; }
    void SetSourcePath(const std::string& path) { mSourcePath = path; }

private:
    std::shared_ptr<Mesh> mStaticMesh;
    std::shared_ptr<SkinnedMesh> mSkinnedMesh;
    std::vector<std::shared_ptr<Animation>> mAnimations;
    std::string mSourcePath;
};