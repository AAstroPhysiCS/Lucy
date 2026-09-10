#include "LucySamplingUtilities.slang"
#include "LucyResource.slang"

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = pow(roughness, 4.0);
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float2 ComputeBRDF(float2 uv) {
	float NdotV = uv.x;
    float roughness = uv.y;
    //roughness = (roughness * roughness) / 2.0;

    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
	
    // Normal always points along z-axis
	float3 N = float3(0.0, 0.0, 1.0);

    float2 LUT = float2(0.0, 0.0);
	for (uint i = 0u; i < SAMPLE_COUNT; i++) {
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if (NdotL > 0.0) {
            float V_pdf = V_SmithGGXCorrelated(NdotV, NdotL, roughness) * VdotH * NdotL / NdotH;
            float Fc = pow(1.0 - VdotH, 5.0);

            LUT += float2((1.0 - Fc) * V_pdf, Fc * V_pdf);
		}
	}

    LUT = 4.0f * LUT / SAMPLE_COUNT;
    return LUT;
}

struct LocalPushConstant {
    uint4 u_BRDFData;           // x: width, y: height, z: input texture index, w: unused
};

[[vk::push_constant]]
LocalPushConstant PushConstants;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint width = PushConstants.u_BRDFData.x;
    uint height = PushConstants.u_BRDFData.y;

    if (threadID.x >= width || threadID.y >= height) {
        return;
    }

    float2 uv = (float2(threadID.x + 0.5f, threadID.y + 0.5f)) / float2(width, height);
    //uv.y = 1.0 - uv.y;

    float2 brdf = ComputeBRDF(uv);

    RWTexture2D<float2> outputTexture = BindlessTable::StorageTextures2D_Float2[PushConstants.u_BRDFData.z];
    outputTexture[threadID.xy] = brdf;
}