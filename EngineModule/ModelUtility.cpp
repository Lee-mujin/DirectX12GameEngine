#include "pch.h"
#include "ModelUtility.h"
#include "GameObject.h"
#include "Model.h"
#include "MeshRenderer.h"
#include "AnimatorComponent.h"
#include "Material.h"

void ModelUtility::ReplaceModel(GameObject& targetObject, std::shared_ptr<Model> model, AssetHandle sourceHandle)
{
    if (!model) return;

    // 1. SourceAssetHandle 저장 (에셋 핫 리로드 및 씬 저장/복원용)
    if (sourceHandle.IsValid())
    {
        targetObject.SetSourceAssetHandle(sourceHandle);
    }

    MeshRenderer* mr = targetObject.GetComponent<MeshRenderer>();
    if (!mr) mr = targetObject.AddComponent<MeshRenderer>();

    // 2. Mesh 교체
    mr->SetMesh(model->GetMesh());

    // 3. 이전 모델의 Material이 남지 않도록 새 기본 Material 생성 후 할당
    auto defaultMaterial = std::make_shared<Material>();
    mr->SetMaterial(defaultMaterial);

    // 4. Skinned / Static 컴포넌트 동기화
    if (model->IsSkinned())
    {
        AnimatorComponent* animator = targetObject.GetComponent<AnimatorComponent>();
        if (!animator) animator = targetObject.AddComponent<AnimatorComponent>();

        animator->SetSkeleton(model->GetSkeleton());
        animator->SetAnimationList(model->GetAnimations());
    }
    else
    {
        // Static 모델로 교체 시 불필요한 Animator 제거
        targetObject.RemoveComponent<AnimatorComponent>();
    }
}