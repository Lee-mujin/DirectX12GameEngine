cbuffer ObjectCB : register(b0)
{
    matrix gWorld;
};

cbuffer CameraCB : register(b1)
{
    matrix gView;
    matrix gProj;
    float3 gCameraPos;
    float gCameraPad;
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
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
    float3 normalWS : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(float4(input.position, 1.0f), gWorld);
    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, gView), gProj);
    output.normalWS = normalize(mul(input.normal, (float3x3) gWorld));
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
    float3 specular = gSpotLightColor * pow(NdotH, 32.0f) * NdotL;

    return (diffuse + specular * 0.3f) * attenuation * spotFactor;
}

float3 CalcDirLight(float3 normal, float3 viewDir)
{
    float3 lightDir = normalize(-gDirLightDirection);
    float NdotL = saturate(dot(normal, lightDir));
    float3 diffuse = gDirLightColor * gDirLightIntensity * NdotL;

    float3 halfDir = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfDir));
    float3 specular = gDirLightColor * pow(NdotH, 32.0f) * NdotL;

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
    float3 specular = gPointLightColor * pow(NdotH, 32.0f) * NdotL;

    return (diffuse + specular * 0.3f) * attenuation;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    float4 baseColor = texColor * input.color;

    float3 normal = normalize(input.normalWS);
    float3 viewDir = normalize(gCameraPos - input.worldPos);

    float3 lighting = CalcDirLight(normal, viewDir)
    + CalcPointLight(input.worldPos, normal, viewDir)
    + CalcSpotLight(input.worldPos, normal, viewDir);

    return float4(baseColor.rgb * lighting, baseColor.a);
}