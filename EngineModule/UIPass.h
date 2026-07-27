#pragma once
#include "RenderPass.h"

class UIPass : public RenderPass
{
public:
    void Execute(const RenderContext& context) override;
};