#pragma once
#include <d3d12.h>

class Viewport
{
public:
    void Draw(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, float& outWidth, float& outHeight, float& outPosX, float& outPosY);
};