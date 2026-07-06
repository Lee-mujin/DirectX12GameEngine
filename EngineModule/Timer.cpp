#include "pch.h"
#include "Timer.h"

void Timer::Tick()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    if (mFrequency == 0) //타이머 호출
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        mFrequency = freq.QuadPart;
        mPrevTime = now.QuadPart; //이전 시간
        mDeltaTime = 0.0f; //경과
        return;
    }

    __int64 elapsedTicks = now.QuadPart - mPrevTime;
    mPrevTime = now.QuadPart;

    mDeltaTime = static_cast<float>(elapsedTicks) / static_cast<float>(mFrequency); //프레임 계산

    if (mDeltaTime > 0.1f) { mDeltaTime = 0.1f; } //0.1초로 고정해 디버거 브레이크 등으로 프레임이 튀는 것을 방지

    mFrameCount++;
    mFpsTimeElapsed += mDeltaTime; //프레임 시간 누적

    if (mFpsTimeElapsed >= 1.0f)
    {
        mFPS = static_cast<float>(mFrameCount) / mFpsTimeElapsed; //FPS를 갱신하고 카운터 초기화
        mFrameCount = 0;
        mFpsTimeElapsed = 0.0f;
    }
}