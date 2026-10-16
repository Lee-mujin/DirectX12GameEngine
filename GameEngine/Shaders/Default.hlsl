cbuffer ObjectCB : register(b0)
{
    matrix gWorld;
    float3 gMaterialColor;
    float gShininess;
    float gMetallic;
    float gRoughness;
    int gHasNormalMap;
    float gObjectPad0;
};

cbuffer CameraCB : register(b1)
{
    matrix gView;
    matrix gProj;
    float3 gCameraPos;
    float gCameraPad;
};

#define MAX_BONES 96
cbuffer BoneCB : register(b3)
{
    matrix gBoneMatrices[MAX_BONES];
};

cbuffer LightCB : register(b2)
{
    // directional light parameters
    float3 gDirLightDirection;
    float gDirLightIntensity;
    float3 gDirLightColor;
    float gLightPad0;
    
    // Light Space 변환 행렬 (Directional Light 전용)
    // C++ LightCBData 규격(WorldLightViewProj[3])에 맞춰 3개 캐스케이드 행렬 및 분할 거리 추가
    matrix gWorldLightViewProj[3];
    float4 gCascadeSplits; // x: Near, y: Mid, z: Far

    // point light parameters
    float3 gPointLightPosition;
    float gPointLightIntensity;
    float3 gPointLightColor;
    float gPointLightRange;

    int gPointLightEnabled;
    float3 gLightPad1;

    // spot light parameters
    float3 gSpotLightPosition;
    float gSpotLightIntensity;
    float3 gSpotLightDirection;
    float gSpotLightRange;
    float3 gSpotLightColor;
    float gSpotLightInnerCos;
    float gSpotLightOuterCos;
    int gSpotLightEnabled;
    
    // Spot Light 전용 View-Projection 변환 행렬 (LightCBData의 SpotLightViewProj와 1:1 매칭)
    matrix gSpotLightViewProj;
};

Texture2D gTexture : register(t0);
Texture2D gNormalTexture : register(t1);
//CSM 3-Layer 지원을 위해 Texture2DArray로 변경
Texture2DArray gShadowMap : register(t2);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

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

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
    float3 normalWS : NORMAL;
    float3 tangentWS : TANGENT;
    float3 bitangentWS : BINORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float4 shadowPosH : TEXCOORD2;
    float4 spotShadowPosH : TEXCOORD3; // Spot Light용 Light Space 클립 좌표
};

PSInput VSMain_Skinned(SkinnedVSInput input)
{
    PSInput output;

    matrix skinMatrix =
        gBoneMatrices[input.boneIndices.x] * input.boneWeights.x +
        gBoneMatrices[input.boneIndices.y] * input.boneWeights.y +
        gBoneMatrices[input.boneIndices.z] * input.boneWeights.z +
        gBoneMatrices[input.boneIndices.w] * input.boneWeights.w;

    float4 skinnedPos = mul(float4(input.position, 1.0f), skinMatrix);
    float3 skinnedNormal = mul(input.normal, (float3x3) skinMatrix);
    float3 skinnedTangent = mul(input.tangent.xyz, (float3x3) skinMatrix);

    float4 worldPos = mul(skinnedPos, gWorld);
    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, gView), gProj);

    // 0번 캐스케이드(근거리) 기준 초기 변환 값
    output.shadowPosH = mul(worldPos, gWorldLightViewProj[0]);
    output.spotShadowPosH = mul(worldPos, gSpotLightViewProj); // Spot Light 좌표 변환

    float3 N = normalize(mul(skinnedNormal, (float3x3) gWorld));
    float3 T = normalize(mul(skinnedTangent, (float3x3) gWorld));
    T = normalize(T - N * dot(N, T));
    float3 B = cross(N, T) * input.tangent.w;

    output.normalWS = N;
    output.tangentWS = T;
    output.bitangentWS = B;
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(float4(input.position, 1.0f), gWorld);
    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, gView), gProj);

    // 0번 캐스케이드(근거리) 기준 초기 변환 값
    output.shadowPosH = mul(worldPos, gWorldLightViewProj[0]);
    output.spotShadowPosH = mul(worldPos, gSpotLightViewProj); // Spot Light 좌표 변환

    float3 N = normalize(mul(input.normal, (float3x3) gWorld));
    float3 T = normalize(mul(input.tangent.xyz, (float3x3) gWorld));
    T = normalize(T - N * dot(N, T));
    float3 B = cross(N, T) * input.tangent.w;

    output.normalWS = N;
    output.tangentWS = T;
    output.bitangentWS = B;
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

// Bias & Resolution 대응 Shadow Factor 계산 함수 (SpotLight 등 single-slice용 겸용)
float CalculateShadowFactor(float4 shadowPosH, float3 normalWS, float3 lightDir)
{
    // 광원 뒤쪽에 있는 메시나 clipped 좌표의 divide-by-zero 및 좌표 뒤집힘 방지
    if (shadowPosH.w <= 0.0f)
    {
        return 1.0f;
    }

    // 원근 제형(Perspective Divide) 및 NDC 변환
    shadowPosH.xyz /= shadowPosH.w;

    // UV 좌표 변환 (-1~1 -> 0~1), Y축 반전
    float2 shadowUV = shadowPosH.xy * 0.5f + 0.5f;
    shadowUV.y = 1.0f - shadowUV.y;

    float depth = shadowPosH.z;

    // 섀도우 맵 범위 밖인 경우 그림자 미적용
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f || depth > 1.0f)
    {
        return 1.0f;
    }

    // Slope-based Dynamic Depth Bias 계산
    float NdotL = saturate(dot(normalWS, lightDir));
    float bias = max(0.0005f, 0.005f * (1.0f - NdotL));

    // 섀도우 맵 해상도 자동 추출 (Texture2DArray 규격)
    float width, height, elements;
    gShadowMap.GetDimensions(width, height, elements);
    float2 texelSize = float2(1.0f / width, 1.0f / height);

    // 5x5 PCF 샘플링 (25번 비교로 외곽선 부드럽게 무화, 0번 슬라이스 기준)
    float percentLit = 0.0f;
    [unroll]
    for (int x = -2; x <= 2; ++x)
    {
        [unroll]
        for (int y = -2; y <= 2; ++y)
        {
            percentLit += gShadowMap.SampleCmpLevelZero(
                gShadowSampler,
                float3(shadowUV + float2(x, y) * texelSize, 0.0f),
                depth - bias
            );
        }
    }
    return percentLit / 25.0f; // 5x5 샘플링 개수(25개)에 맞춘 평균값 계산
}

// View Depth 기반 Cascade 인덱스 자동 선택 및 Texture2DArray 샘플링
float CalculateCSMShadowFactor(float3 worldPos, float3 normalWS, float3 lightDir)
{
    // 1. 카메라 View Space의 Depth(Z) 계산
    float4 viewPos = mul(float4(worldPos, 1.0f), gView);
    float depthZ = viewPos.z;

    // 2. View Depth를 Cascade 분할 거리(x: Near, y: Mid, z: Far)와 비교하여 적절한 Cascade 선택
    int cascadeIndex = 0;
    if (depthZ > gCascadeSplits.x)
    {
        cascadeIndex = 1;
    }
    if (depthZ > gCascadeSplits.y)
    {
        cascadeIndex = 2;
    }

    // 3. 선택된 Cascade의 Light ViewProj 행렬로 좌표 변환
    float4 shadowPosH = mul(float4(worldPos, 1.0f), gWorldLightViewProj[cascadeIndex]);

    if (shadowPosH.w <= 0.0f)
    {
        return 1.0f;
    }

    shadowPosH.xyz /= shadowPosH.w;

    float2 shadowUV = shadowPosH.xy * 0.5f + 0.5f;
    shadowUV.y = 1.0f - shadowUV.y;

    float depth = shadowPosH.z;

    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f || depth > 1.0f)
    {
        return 1.0f;
    }

    float NdotL = saturate(dot(normalWS, lightDir));
    float bias = max(0.0005f, 0.005f * (1.0f - NdotL));

    float width, height, elements;
    gShadowMap.GetDimensions(width, height, elements);
    float2 texelSize = float2(1.0f / width, 1.0f / height);

    //Texture2DArray용 float3(UV, cascadeIndex) 전달
    float percentLit = 0.0f;
    [unroll]
    for (int x = -2; x <= 2; ++x)
    {
        [unroll]
        for (int y = -2; y <= 2; ++y)
        {
            float3 sampleUV = float3(shadowUV + float2(x, y) * texelSize, (float) cascadeIndex);
            percentLit += gShadowMap.SampleCmpLevelZero(
                gShadowSampler,
                sampleUV,
                depth - bias
            );
        }
    }
    return percentLit / 25.0f;
}

static const float PI = 3.14159265359f;

// 1. Trowbridge-Reitz GGX Normal Distribution Function (NDF)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001f);
}

// 2. Schlick-GGX Geometric Shadowing Function
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float denom = NdotV * (1.0f - k) + k;
    return NdotV / max(denom, 0.0000001f);
}

// Smith Model for Geometry Shadowing
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. Fresnel-Schlick Approximation
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// Cook-Torrance Specular + Lambertian Diffuse BRDF Evaluator
float3 EvaluatePBR(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0)
{
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);

    if (NdotL <= 0.0f || NdotV <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 H = normalize(V + L);

    // Cook-Torrance Specular BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.0001f;
    float3 specular = numerator / denominator;

    // Energy conservation: kD (diffuse) + kS (specular) = 1
    float3 kS = F;
    float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1.0f - metallic);

    // Outgoing radiance
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float3 CalcDirLight(float3 worldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float3 L = normalize(-gDirLightDirection);
    float3 radiance = gDirLightColor * gDirLightIntensity;

    float shadowFactor = CalculateCSMShadowFactor(worldPos, N, L);

    return EvaluatePBR(N, V, L, radiance, albedo, metallic, roughness, F0) * shadowFactor;
}

float3 CalcPointLight(float3 worldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    if (gPointLightEnabled == 0)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 toLight = gPointLightPosition - worldPos;
    float distance = length(toLight);
    float3 L = toLight / max(distance, 0.0001f);

    float attenuation = saturate(1.0f - (distance * distance) / (gPointLightRange * gPointLightRange));
    attenuation *= attenuation;

    float3 radiance = gPointLightColor * gPointLightIntensity * attenuation;

    return EvaluatePBR(N, V, L, radiance, albedo, metallic, roughness, F0);
}

float3 CalcSpotLight(float3 worldPos, float3 N, float3 V, float4 spotShadowPosH, float3 albedo, float metallic, float roughness, float3 F0)
{
    if (gSpotLightEnabled == 0)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 toLight = gSpotLightPosition - worldPos;
    float distance = length(toLight);
    float3 L = toLight / max(distance, 0.0001f);

    float3 spotForward = normalize(-gSpotLightDirection);
    float cosAngle = dot(L, spotForward);

    float spotFactor = saturate((cosAngle - gSpotLightOuterCos) / max(gSpotLightInnerCos - gSpotLightOuterCos, 0.0001f));
    if (spotFactor <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float attenuation = saturate(1.0f - (distance * distance) / (gSpotLightRange * gSpotLightRange));
    attenuation *= attenuation;

    float3 radiance = gSpotLightColor * gSpotLightIntensity * attenuation * spotFactor;

    // Spot Light는 전용 섀도우 맵 패스 추가 시 shadowFactor 연동 가능
    float shadowFactor = 1.0f;

    return EvaluatePBR(N, V, L, radiance, albedo, metallic, roughness, F0) * shadowFactor;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    float4 baseColor = texColor * input.color * float4(gMaterialColor, 1.0f);

    float3 N = normalize(input.normalWS);

    if (gHasNormalMap)
    {
        float3 T = normalize(input.tangentWS);
        float3 B = normalize(input.bitangentWS);
        float3x3 TBN = float3x3(T, B, N);

        float3 tangentNormal = gNormalTexture.Sample(gSampler, input.uv).rgb;
        tangentNormal = tangentNormal * 2.0f - 1.0f;

        N = normalize(mul(tangentNormal, TBN));
    }

    float3 V = normalize(gCameraPos - input.worldPos);

    // PBR 머티리얼 파라미터 유효 범위 보정
    float roughness = clamp(gRoughness, 0.05f, 1.0f);
    float metallic = saturate(gMetallic);
    float3 albedo = baseColor.rgb;

    // 기본 반사율 F0: 절연체는 0.04, 도체(금속)는 Albedo 색상 사용
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    // 직사광 PBR 누적 계산
    float3 Lo = CalcDirLight(input.worldPos, N, V, albedo, metallic, roughness, F0)
        + CalcPointLight(input.worldPos, N, V, albedo, metallic, roughness, F0)
        + CalcSpotLight(input.worldPos, N, V, input.spotShadowPosH, albedo, metallic, roughness, F0);

    // 기본 환경광 (IBL 도입 전까지 완전 암전 방지용 보조광)
    float3 ambient = float3(0.04f, 0.04f, 0.04f) * albedo;

    float3 finalColor = ambient + Lo;

    return float4(finalColor, baseColor.a);
}