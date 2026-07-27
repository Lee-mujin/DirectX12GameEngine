#include "pch.h"
#include "ShadowPass.h"
#include "D3D12Renderer.h"

void ShadowPass::Execute(const RenderContext& context)
{
    if (!context.Renderer || !context.Queue || !context.DirLight || !context.MainCamera) return;

    //섀도우 패스 시작 및 드로우
    context.Renderer->BeginShadowPass(*context.DirLight, *context.MainCamera);
    context.Renderer->RenderShadow(*context.Queue);
    context.Renderer->EndShadowPass();
}