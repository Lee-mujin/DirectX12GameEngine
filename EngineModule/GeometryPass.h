#pragma once
#include "RenderPass.h"

class GeometryPass : public RenderPass
{
public:
    void Execute(const RenderContext& context) override;
};