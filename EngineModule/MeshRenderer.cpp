// MeshRenderer.cpp
#include "pch.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "D3D12Renderer.h"

void MeshRenderer::Render(D3D12Renderer& renderer)
{
    if (!mMesh || !mMaterial)
    {
        return;
    }

    renderer.DrawMesh(*mMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix());
}