#include "LucyCommons.slang"

struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
    [[vk::location(1)]] float3 a_ID : ID;
    [[vk::location(2)]] float2 a_TextureCoords : TEXCOORD0;
    [[vk::location(3)]] float3 a_Normals : NORMAL;
    [[vk::location(4)]] float3 a_Tangents : TANGENT;
    [[vk::location(5)]] float3 a_Bitangents : BITANGENT;
};

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_PosOutWS : POSITION0;
    [[vk::location(1)]] float2 a_TextureCoordsOut : TEXCOORD0;
    [[vk::location(2)]] float3 a_NormalsWSOut : NORMAL;
    [[vk::location(3)]] float4 a_TangentWSOut : TANGENT0;
    [[vk::location(4)]] float a_ViewDepthOut : TEXCOORD1;
};

struct LucyCamera {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LucyCamera> Camera;

struct LocalPushConstant {
    float4x4 u_ModelMatrix;
    float4x4 u_ModelMatrixInversedTransposed;
    float u_MaterialID;
};

[[vk::push_constant]]
LocalPushConstant PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    float4 worldPos = mul(PushConstants.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float4 viewPos = mul(Camera.u_ViewMatrix, worldPos);

    float3x3 normalMatrix = float3x3(PushConstants.u_ModelMatrixInversedTransposed);

    float3 N = normalize(mul(normalMatrix, input.a_Normals));
    float3 T = normalize(mul(normalMatrix, input.a_Tangents));
    float3 B = normalize(mul(normalMatrix, input.a_Bitangents));

    // Make tangent perpendicular to the normal.
    T = normalize(T - N * dot(N, T));

    /* need to do that */
    float handedness = (dot(cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;

    VertexOutput output;
    output.a_Pos = mul(Camera.u_ProjMatrix, viewPos);
    output.a_PosOutWS = worldPos.xyz;
    output.a_TextureCoordsOut = input.a_TextureCoords;
    output.a_TangentWSOut = float4(T, handedness);
    output.a_ViewDepthOut = viewPos.z;
    output.a_NormalsWSOut = N;

    return output;
}

#include "LucyCommons.slang"

struct PixelInput {
    [[vk::location(0)]] float3 a_PosOutWS : POSITION0;
    [[vk::location(1)]] float2 a_TextureCoordsOut : TEXCOORD0;
    [[vk::location(2)]] float3 a_NormalsWSOut : NORMAL;
    [[vk::location(3)]] float4 a_TangentWSOut : TANGENT0;
    [[vk::location(4)]] float a_ViewDepthOut : TEXCOORD1;
};

struct PixelOutput {
    [[vk::location(0)]] float4 a_Color : SV_Target0;
};

static const int NULL_TEXTURE_SLOT = -1;

static const uint ROUGHNESS_MASK = 0x00000001u;
static const uint METALLIC_MASK = 0x00000002u;
static const uint AO_MASK = 0x00000004u;

static const bool DEBUG_CASCADED_SHADOW_MAPS = false;

static const float4 DEBUG_CASCADE_COLORS[NUM_CASCADES] = {
    float4(1.0f, 0.0f, 0.5f, 1.0f),
    float4(0.0f, 1.0f, 0.5f, 1.0f),
    float4(0.3f, 0.5f, 1.0f, 1.0f),
    float4(1.0f, 0.5f, 0.5f, 1.0f),
};

struct LucyMaterialAttribute {
    float AlbedoSlot;
    float NormalSlot;
    float RoughnessSlot;
    float MetallicSlot;
    float4 BaseDiffuseColor;
    float BaseRoughnessValue;
    float BaseMetallicValue;
    float BaseAOValue;
    float AOSlot;
    float NormalStrength;
};

struct DirectionalLight {
    float3 Direction;
    float3 Color;
    float4x4 DirLightShadowMatrices[NUM_CASCADES];
    float4 DirLightShadowCascadeSplits; // x = 0, y = 1, z = 2, w = 3 cascadeIndex
};

struct LucyLightValues {
    DirectionalLight u_DirectionalLight;
};

[[vk::binding(1, 0)]]
ConstantBuffer<LucyLightValues> LightValues;

[[vk::binding(2, 0)]]
StructuredBuffer<LucyMaterialAttribute> MaterialAttributes;

[[vk::binding(3, 0)]]
SamplerCube u_IrradianceMap;

[[vk::binding(4, 0)]]
SamplerCube u_PrefilterMap;

[[vk::binding(5, 0)]]
Sampler2D u_BRDFLut;

[[vk::binding(6, 0)]]
Sampler2DArray u_ShadowMap;

[[vk::binding(0, 1)]] 
Sampler2D u_Textures[];

#include "LucySamplingUtilities.slang"

#define DIELECTRIC_SPECULAR 0.04

void SampleAttributeTexture(in PixelInput input, float slot, inout float4 outColor) {
    if (slot == NULL_TEXTURE_SLOT)
        return;
    outColor = u_Textures[int(slot)].Sample(input.a_TextureCoordsOut);
}

void SampleAttributeTexture(in PixelInput input, float slot, inout float outValue, uint mask) {
    if (slot == NULL_TEXTURE_SLOT)
        return;
    float4 texValue = u_Textures[int(slot)].Sample(input.a_TextureCoordsOut);
    switch (mask) {
        case ROUGHNESS_MASK:
            outValue *= texValue.g;
            break;
        case METALLIC_MASK:
            outValue *= texValue.b;
            break;
        case AO_MASK:
            outValue *= texValue.r;
            break;
    }
}

float ChebyshevUpperBound(float2 moments, float t) {
    const float minVariance = 0.00001f;

    if (t <= moments.x)
        return 1.0f;

    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);

    float d = t - moments.x;
    float p_max = variance / (variance + d * d);
    return p_max;
}

float LinearStep(float min, float max, float v) {
    return clamp((v - min) / (max - min), 0.0, 1.0);
} 

float ReduceLightBleeding(float p_max, float amount) {
    return LinearStep(amount, 1.0f, p_max);
}

float VSMContribution(float2 posInShadowSpace, float distanceToLight, uint cascadeIndex) {
    float2 moments = u_ShadowMap.Sample(float3(posInShadowSpace, cascadeIndex)).xy;
    return ReduceLightBleeding(ChebyshevUpperBound(moments, distanceToLight), 0.8f);
}

static const float4x4 g_TexScaleBias = float4x4(
    float4(0.5f, 0.0f, 0.0f, 0.5f),
    float4(0.0f, 0.5f, 0.0f, 0.5f),
    float4(0.0f, 0.0f, 1.0f, 0.0f),
    float4(0.0f, 0.0f, 0.0f, 1.0f),
);

float4 CalculatePosInShadowSpace(float3 modelWorldPos, uint cascadeIndex) {
    float4x4 lightMatrix = LightValues.u_DirectionalLight.DirLightShadowMatrices[cascadeIndex];
    float4 posInShadowSpace = mul(g_TexScaleBias, mul(lightMatrix, float4(modelWorldPos, 1.0f)));
    posInShadowSpace /= posInShadowSpace.w;
    return posInShadowSpace;
}

float CalculateShadowFactor(float3 modelWorldPos, uint cascadeIndex) {
    float4 posInShadowSpace = CalculatePosInShadowSpace(modelWorldPos, cascadeIndex);

    if (any(posInShadowSpace.xy < 0.0f) || 
            any(posInShadowSpace.xy > 1.0f) ||
            posInShadowSpace.z < 0.0f || posInShadowSpace.z > 1.0f) {
        return 1.0f;
    }

    return VSMContribution(posInShadowSpace.xy, posInShadowSpace.z, cascadeIndex);
}

float4 ShadowContribution(in PixelInput input, float3 modelWorldPos) {
    uint cascadeIndex = 0;
    for (uint i = 0; i < NUM_CASCADES - 1; i++) {
        if (-input.a_ViewDepthOut > LightValues.u_DirectionalLight.DirLightShadowCascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }

    if (DEBUG_CASCADED_SHADOW_MAPS)
        return DEBUG_CASCADE_COLORS[cascadeIndex];
        
    return float4(CalculateShadowFactor(modelWorldPos, cascadeIndex));
}

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;

    const float gamma = 2.2f;

    LucyMaterialAttribute attributes = MaterialAttributes[(int)PushConstants.u_MaterialID];

    float4 albedoColor = attributes.BaseDiffuseColor;
    float metallicValue = attributes.BaseMetallicValue;
    float roughnessValue = attributes.BaseRoughnessValue;
    float aoValue = attributes.BaseAOValue;

    SampleAttributeTexture(input, attributes.AlbedoSlot, albedoColor);
    SampleAttributeTexture(input, attributes.MetallicSlot, metallicValue, METALLIC_MASK);
    SampleAttributeTexture(input, attributes.RoughnessSlot, roughnessValue, ROUGHNESS_MASK);
    SampleAttributeTexture(input, attributes.AOSlot, aoValue, AO_MASK);

    float alpha = albedoColor.w;
    if (alpha < 0.5f)
        discard;

    int normalSlot = (int)attributes.NormalSlot;
    float3 normalsWS = normalize(input.a_NormalsWSOut);
    if (normalSlot != NULL_TEXTURE_SLOT) {
        float3 normalTS = u_Textures[normalSlot].Sample(input.a_TextureCoordsOut).rgb * 2.0 - 1.0;

        float3 tangentWS = normalize(input.a_TangentWSOut.xyz - normalsWS * dot(normalsWS, input.a_TangentWSOut.xyz));
        float3 bitangentWS = cross(normalsWS, tangentWS) * input.a_TangentWSOut.w;

        normalTS.rg *= attributes.NormalStrength;

        normalsWS = normalize(tangentWS * normalTS.x + bitangentWS * normalTS.y + normalsWS * normalTS.z);
    }

    float3 F0 = float3(DIELECTRIC_SPECULAR);
    F0 = lerp(F0, albedoColor.rgb, metallicValue);

    float3 viewDirectionLight = normalize(-LightValues.u_DirectionalLight.Direction);
    float3 viewDirCamera = normalize(Camera.u_CamPos.xyz - input.a_PosOutWS.xyz);

    float4 shadow = ShadowContribution(input, input.a_PosOutWS);

    float3 ambientValue = albedoColor.rgb * aoValue;

    float3 directLight = BRDF(
        viewDirectionLight,
        viewDirCamera,
        normalsWS, 
        F0, 
        metallicValue,
        roughnessValue,
        albedoColor.rgb,
        LightValues.u_DirectionalLight.Color
    ) * shadow.r;

    float NdotV = saturate(dot(normalsWS, viewDirCamera));
    float3 F = F_SchlickRoughness(NdotV, F0, roughnessValue);
    float3 kD = (1.0f - F) * (1.0 - metallicValue);

    float3 R = normalize(reflect(-viewDirCamera, normalsWS));
    R.xy *= -1.0f;

    float3 prefilterColor = u_PrefilterMap.SampleLevel(R, roughnessValue * (NUM_PREFILTER_MIP_LEVELS - 1)).rgb;
    float2 brdf = u_BRDFLut.Sample(float2(NdotV, roughnessValue)).rg;

    // Indirect diffuse (ambient term) — AO affects only diffuse IBL
    float3 diffuseIBL = u_IrradianceMap.Sample(normalsWS).rgb * albedoColor.rgb * aoValue * kD;
    // Indirect specular (prefiltered env map * BRDF LUT)
    float3 specularIBL = prefilterColor * (F0 * brdf.x + brdf.y);

    float3 ibl = diffuseIBL + specularIBL;
    float3 outputColor = ibl + directLight;

    outputColor = pow(outputColor, float3(1.0f / gamma));
    output.a_Color = float4(outputColor, alpha);

    if (DEBUG_CASCADED_SHADOW_MAPS)
        output.a_Color = shadow;

    //output.a_Color = float4(normalsWS * 0.5f + 0.5f, 1.0f);

    return output;
}
