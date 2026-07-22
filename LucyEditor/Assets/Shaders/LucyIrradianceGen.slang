#include "LucyCommons.slang"
#include "LucyIBLUtilities.slang"
#include "LucySamplingUtilities.slang"
#include "LucyComputeUtilities.slang"
#include "LucyResource.slang"

struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
};

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
};

struct LocalPushConstant {
    float4x4 u_ProjMatrix;
    float4 u_Data; // x: size, y: sampler index, z: environment map index, w: environment irradiance compute index
};

[vk::push_constant]
PushConstantGlobal<LocalPushConstant> PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input, uint32_t viewIndex : SV_ViewID) {
    VertexOutput output;
    output.a_PosOut = input.a_Pos;
    
    float4x4 viewMatrix = Inverse(CaptureViews[viewIndex]);
    float4x4 viewProjMatrix = mul(PushConstants.Data.u_ProjMatrix, viewMatrix);
    output.a_Pos = mul(viewProjMatrix, float4(input.a_Pos, 1.0));
    
    return output;
}

#include "LucyCommons.slang"

static const float SAMPLE_DELTA = 0.025f;

float3 CalculateIrradiance(float3 samplingPos, in TextureCube environmentMap, in SamplerState sampler) {
    float3 irradiance = 0.0;
    float3 normal = normalize(samplingPos);

    // Create orthonormal basis
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    // Precompute constants
    const uint phiSteps = uint(TWO_PI / SAMPLE_DELTA);
    const uint thetaSteps = uint(HALF_PI / SAMPLE_DELTA);
    const float totalSamples = phiSteps * thetaSteps;

    // Hemisphere sampling
    [loop] [unroll(8)]
    for (uint i = 0; i < phiSteps; i++) {
        float phi = i * SAMPLE_DELTA;
        float sinPhi, cosPhi;
        sincos(phi, sinPhi, cosPhi);

        [loop] [unroll(8)]
        for (uint j = 0; j < thetaSteps; j++) {
            float theta = j * SAMPLE_DELTA;
            float sinTheta, cosTheta;
            sincos(theta, sinTheta, cosTheta);

            // Hemisphere sample in tangent space
            float3 tangentSample = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);

            // Transform to world space
            float3 sampleDir = normalize(
                tangentSample.x * right +
                tangentSample.y * up +
                tangentSample.z * normal
            );

            // Project to face plane and sample
            irradiance += environmentMap.Sample(sampler, sampleDir).rgb * cosTheta * sinTheta;
        }
    }

    // Final integration
    return PI * irradiance / totalSamples;
}

struct PixelInput {
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
};

struct PixelOutput {
    float4 a_LayeredColorAttachments : SV_Target0;
};

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;

    SamplerState sampler = BindlessTable::Samplers[uint(PushConstants.Data.u_Data.y)];
    TextureCube<float4> environmentMap = BindlessTable::CubeTextures[uint(PushConstants.Data.u_Data.z)];

    float3 irradiance = CalculateIrradiance(input.a_PosOut, environmentMap, sampler);
    output.a_LayeredColorAttachments = float4(irradiance, 1.0);
    
    return output;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchID: SV_DispatchThreadID) {
    const float2 cubemapSize = float2(PushConstants.Data.u_Data.x);

    SamplerState sampler = BindlessTable::Samplers[uint(PushConstants.Data.u_Data.y)];
    TextureCube<float4> environmentMap = BindlessTable::CubeTextures[uint(PushConstants.Data.u_Data.z)];

    uint face = dispatchID.z;
    float3 samplingPos = ThreadIDToSamplingPos(dispatchID.xy, face, cubemapSize);
    float3 irradiance = CalculateIrradiance(samplingPos, environmentMap, sampler);

    RWTexture2DArray<float4> environmentIrradianceCompute = BindlessTable::StorageTextureArrays2D[uint(PushConstants.Data.u_Data.w)];

    uint3 uv = dispatchID;
    // uv.y = uint(height) - uv.y - 1u;
    environmentIrradianceCompute[uv] = float4(irradiance, 1.0f);
}