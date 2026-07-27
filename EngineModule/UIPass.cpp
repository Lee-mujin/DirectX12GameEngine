#include "pch.h"
#include "UIPass.h"
#include "D3D12Renderer.h"

void UIPass::Execute(const RenderContext& context)
{
    if (!context.Renderer) return;

    //ImGui / Editor UI 렌더링 및 프레임 종료(Present)
    context.Renderer->EndFrame();
}