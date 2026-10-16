#include "pch.h"
#include "GeometryPass.h"
#include "D3D12Renderer.h"

void GeometryPass::Execute(const RenderContext& context)
{
    if (!context.Renderer || !context.Queue || !context.MainCamera || !context.DirLight) return;

    //메인 지오메트리 패스 프레임 시작, 드로우 및 타겟 전이
    context.Renderer->BeginFrame(*context.MainCamera, *context.DirLight, *context.PointLight, *context.SpotLight);
    context.Renderer->Render(*context.Queue);
    context.Renderer->ExecuteBrightPass();
    context.Renderer->ExecuteGaussianBlur();
    context.Renderer->ExecuteToneMappingPass();
    context.Renderer->TransitionToSwapChain();
}