#pragma once
#include "RenderPass.h"

class ShadowPass : public RenderPass
{
public:
    void Execute(const RenderContext& context) override;
};