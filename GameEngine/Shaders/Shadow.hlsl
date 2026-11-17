cbuffer ShadowCB : register(b0)
{
    matrix gWorldLightViewProj;
};

// C++에서 -D 매크로로 주입한다(kEngineMaxBones). 단독 컴파일용 폴백.
#ifndef MAX_BONES
#define MAX_BONES 96
#endif
cbuffer BoneCB : register(b1)
{
    matrix gBoneMatrices[MAX_BONES];
};

// Input Layout 시맨틱과 동일하게 매칭되는 구조체 정의
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct SkinnedVSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    uint4 boneIndices : BONEINDICES;
    float4 boneWeights : BONEWEIGHTS;
};

// Static Mesh용 Shadow Vertex Shader (Entry point: VSMain)
float4 VSMain(VSInput input) : SV_POSITION
{
    return mul(float4(input.position, 1.0f), gWorldLightViewProj);
}

// Skinned Mesh용 Shadow Vertex Shader (Entry point: VSMain_Skinned)
float4 VSMain_Skinned(SkinnedVSInput input) : SV_POSITION
{
    matrix skinMatrix =
        gBoneMatrices[input.boneIndices.x] * input.boneWeights.x +
        gBoneMatrices[input.boneIndices.y] * input.boneWeights.y +
        gBoneMatrices[input.boneIndices.z] * input.boneWeights.z +
        gBoneMatrices[input.boneIndices.w] * input.boneWeights.w;

    float4 skinnedPos = mul(float4(input.position, 1.0f), skinMatrix);
    return mul(skinnedPos, gWorldLightViewProj);
}