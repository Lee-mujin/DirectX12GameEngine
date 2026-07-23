cbuffer ObjectCB : register(b0)
{
    matrix gWorld;
    float3 gMaterialColor;
    float gShininess;
    int gHasNormalMap;
    float3 gObjectPad0;
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
    //directional light parameters
    float3 gDirLightDirection;
    float gDirLightIntensity;
    float3 gDirLightColor;
    float gLightPad0;
    //point light parameters
    float3 gPointLightPosition;
    float gPointLightIntensity;
    float3 gPointLightColor;
    float gPointLightRange;

    int gPointLightEnabled;
    float3 gLightPad1;
    //spot light parameters
    float3 gSpotLightPosition;
    float gSpotLightIntensity;
    float3 gSpotLightDirection;
    float gSpotLightRange;
    float3 gSpotLightColor;
    float gSpotLightInnerCos;
    float gSpotLightOuterCos;
    int gSpotLightEnabled;
    float2 gLightPad2;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT; // 추가
    float4 color : COLOR;
    float2 uv : TEXCOORD;
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

    float3 N = normalize(mul(input.normal, (float3x3) gWorld));
    float3 T = normalize(mul(input.tangent.xyz, (float3x3) gWorld));
    T = normalize(T - N * dot(N, T)); // Gram-Schmidt, gWorld 스케일로 인한 비직교 보정
    float3 B = cross(N, T) * input.tangent.w;

    output.normalWS = N;
    output.tangentWS = T;
    output.bitangentWS = B;
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float3 CalcSpotLight(float3 worldPos, float3 normal, float3 viewDir)
{
    if (gSpotLightEnabled == 0)
    {
        return float3(0, 0, 0);
    }

    float3 toLight = gSpotLightPosition - worldPos;
    float distance = length(toLight);
    float3 lightDir = toLight / max(distance, 0.0001f);

    // 원뿔 안쪽인지 검사
    float3 spotForward = normalize(-gSpotLightDirection);
    float cosAngle = dot(lightDir, spotForward);

    float spotFactor = saturate((cosAngle - gSpotLightOuterCos) / max(gSpotLightInnerCos - gSpotLightOuterCos, 0.0001f));

    if (spotFactor <= 0.0f)
    {
        return float3(0, 0, 0);
    }

    float attenuation = saturate(1.0f - (distance * distance) / (gSpotLightRange * gSpotLightRange));
    attenuation *= attenuation;

    float NdotL = saturate(dot(normal, lightDir));
    float3 diffuse = gSpotLightColor * gSpotLightIntensity * NdotL;

    float3 halfDir = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfDir));
    float3 specular = gSpotLightColor * pow(NdotH, gShininess) * NdotL;

    return (diffuse + specular * 0.3f) * attenuation * spotFactor;
}

float3 CalcDirLight(float3 normal, float3 viewDir)
{
    float3 lightDir = normalize(-gDirLightDirection);
    float NdotL = saturate(dot(normal, lightDir));
    float3 diffuse = gDirLightColor * gDirLightIntensity * NdotL;

    float3 halfDir = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfDir));
    float3 specular = gDirLightColor * pow(NdotH, gShininess) * NdotL;

    return diffuse + specular * 0.3f;
}

float3 CalcPointLight(float3 worldPos, float3 normal, float3 viewDir)
{
    if (gPointLightEnabled == 0)
    {
        return float3(0, 0, 0);
    }

    float3 toLight = gPointLightPosition - worldPos;
    float distance = length(toLight);
    float3 lightDir = toLight / max(distance, 0.0001f);

    float attenuation = saturate(1.0f - (distance * distance) / (gPointLightRange * gPointLightRange));
    attenuation *= attenuation;

    float NdotL = saturate(dot(normal, lightDir));
    float3 diffuse = gPointLightColor * gPointLightIntensity * NdotL;

    float3 halfDir = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfDir));
    float3 specular = gPointLightColor * pow(NdotH, gShininess) * NdotL;

    return (diffuse + specular * 0.3f) * attenuation;
}

Texture2D gNormalTexture : register(t1); // 추가 (기존 gTexture는 t0 그대로)

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    float4 baseColor = texColor * input.color * float4(gMaterialColor, 1.0f);

    float3 N = normalize(input.normalWS);

    // Normal Map이 있으면 TBN으로 world normal 교체, 없으면 정점 normal 그대로 사용
    if (gHasNormalMap)
    {
        float3 T = normalize(input.tangentWS);
        float3 B = normalize(input.bitangentWS);
        float3x3 TBN = float3x3(T, B, N);

        float3 tangentNormal = gNormalTexture.Sample(gSampler, input.uv).rgb;
        tangentNormal = tangentNormal * 2.0f - 1.0f; // 0~1 -> -1~1

        N = normalize(mul(tangentNormal, TBN));
    }

    float3 viewDir = normalize(gCameraPos - input.worldPos);

    float3 lighting = CalcDirLight(N, viewDir)
        + CalcPointLight(input.worldPos, N, viewDir)
        + CalcSpotLight(input.worldPos, N, viewDir);

    return float4(baseColor.rgb * lighting, baseColor.a);
}