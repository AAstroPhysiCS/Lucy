// Vertex Shader
[shader("vertex")]
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
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
    [[vk::location(1)]] float2 a_TextureCoordsOut : TEXCOORD0;
    [[vk::location(2)]] float3 a_NormalsOut : NORMAL;
    [[vk::location(3)]] float a_Depth : DEPTH;
};

[[vk::binding(0, 0)]] 
cbuffer LucyCamera : register(b0) {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

[[vk::push_constant]] 
struct LocalPushConstant {
    float4x4 u_ModelMatrix;
    float u_MaterialID;
} pushConstants;

VertexOutput main(VertexInput input) {
    VertexOutput output;
    
    output.a_PosOut = input.a_Pos;
    output.a_TextureCoordsOut = input.a_TextureCoords;
    output.a_NormalsOut = input.a_Normals;

    float4 worldPos = mul(pushConstants.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float4 viewPos = mul(u_ViewMatrix, worldPos);
    output.a_Depth = viewPos.z;

    output.a_Pos = mul(u_ProjMatrix, viewPos);
    return output;
}

// Fragment Shader
[shader("fragment")]
struct FragmentInput {
    [[vk::location(0)]] float3 a_Pos : POSITION0;
    [[vk::location(1)]] float2 a_TextureCoords : TEXCOORD0;
    [[vk::location(2)]] float3 a_Normals : NORMAL;
    [[vk::location(3)]] float a_Depth : DEPTH;
};

struct FragmentOutput {
    [[vk::location(0)]] float4 a_Color : SV_Target0;
};

static const uint ROUGHNESS_MASK = 0x00000001u;
static const uint METALLIC_MASK = 0x00000002u;
static const uint AO_MASK = 0x00000004u;

#define NULL_TEXTURE_SLOT -1
#define NUM_CASCADES 4
#define DEBUG_CASCADED_SHADOW_MAPS 0

static const float4 DEBUG_CASCADE_COLORS[NUM_CASCADES] = {
    float4(1.0f, 0.0f, 0.5f, 1.0f),
    float4(0.0f, 1.0f, 0.5f, 1.0f),
    float4(0.3f, 0.5f, 1.0f, 1.0f),
    float4(1.0f, 0.5f, 0.5f, 1.0f),
};

struct MaterialAttributes {
    float AlbedoSlot;
    float NormalSlot;
    float RoughnessSlot;
    float MetallicSlot;
    float4 BaseDiffuseColor;
    float BaseRoughnessValue;
    float BaseMetallicValue;
    float BaseAOValue;
    float AOSlot;
};

struct DirectionalLight {
    float3 Direction;
    float _padding0;
    float3 Color;
    float _padding1;
    float4x4 DirLightShadowMatrices[NUM_CASCADES];
    float4 DirLightShadowCascadeSplits;
};

[[vk::push_constant]] 
struct LocalPushConstant {
    float4x4 u_ModelMatrix;
    float u_MaterialID;
} pushConstants;

[[vk::binding(0, 0)]] 
cbuffer LucyCamera : register(b0) {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

[[vk::binding(1, 0)]] 
cbuffer LucyLightningValues : register(b1) {
    DirectionalLight u_DirectionalLight;
};

[[vk::binding(2, 0)]] 
StructuredBuffer<MaterialAttributes> b_MaterialAttributes : register(t0, space0);

[[vk::binding(3, 0)]] 
TextureCube u_IrradianceMap : register(t1, space0);
[[vk::binding(3, 0)]] 
SamplerState u_IrradianceSampler : register(s1, space0);

[[vk::binding(4, 0)]] 
Texture2DArray u_ShadowMap : register(t2, space0);
[[vk::binding(4, 0)]] 
SamplerState u_ShadowSampler : register(s2, space0);

[[vk::binding(0, 1)]] 
Texture2D u_Textures[] : register(t0, space1);
[[vk::binding(0, 1)]] 
SamplerState u_LinearSampler : register(s0, space1);

// Include LucySamplingUtilities (assumed to contain BRDF implementation)
#include "LucySamplingUtilities"

void GetAttributeColor(int slot, float4 baseColor, out float4 outColor) {
    if (slot != NULL_TEXTURE_SLOT) {
        outColor = u_Textures[slot].Sample(u_LinearSampler, a_TextureCoords);
    } else {
        outColor = baseColor;
    }
}

void GetAttributeValue(int slot, float baseValue, out float outValue, uint mask) {
    if (slot != NULL_TEXTURE_SLOT) {
        float4 texValue = u_Textures[slot].Sample(u_LinearSampler, a_TextureCoords);
        switch (mask) {                                                        
            case ROUGHNESS_MASK:                                                
                outValue = texValue.r;        
                break;
            case METALLIC_MASK:                                                
                outValue = texValue.g;        
                break;
            case AO_MASK:                                                
                outValue = texValue.b;        
                break;
        }
    } else {                                            
        outValue = baseValue;
    }
}

float4 LinearizeDepth(float4 fragCoord, float4x4 projMat) {
    float4 normalizedFrag = fragCoord * 2.0 - 1.0;
    float4x4 invProj = inverse(projMat);
    float4 unprojected = mul(invProj, normalizedFrag);
    return unprojected / unprojected.w;
}

float ChebyshevUpperBound(float2 moments, float t) {
    const float minVariance = 0.002f;

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
    float2 moments = u_ShadowMap.Sample(u_ShadowSampler, float3(posInShadowSpace, cascadeIndex)).xy;
    return ReduceLightBleeding(ChebyshevUpperBound(moments, distanceToLight), 0.9f);
}

static const float4x4 g_TexScaleBias = float4x4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0 
);

float CalculateShadowFactor(float4 modelWorldPos, uint cascadeIndex) {
    float4x4 lightMatrix = u_DirectionalLight.DirLightShadowMatrices[cascadeIndex];
    float4 posInShadowSpace = mul(g_TexScaleBias, mul(lightMatrix, modelWorldPos));
    posInShadowSpace /= posInShadowSpace.w;

    if (any(posInShadowSpace.xy < 0.0) || 
        any(posInShadowSpace.xy > 1.0) ||
        posInShadowSpace.z < 0.0 || posInShadowSpace.z > 1.0) {
        return 1.0;
    }

    return VSMContribution(posInShadowSpace.xy, posInShadowSpace.z, cascadeIndex);
}

float4 ShadowContribution(float4 modelWorldPos) {
    uint cascadeIndex = 0;
    for(uint i = 0; i < NUM_CASCADES - 1; i++) {
        if(-FragmentInput.a_Depth > u_DirectionalLight.DirLightShadowCascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }

    if (DEBUG_CASCADED_SHADOW_MAPS)
        return DEBUG_CASCADE_COLORS[cascadeIndex];
        
    return float4(CalculateShadowFactor(modelWorldPos, cascadeIndex).xxx, 1.0);
}

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    float alpha = 1.0f;
    MaterialAttributes attributes = b_MaterialAttributes[(int)pushConstants.u_MaterialID];

    int albedoSlot = (int)attributes.AlbedoSlot;
    int normalSlot = (int)attributes.NormalSlot;
    int metallicSlot = (int)attributes.MetallicSlot;
    int roughnessSlot = (int)attributes.RoughnessSlot;
    int aoSlot = (int)attributes.AOSlot;

    float4 albedoColor = float4(0.0f, 0.0f, 0.0f, alpha);
    float metallicValue = 0.0f;
    float roughnessValue = 0.0f;
    float aoValue = 0.0f;

    GetAttributeColor(albedoSlot, attributes.BaseDiffuseColor, albedoColor);
    GetAttributeValue(metallicSlot, attributes.BaseMetallicValue, metallicValue, METALLIC_MASK);
    GetAttributeValue(roughnessSlot, attributes.BaseRoughnessValue, roughnessValue, ROUGHNESS_MASK);
    GetAttributeValue(aoSlot, attributes.BaseAOValue, aoValue, AO_MASK);

    alpha = albedoColor.w;
    if (alpha < 0.5f)
        discard;

    float4 modelWorldPos = mul(pushConstants.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float3 modelNormalNormalized = normalize(mul((float3x3)pushConstants.u_ModelMatrix, input.a_Normals));
    float3 viewDirCamera = normalize(u_CamPos.xyz - modelWorldPos.xyz);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedoColor.rgb, metallicValue);

    float3 viewDirectionLight = normalize(modelWorldPos.xyz - u_DirectionalLight.Direction);
    float3 specularContribution = BRDF(
        viewDirectionLight, 
        viewDirCamera, 
        modelNormalNormalized, 
        F0, 
        metallicValue, 
        roughnessValue, 
        albedoColor.rgb, 
        u_DirectionalLight.Color
    );

    float3 ambientContribution = u_IrradianceMap.Sample(u_IrradianceSampler, normalize(input.a_Normals)).rgb;

    float4 shadow = ShadowContribution(modelWorldPos);
    float3 directLighting = specularContribution * shadow.r;
    float3 ambient = ambientContribution * aoValue;
    float3 outputColor = (directLighting + ambient) * albedoColor.rgb;

    const float gamma = 2.2f;
    outputColor = pow(outputColor, float3(1.0f / gamma));

    output.a_Color = float4(outputColor, alpha);

    if (DEBUG_CASCADED_SHADOW_MAPS)
        output.a_Color = shadow;

    return output;
}