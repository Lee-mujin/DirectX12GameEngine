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
#include <cstdio>
#include <cstdlib>
#include <comdef.h>
#include <commdlg.h>
#include <algorithm>
#include <functional>
#include <filesystem>
#include "d3dx12.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Mesh.h"
#include "Material.h"
#include "CameraComponent.h"
#include "MeshRenderer.h"
#include "RotatorComponent.h"
#include "AnimatorComponent.h"
#include "GameObject.h"

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailedImpl(HRESULT hr, const char* file, int line)
{
    if (FAILED(hr))
    {
        _com_error comError(hr);
        const wchar_t* wide = comError.ErrorMessage();
        char message[512] = {};
        size_t converted = 0;
        if (wide)
        {
            wcstombs_s(&converted, message, sizeof(message), wide, sizeof(message) - 1);
        }
        else
        {
            strncpy_s(message, sizeof(message) - 1, "Unknown HRESULT", sizeof(message) - 1);
        }

        char detail[768] = {};
        sprintf_s(detail, "DX12 call failed: hr=0x%08X (%s) at %s:%d\n",
            static_cast<unsigned int>(hr), message, file, line);
        OutputDebugStringA(detail);
        throw std::runtime_error(detail);
    }
}

// 파일/라인 정보를 자동으로 포함한다.
#define ThrowIfFailed(hr) ThrowIfFailedImpl((hr), __FILE__, __LINE__)