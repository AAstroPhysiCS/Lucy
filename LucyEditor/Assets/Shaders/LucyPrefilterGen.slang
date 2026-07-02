#include "LucySamplingUtilities.slang"
#include "LucyComputeUtilities.slang"
#include "LucyCommons.slang"

struct LocalPushConstant {
    float4 PrefilterParams; // x: sizeX, y: sizeY, z: roughness, w: mip
};

[[vk::push_constant]]
LocalPushConstant PushConstants;

[[vk::binding(0, 0)]]
SamplerCube<float4> u_EnvironmentMap;

[[vk::binding(1, 0)]]
RWTexture2DArray<float4> u_EnvironmentPrefilterMapOut[MAX_MIP];

float4 PrefilterEnvMap(float3 samplingPos) {
    float3 N = normalize(samplingPos);
    float3 R = N;
    float3 V = R;

    float roughness = PushConstants.PrefilterParams.z;

    float3 prefilteredColor = float3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float D = DistributionGGX(NdotH, roughness);
            
            float resolution = PushConstants.PrefilterParams.x * pow(2, PushConstants.PrefilterParams.w); // resolution = size * 2^mip

            // float pdf = D_GGX(NoH, roughness) * NoH / (4.0 * VoH);
            // but since V = N => VoH == NoH
            float pdf = D * NdotH / max(4.0 * HdotV, 1e-4) + 1e-4;
            // Solid angle of current sample -- bigger for less likely samples
            float omegaS = 1.0 / (float(SAMPLE_COUNT) * pdf + 1e-4);
            // Solid angle of texel
            float omegaP = 4.0 * PI / (6.0 * resolution * resolution);
            // Mip level is determined by the ratio of our sample's solid angle to a texel's solid angle
            float mipLevel = max(0.5 * log2(omegaS / omegaP), 0.0);

            float4 sampleColor = u_EnvironmentMap.SampleLevel(L, mipLevel) * NdotL;
            prefilteredColor += sampleColor.rgb;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= totalWeight;
    return float4(prefilteredColor, 1.0f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 threadID: SV_DispatchThreadID) {
    int mip = int(PushConstants.PrefilterParams.w);
    const float2 cubemapSize = float2(PushConstants.PrefilterParams.x, PushConstants.PrefilterParams.y);

    float3 samplingPos = ThreadIDToSamplingPos(threadID.xy, threadID.z, cubemapSize);
    uint3 uv = threadID;
    //uv.y = uint(cubemapSize.y) - uv.y - 1; // Flip Y for texture coordinates

    u_EnvironmentPrefilterMapOut[mip][uv] = PrefilterEnvMap(samplingPos);
}