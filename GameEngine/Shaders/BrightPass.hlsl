cbuffer BrightPassCB : register(b0)
{
    float gThreshold;
    float gSoftKnee;
    float gBrightPassPad0;
    float gBrightPassPad1;
};

Texture2D gHDRTexture : register(t0);
SamplerState gLinearClampSampler : register(s0);

struct VSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

// 정점 버퍼 없이 SV_VertexID (0, 1, 2)만으로 뷰포트를 완전히 덮는 풀스크린 삼각형 생성
VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.TexCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 color = gHDRTexture.Sample(gLinearClampSampler, input.TexCoord).rgb;

    // Rec.709 휘도 (Luminance) 계산
    float luma = dot(color, float3(0.2126f, 0.7152f, 0.0722f));

    // Quadratic Soft-Knee 커브: 임계치 경계에서 깜빡임(Flickering) 없이 부드러운 하이라이트 추출
    float knee = max(gThreshold * gSoftKnee, 0.0001f);
    float soft = luma - gThreshold + knee;
    soft = clamp(soft, 0.0f, 2.0f * knee);
    soft = (soft * soft) / (4.0f * knee);

    float weight = max(soft, luma - gThreshold) / max(luma, 0.0001f);
    weight = clamp(weight, 0.0f, 1.0f);

    return float4(color * weight, 1.0f);
}
