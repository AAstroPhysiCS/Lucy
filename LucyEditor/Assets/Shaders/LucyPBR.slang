#include "LucyCommons.slang"

struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
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

struct LocalPushConstant {
    float4x4 u_ModelMatrix;
    float4x4 u_ModelMatrixInversedTransposed;
    uint u_MaterialID;
    GPUTextureResource u_PBRTextureResources[4];
};

[[vk::push_constant]]
PushConstantGlobal<LocalPushConstant> PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    SceneCamera camera = PushConstants.Root->Camera;

    float4 worldPos = mul(PushConstants.Data.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float4 viewPos = mul(camera.u_ViewMatrix, worldPos);

    float3x3 normalMatrix = float3x3(PushConstants.Data.u_ModelMatrixInversedTransposed);

    float3 N = normalize(mul(normalMatrix, input.a_Normals));
    float3 T = normalize(mul(normalMatrix, input.a_Tangents));
    float3 B = normalize(mul(normalMatrix, input.a_Bitangents));

    // Make tangent perpendicular to the normal.
    T = normalize(T - N * dot(N, T));

    /* need to do that */
    float handedness = (dot(cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;

    VertexOutput output;
    output.a_Pos = mul(camera.u_ProjMatrix, viewPos);
    output.a_PosOutWS = worldPos.xyz;
    output.a_TextureCoordsOut = input.a_TextureCoords;
    output.a_TangentWSOut = float4(T, handedness);
    output.a_ViewDepthOut = viewPos.z;
    output.a_NormalsWSOut = N;

    return output;
}

#include "LucyCommons.slang"
#include "LucyResource.slang"

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

#include "LucySamplingUtilities.slang"

#define DIELECTRIC_SPECULAR 0.04

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
    float2 moments = PushConstants.Data.u_PBRTextureResources[0].Sample2DArrayTexture_Float2(float3(posInShadowSpace, cascadeIndex));
    return ReduceLightBleeding(ChebyshevUpperBound(moments, distanceToLight), 0.8f);
}

static const float4x4 g_TexScaleBias = float4x4(
    float4(0.5f, 0.0f, 0.0f, 0.5f),
    float4(0.0f, 0.5f, 0.0f, 0.5f),
    float4(0.0f, 0.0f, 1.0f, 0.0f),
    float4(0.0f, 0.0f, 0.0f, 1.0f),
);

float4 CalculatePosInShadowSpace(float3 modelWorldPos, uint cascadeIndex) {
    float4x4 lightMatrix = PushConstants.Root->DirectionalLight.DirLightShadowMatrices[cascadeIndex];
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
        if (-input.a_ViewDepthOut > PushConstants.Root->DirectionalLight.DirLightShadowCascadeSplits[i]) {
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

    PBRMaterial material = PushConstants.Root->Addresses.PBRMaterials[PushConstants.Data.u_MaterialID];
    PBRMaterialSample materialSample = material.Sample(input.a_TextureCoordsOut);

    float alpha = materialSample.BaseColor.a;
    if (alpha < PBRMaterial::GetAlphaCutoff())
        discard;

    float3 normalsWS = normalize(input.a_NormalsWSOut);
    if (material.HasNormalMap()) {
        float3 normalTS = materialSample.NormalTS;
        float3 tangentWS = normalize(input.a_TangentWSOut.xyz - normalsWS * dot(normalsWS, input.a_TangentWSOut.xyz));
        float3 bitangentWS = cross(normalsWS, tangentWS) * input.a_TangentWSOut.w;

        normalsWS = normalize(tangentWS * normalTS.x + bitangentWS * normalTS.y + normalsWS * normalTS.z);
    }

    SceneCamera camera = PushConstants.Root->Camera;

    float3 F0 = float3(DIELECTRIC_SPECULAR);
    F0 = lerp(F0, materialSample.BaseColor.rgb, materialSample.Metallic);

    float3 viewDirectionLight = normalize(-PushConstants.Root->DirectionalLight.Direction);
    float3 viewDirCamera = normalize(camera.u_CamPos.xyz - input.a_PosOutWS.xyz);

    float4 shadow = ShadowContribution(input, input.a_PosOutWS);

    float3 ambientValue = materialSample.BaseColor.rgb * materialSample.AO;

    float3 directLight = BRDF(
        viewDirectionLight,
        viewDirCamera,
        normalsWS, 
        F0, 
        materialSample.Metallic,
        materialSample.Roughness,
             materialSample.BaseColor.rgb,
        PushConstants.Root->DirectionalLight.Color
    ) * shadow.r;

    float NdotV = saturate(dot(normalsWS, viewDirCamera));
    float3 F = F_SchlickRoughness(NdotV, F0, materialSample.Roughness);
    float3 kD = (1.0f - F) * (1.0 - materialSample.Metallic);

    float3 R = normalize(reflect(-viewDirCamera, normalsWS));
    R.xy *= -1.0f;

    float3 prefilterColor = PushConstants.Data.u_PBRTextureResources[1].SampleCubeTextureLevel(R, materialSample.Roughness * (NUM_PREFILTER_MIP_LEVELS - 1)) .rgb;
    float2 brdf = PushConstants.Data.u_PBRTextureResources[2].Sample2DTexture_Float2(float2(NdotV, materialSample.Roughness));

    // Indirect diffuse (ambient term) — AO affects only diffuse IBL
    float3 diffuseIBL = PushConstants.Data.u_PBRTextureResources[3].SampleCubeTexture(normalsWS).rgb * materialSample.BaseColor.rgb * materialSample.AO * kD;
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
