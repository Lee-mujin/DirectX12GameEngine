#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// 엔진 전역 상수 (C++ / HLSL 공유)
//
// 이 헤더는 Cascade 개수, 드로우 슬롯, 본 개수 등 렌더러 전반에서 사용되는
// 상수를 한 곳에서 정의한다. HLSL에는 D3D12Renderer::CompileShaderInternal()이
// -D 매크로로 동일한 값을 주입한다(Default.hlsl / Shadow.hlsl 참조).
//
// 이전에는 kCascadeCount / kNumCascades / kCascadeCount(ShadowMap) / 하드코딩 3이
// 흩어져 있어 값이 어긋날 위험이 있었다.
// ---------------------------------------------------------------------------

// CSM Cascade 개수 (Near / Mid / Far)
static constexpr unsigned int kEngineCascadeCount = 3;

// 한 프레임에 그릴 수 있는 최대 "서브메시 draw" 수.
// 오브젝트 수가 아니라 RenderItem(서브메시) 단위다.
static constexpr unsigned int kEngineMaxObjectsPerFrame = 128;

// 스켈레톤이 가질 수 있는 최대 본 수 (HLSL gBoneMatrices 배열 크기와 일치)
static constexpr unsigned int kEngineMaxBones = 96;

// SRV 디스크립터 힙 크기. 모델/텍스처가 늘어나도 조용히 실패하지 않도록 충분히 크게.
static constexpr unsigned int kEngineMaxSrvCount = 1024;

// 섀도우 맵 한面(Texture2DArray 슬라이스) 해상도
static constexpr unsigned int kEngineShadowMapSize = 2048;
