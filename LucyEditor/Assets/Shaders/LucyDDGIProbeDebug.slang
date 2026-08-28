#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIProbeDebugData {
    float4 ProbeOriginAndRadius;
    float4 ProbeSpacing;
    uint4 ProbeCounts;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIProbeDebugData> PushConstants;

struct VertexOutput {
    float4 Position : SV_Position;

    [[vk::location(0)]]
    float3 Normal : NORMAL;
};

[shader("vertex")]
VertexOutput main(uint vertexIndex: SV_VulkanVertexID, uint instanceIndex: SV_VulkanInstanceID) {
    SceneCamera camera = PushConstants.Root->Camera;

    uint3 probeCoordinate = GetProbeCoordinate(instanceIndex, PushConstants.Data.ProbeCounts.xyz);
    float3 probePosition = PushConstants.Data.ProbeOriginAndRadius.xyz + float3(probeCoordinate) * PushConstants.Data.ProbeSpacing.xyz;
    GPUVertex vertex = PushConstants.Root->Addresses.Vertices[vertexIndex];

    float radius = PushConstants.Data.ProbeOriginAndRadius.w;
    float3 worldPosition = probePosition + vertex.Position * radius;
    float4 viewPosition = mul(camera.u_ViewMatrix, float4(worldPosition, 1.0f));
    
    VertexOutput output;
    output.Position = mul(camera.u_ProjMatrix, viewPosition);
    return output;
}

struct PixelInput {
    [[vk::location(0)]]
    float3 Normal : NORMAL;
};

struct PixelOutput {
    float4 Color : SV_Target0;
};

[shader("fragment")]
PixelOutput main(PixelInput input) {
    float3 normal = normalize(input.Normal);
    float3 color = float3(1.0f, 1.0f, 1.0f);
    //do some sort of basic lighting
    PixelOutput output;
    output.Color = float4(color, 1.0f);
    return output;
}