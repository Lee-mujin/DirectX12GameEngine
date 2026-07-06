#pragma once

class Timer
{
public:
    void Tick();

    float DeltaTime() const { return mDeltaTime; }
    float FPS() const { return mFPS; }

private:
    __int64 mFrequency = 0;
    __int64 mPrevTime = 0;

    float mDeltaTime = 0.f;
    float mFPS = 0.f;

    float mFpsTimeElapsed = 0.f;
    int mFrameCount = 0;
};