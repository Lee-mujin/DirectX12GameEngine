cbuffer ToneMappingCB : register(b0)
{
    float gExposure;
    int   gToneMappingType; // 0: None, 1: Reinhard, 2: ACES
    float gGamma;
    float gBloomIntensity; // 블룸 합성 강도
};

Texture2D gHDRTexture : register(t0);
Texture2D gBloomTexture : register(t1);
SamplerState gLinearClampSampler : register(s0);

struct VSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

// 정점 버퍼 없이 SV_VertexID (0, 1, 2)만으로 뷰포트를 완전히 덮는 대형 단일 삼각형 생성
VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    // vertexID 0 -> TexCoord(0, 0), Position(-1,  1)
    // vertexID 1 -> TexCoord(2, 0), Position( 3,  1)
    // vertexID 2 -> TexCoord(0, 2), Position(-1, -3)
    output.TexCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

// Narkowicz 2015 ACES Tone Mapping Fit
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Reinhard Tone Mapping
float3 Reinhard(float3 x)
{
    return x / (1.0f + x);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 hdrColor = gHDRTexture.Sample(gLinearClampSampler, input.TexCoord).rgb;
    float3 bloomColor = gBloomTexture.Sample(gLinearClampSampler, input.TexCoord).rgb;

    // 0. HDR Scene + Bloom 결합 (Tone Mapping 이전 선형 공간에서 합성)
    float3 color = hdrColor + bloomColor * max(gBloomIntensity, 0.0f);

    // 1. Exposure 적용
    color *= max(gExposure, 0.0f);

    // 2. Tone Mapping 적용
    if (gToneMappingType == 1)
    {
        color = Reinhard(color);
    }
    else if (gToneMappingType == 2)
    {
        color = ACESFilm(color);
    }
    else
    {
        // None (클램핑)
        color = saturate(color);
    }

    // 3. Gamma 보정 (Linear -> sRGB 공간 변환)
    color = pow(max(color, 0.0f), 1.0f / max(gGamma, 0.0001f));

    return float4(color, 1.0f);
}
