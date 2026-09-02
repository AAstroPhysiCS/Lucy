#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIProbeDebugData {
    DDGIRayResult* RayResults;

    float4 ProbeOriginAndRadius;
    float4 ProbeSpacing;
    uint4 ProbeCountsAndRays;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIProbeDebugData> PushConstants;

struct VertexOutput {
    float4 Position : SV_Position;

    [[vk::location(0)]]
    nointerpolation uint ProbeIndex : PROBE_INDEX;
};

[shader("vertex")]
VertexOutput main(uint vertexIndex: SV_VulkanVertexID, uint instanceIndex: SV_VulkanInstanceID) {
    SceneCamera camera = PushConstants.Root->Camera;

    uint3 probeCoordinate = GetProbeCoordinate(instanceIndex, PushConstants.Data.ProbeCountsAndRays.xyz);
    float3 probePosition = PushConstants.Data.ProbeOriginAndRadius.xyz + float3(probeCoordinate) * PushConstants.Data.ProbeSpacing.xyz;
    GPUVertex vertex = PushConstants.Root->Addresses.Vertices[vertexIndex];

    float radius = PushConstants.Data.ProbeOriginAndRadius.w;
    float3 worldPosition = probePosition + vertex.Position * radius;
    float4 viewPosition = mul(camera.u_ViewMatrix, float4(worldPosition, 1.0f));
    
    VertexOutput output;
    output.Position = mul(camera.u_ProjMatrix, viewPosition);
    output.ProbeIndex = instanceIndex;
    return output;
}

struct PixelInput {
    [[vk::location(0)]]
    nointerpolation uint ProbeIndex : PROBE_INDEX;
};

struct PixelOutput {
    float4 Color : SV_Target0;
};

[shader("fragment")]
PixelOutput main(PixelInput input) {
    uint raysPerProbe = PushConstants.Data.ProbeCountsAndRays.w;
    uint firstRayIndex = input.ProbeIndex * raysPerProbe;

    float3 irradiance = float3(0.0f);

    for (uint rayIndex = 0; rayIndex < raysPerProbe; rayIndex++) {
        irradiance += PushConstants.Data.RayResults[firstRayIndex + rayIndex].RadianceAndDistance.xyz;
    }
    irradiance /= float(raysPerProbe);

    PixelOutput output;
    output.Color = float4(irradiance, 1.0f);
    return output;
}