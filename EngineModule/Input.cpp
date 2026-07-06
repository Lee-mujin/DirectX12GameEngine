// Input.cpp
#include "pch.h"
#include "Input.h"

void Input::Update()
{
    memcpy(mPrevKeys, mCurrKeys, sizeof(mCurrKeys));
    memcpy(mPrevMouseButtons, mCurrMouseButtons, sizeof(mCurrMouseButtons));
    mPrevMouseX = mMouseX;
    mPrevMouseY = mMouseY;
}

void Input::OnKeyDown(WPARAM key)
{
    if (key < kKeyCount)
    {
        mCurrKeys[key] = true;
    }
}

void Input::OnKeyUp(WPARAM key)
{
    if (key < kKeyCount)
    {
        mCurrKeys[key] = false;
    }
}

void Input::OnMouseMove(int x, int y)
{
    mMouseX = x;
    mMouseY = y;
}

void Input::OnMouseDown(int button)
{
    if (button < kMouseButtonCount)
    {
        mCurrMouseButtons[button] = true;
    }
}

void Input::OnMouseUp(int button)
{
    if (button < kMouseButtonCount)
    {
        mCurrMouseButtons[button] = false;
    }
}

bool Input::IsKeyDown(int key) const
{
    return mCurrKeys[key];
}

bool Input::IsKeyPressed(int key) const
{
    return mCurrKeys[key] && !mPrevKeys[key];
}

bool Input::IsKeyReleased(int key) const
{
    return !mCurrKeys[key] && mPrevKeys[key];
}

bool Input::IsMouseDown(int button) const
{
    return mCurrMouseButtons[button];
}

bool Input::IsMousePressed(int button) const
{
    return mCurrMouseButtons[button] && !mPrevMouseButtons[button];
}