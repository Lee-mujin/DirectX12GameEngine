#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"

//RenderPass 패스 목록을 순서대로 등록하고 실행하는 파이프라인 오케스트레이터
class RenderPipeline
{
public:
    RenderPipeline() = default;

    //패스 등록
    template<typename T, typename... Args>
    void AddPass(Args&&... args)
    {
        mPasses.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void ClearPasses()
    {
        mPasses.clear();
    }

    //등록된 모든 RenderPass 순차 실행
    void Execute(const RenderContext& context)
    {
        for (auto& pass : mPasses)
        {
            if (pass)
            {
                pass->Execute(context);
            }
        }
    }

private:
    std::vector<std::unique_ptr<RenderPass>> mPasses;
};