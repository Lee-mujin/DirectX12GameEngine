// ==============================================================================
// GaussianBlur.hlsl: 9-Tap 수평/수직 분리 2-Pass 가우시안 블러 풀스크린 셰이더
// ==============================================================================

cbuffer BlurCB : register(b0)
{
    float2 gTexelSize;   // 1.0 / Width, 1.0 / Height
    float2 gDirection;   // (1.0, 0.0): 수평 블러, (0.0, 1.0): 수직 블러
    float  gSigma;       // 블러 반경 / 확산 계수 (기본 1.0)
    float3 gPad;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// 정점 버퍼 없이 3개의 정점 ID로 화면 전체를 덮는 풀스크린 삼각형 생성
VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

// 9-Tap 1D 가우시안 커널 가중치 (중심 1개 + 양방향 4개)
// 총 가중치 합: 0.227027 + 2 * (0.1945946 + 0.1216216 + 0.0540540 + 0.0162162) = 1.000000
static const float gWeights[5] =
{
    0.2270270f,
    0.1945946f,
    0.1216216f,
    0.0540540f,
    0.0162162f
};

float4 PSMain(VSOutput input) : SV_TARGET
{
    float2 stepOffset = gDirection * gTexelSize * max(gSigma, 0.1f);

    // 중심 픽셀 샘플링
    float3 result = gTexture.Sample(gSampler, input.uv).rgb * gWeights[0];

    // 양방향 대칭 4개 탭 샘플링 (총 9-Tap)
    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = stepOffset * (float)i;
        result += gTexture.Sample(gSampler, input.uv + offset).rgb * gWeights[i];
        result += gTexture.Sample(gSampler, input.uv - offset).rgb * gWeights[i];
    }

    return float4(result, 1.0f);
}
