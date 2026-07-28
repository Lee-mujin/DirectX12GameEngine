#include "pch.h"
#include "ShadowPass.h"
#include "D3D12Renderer.h"

void ShadowPass::Execute(const RenderContext& context)
{
    if (!context.Renderer || !context.Queue || !context.DirLight || !context.MainCamera) return;

    //3개의 Cascade Layer를 순회하며 섀도우 맵 생성
    for (UINT i = 0; i < 3; ++i)
    {
        context.Renderer->BeginShadowPass(*context.DirLight, *context.MainCamera, i);
        context.Renderer->RenderShadow(*context.Queue);
    }

    context.Renderer->EndShadowPass();
}