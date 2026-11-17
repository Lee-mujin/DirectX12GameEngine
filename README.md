# DirectX 12 Game Engine

DirectX 12 기반의 소규모 게임 엔진 + 에디터 데모입니다. skinned mesh 애니메이션, CSM(3-cascade) 그림자, PBR 조명, Bloom/ACES 톤매핑, ImGui 기반 씬 에디터를 포함합니다.

## 요구 사항

- Windows 10/11 x64
- Visual Studio 2022 이상 (C++20, v143+) — `.sln`과 `.slnx` 모두 제공
- Windows 10 SDK
- [vcpkg](https://github.com/microsoft/vcpkg) — Assimp

## 빌드

### 1) VCPKG_ROOT 환경변수 설정

vcpkg를 `C:\vcpkg`에 설치했다면(이 머신의 실제 경로):

```powershell
# 현재 PowerShell 세션에만 적용
$env:VCPKG_ROOT = "C:\vcpkg"

# 사용자 환경변수로 영구 저장 (새 터미널/VS에서 적용됨)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
```

Windows GUI: `시작 → "환경 변수 편집" → 사용자 변수 새로 만들기 → 이름 VCPKG_ROOT / 값 C:\vcpkg`
설정 후 **Visual Studio를 완전히 재시작**해야 합니다(VS가 시작 시 환경변수를 읽습니다).

### 2) Assimp 설치

```powershell
cd C:\vcpkg
.\vcpkg.exe install assimp:x64-windows
```

`Directory.Build.props`가 `$(VCPKG_ROOT)\installed\x64-windows`를 기본 경로로 사용합니다.

**대안 (manifest 모드):** vcpkg.json 기반 자동 설치를 원하면 vcpkg MSBuild 통합(`vcpkg integrate install`)이 필요하며, 빌드 시 `/p:UseVcpkgManifest=true`를 넘깁니다. 이 경우 경로는 `vcpkg_installed\x64-windows`가 됩니다.

다른 경로에 설치했다면 속성을 직접 지정할 수 있습니다:

```powershell
msbuild DirectX12GameEngine.sln /p:AssimpRoot=C:\path\to\assimp ...
```

> lib 이름은 툴셋에서 자동 계산됩니다(v145 → `assimp-vc145-mt.lib`). 다른 triplet이면 `AssimpLibDebug` / `AssimpLibRelease`를 재정의하세요.

### 3) 빌드

`DirectX12GameEngine.sln`을 열고 `Debug|x64` 또는 `Release|x64`로 빌드합니다.
빌드 결과와 무관하게 실행 파일은 `GameEngine/x64/{Debug|Release}/GameEngine.exe`입니다.

### 실행 시 작업 디렉터리

셰이더(`Shaders/`)와 에셋(`Assets/`)은 상대 경로로 로드합니다.
빌드 후 **프로젝트 루트(또는 `GameEngine/`)**에서 실행해야 합니다.

## 주요 기능

- 렌더링: deferred-style offscreen HDR target + BrightPass + Gaussian Blur + Tone Mapping
- 섀도우: 3-cascade CSM (Texture2DArray), skinned mesh 그림자 지원
- 애니메이션: Assimp 기반 본/애니메이션 로드, cross-fade
- 에디터: Hierarchy / Inspector / Content Browser / Light panel / Viewport Gizmo
- 리소스: 에셋 핸들 DB, 모델/텍스처 캐시, 전용 Copy Queue 업로드
- 씬 저장/불러오기: `Assets/Scenes/scene.txt`

## 조작

| 입력 | 동작 |
|---|---|
| 마우스 우클릭 드래그 | 카메라 회전 |
| W / A / S / D | 이동 |
| Q / E | 하강 / 상승 |
| Content Browser 더블클릭 / 드래그 | 모델을 씬에 스폰 |
| Inspector 텍스처 드롭 | Base Color / Normal Map 교체 |

## 알려진 제한

- 씬 직렬화는 **에셋 오브젝트의 TRS와 이름만** 저장합니다. 카메라/라이트/머티리얼 오버라이드/애니메이션 상태는 저장되지 않습니다.
- 스켈레톤은 최대 96본(`kEngineMaxBones`)까지 지원합니다. 초과 모델은 로드가 거부됩니다.
- 프러스텀 컬링은 오브젝트 단위(메시 전체 AABB)입니다.
- `Assets/Models/*.glb`는 용량이 큽니다(약 62MB). 배포 시 Git LFS 사용을 권장합니다.

## 서드파티

- [Assimp](https://github.com/assimp/assimp) — BSD-3-Clause
- [Dear ImGui](https://github.com/ocornut/imgui) — MIT
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) — MIT
- [DirectXMath / d3dx12](https://github.com/microsoft/DirectX-Graphics-Samples) — MIT

이 프로젝트는 학습/포트폴리오용이며 별도 라이선스는 아직 지정되지 않았습니다.
