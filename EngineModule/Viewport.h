#pragma once
#include <d3d12.h>

class EditorState;

class Viewport
{
public:
    void Draw(EditorState& editorState, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, float& outWidth, float& outHeight, float& outPosX, float& outPosY);
};