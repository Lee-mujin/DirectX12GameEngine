#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>
#include <string>
#include <commdlg.h>
#include "d3dx12.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Mesh.h"
#include "Material.h"
#include "CameraComponent.h"
#include "MeshRenderer.h"
#include "RotatorComponent.h"
#include "AnimatorComponent.h"
#include "GameObject.h"

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("DX12 call failed");
    }
}