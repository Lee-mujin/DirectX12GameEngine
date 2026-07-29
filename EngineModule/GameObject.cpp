#include "pch.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "SkinnedMesh.h"
#include <algorithm>

void GameObject::Update(float deltaTime)
{
    for (auto& c : mComponents)
    {
        c->Update(deltaTime);
    }
}

void GameObject::Render(D3D12Renderer& renderer)
{
    for (auto& c : mComponents)
    {
        c->Render(renderer);
    }
}

void GameObject::RenderShadow(D3D12Renderer& renderer)
{
    for (auto& c : mComponents)
    {
        c->RenderShadow(renderer);
    }
}

BoundingBox GameObject::GetWorldBoundingBox() const
{
    GameObject* nonConstThis = const_cast<GameObject*>(this);
    MeshRenderer* meshRenderer = nonConstThis->GetComponent<MeshRenderer>();

    BoundingBox worldBox;
    bool hasValidMeshBox = false;

    if (meshRenderer)
    {
        const Mesh* targetMesh = nullptr;
        if (auto mesh = meshRenderer->GetMesh())
            targetMesh = mesh.get();
        else if (auto skinnedMesh = meshRenderer->GetSkinnedMesh())
            targetMesh = skinnedMesh.get();

        if (targetMesh)
        {
            BoundingBox localBox = targetMesh->GetBoundingBox();

            //Local Box가 유효할 때 변환
            if (localBox.IsValid())
            {
                worldBox = localBox.Transform(mTransform.GetMatrix());
                hasValidMeshBox = true;
            }
        }
    }

    //메쉬가 없거나 localBox가 0/무효하여 체크에 안 걸린 경우:
    //현재 Transform Matrix 기준 기본 박스를 가져옴
    if (!hasValidMeshBox)
    {
        Matrix4x4 worldMat = mTransform.GetMatrix();
        BoundingBox defaultBox(Vector3(-0.5f, 0.0f, -0.5f), Vector3(0.5f, 2.0f, 0.5f));
        worldBox = defaultBox.Transform(worldMat);
    }

    //Min.Y을 기준점으로 잡고 위쪽으로만 1.8 영역 확보
    Vector3 minPos = worldBox.Min;
    Vector3 maxPos = worldBox.Max;

    float currentHeight = maxPos.Y - minPos.Y;
    if (currentHeight < 0.5f)
    {
        Vector3 center = worldBox.GetCenter();
        float widthX = (std::max)(maxPos.X - minPos.X, 0.8f);
        float depthZ = (std::max)(maxPos.Z - minPos.Z, 0.8f);
        float targetHeight = 1.8f; //높이 1.8

        return BoundingBox(
            Vector3(center.X - widthX * 0.5f, minPos.Y, center.Z - depthZ * 0.5f),
            Vector3(center.X + widthX * 0.5f, minPos.Y + targetHeight, center.Z + depthZ * 0.5f)
        );
    }

    return worldBox;
}