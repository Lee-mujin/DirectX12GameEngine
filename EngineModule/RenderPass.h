#pragma once
#include <d3d12.h>
#include "RenderQueue.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

class D3D12Renderer;

//RenderPass 실행 시 필요한 프레임 컨텍스트 정보
struct RenderContext
{
    D3D12Renderer* Renderer = nullptr;
    const RenderQueue* Queue = nullptr;
    const Camera* MainCamera = nullptr;
    const DirectionalLight* DirLight = nullptr;
    const PointLight* PointLight = nullptr;
    const SpotLight* SpotLight = nullptr;
};

//렌더 패스 추상 인터페이스
class RenderPass
{
public:
    virtual ~RenderPass() = default;
    virtual void Execute(const RenderContext& context) = 0;
};