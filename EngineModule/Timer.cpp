#include "pch.h"
#include "Timer.h"

void Timer::Tick()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    if (mFrequency == 0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        mFrequency = freq.QuadPart;
        mPrevTime = now.QuadPart;
        mDeltaTime = 0.f;
        return;
    }

    __int64 elapsedTicks = now.QuadPart - mPrevTime;
    mPrevTime = now.QuadPart;

    mDeltaTime = static_cast<float>(elapsedTicks) / static_cast<float>(mFrequency);

    if (mDeltaTime > 0.1f)
    {
        mDeltaTime = 0.1f; //디버거 브레이크 등으로 프레임이 오래 멈췄을 때 튀는 것 방지
    }

    mFrameCount++;
    mFpsTimeElapsed += mDeltaTime;
    if (mFpsTimeElapsed >= 1.0f)
    {
        mFPS = static_cast<float>(mFrameCount) / mFpsTimeElapsed;
        mFrameCount = 0;
        mFpsTimeElapsed = 0.f;
    }
}