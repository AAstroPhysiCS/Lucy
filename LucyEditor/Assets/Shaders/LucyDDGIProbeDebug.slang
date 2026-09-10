#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIProbeDebugData {
    float4* ProbeOffsets;

    uint3 IrradianceAtlasData; // x = texture index, y = interior texels, z = tile size

    float4 ProbeOriginAndRadius;
    float3 ProbeSpacing;
    uint3 ProbeCounts;
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

    uint3 probeCoordinate = GetProbeCoordinate(instanceIndex, PushConstants.Data.ProbeCounts);
    float3 probeOffset = PushConstants.Data.ProbeOffsets[instanceIndex].xyz;
    float3 probePosition = PushConstants.Data.ProbeOriginAndRadius.xyz + float3(probeCoordinate) * PushConstants.Data.ProbeSpacing + probeOffset;

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
    uint irradianceAtlasIndex = PushConstants.Data.IrradianceAtlasData.x;
    uint irradianceTexels = PushConstants.Data.IrradianceAtlasData.y;
    uint irradianceTileSize = PushConstants.Data.IrradianceAtlasData.z;

    uint3 probeCounts = PushConstants.Data.ProbeCounts;
    uint3 probeCoordinate = GetProbeCoordinate(input.ProbeIndex, probeCounts);

    uint2 atlasTile = uint2(probeCoordinate.x + probeCoordinate.z * probeCounts.x, probeCoordinate.y);

    uint2 tileStart = atlasTile * irradianceTileSize + uint2(1, 1);

    float3 irradiance = float3(0.0f);

    for (uint y = 0; y < irradianceTexels; y++) {
        for (uint x = 0; x < irradianceTexels; x++) {
            uint2 atlasCoordinate = tileStart + uint2(x, y);
            irradiance += BindlessTable::Textures2D[NonUniformResourceIndex(irradianceAtlasIndex)].Load(int3(atlasCoordinate, 0)).rgb;
        }
    }

    irradiance /= float(irradianceTexels * irradianceTexels);

    // only for debug visualization
    float3 color = irradiance / (irradiance + 1.0f);

    PixelOutput output;
    output.Color = float4(color, 1.0f);
    return output;
}