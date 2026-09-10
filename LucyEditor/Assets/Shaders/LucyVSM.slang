#include "LucyResource.slang"

struct VertexOutput {
    float4 position : SV_Position;
};

struct LocalPushConstant {
    GPUVisibleDraw* VisibleDraws;
};

[vk::push_constant]
PushConstantGlobal<LocalPushConstant> PushConstants;

[shader("vertex")]
VertexOutput main(uint vertexIndex: SV_VulkanVertexID, uint drawIndex: SV_VulkanInstanceID, uint viewIndex: SV_ViewID) {
    const uint cascadeIndex = viewIndex;

    GPUVisibleDraw visibleDraw = PushConstants.Data.VisibleDraws[drawIndex];
    GPUObject object = PushConstants.Root->Addresses.Objects[visibleDraw.ObjectIndex];
    GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[visibleDraw.SubmeshIndex];
    GPUVertex vertex = PushConstants.Root->Addresses.Vertices[vertexIndex];

    float4x4 modelMatrix = submesh.Transform * object.Transform;
    float4 worldPosition = float4(vertex.Position, 1.0f) * modelMatrix;

    VertexOutput output;
    output.position = worldPosition * PushConstants.Root->DirectionalLight.DirLightShadowMatrices[cascadeIndex];
    
    return output;
}

struct PixelInput {
    float4 position : SV_Position;
};

struct PixelOutput {
    float2 a_Moments : SV_Target0;
};

float2 ComputeMoments(float depth) {
    float depthSquared = depth * depth;

    float dx = ddx(depth);
    float dy = ddy(depth);
    float variance = 0.25 * (dx * dx + dy * dy);

    return float2(depth, depthSquared + variance);
}

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;

    float depth = input.position.z / input.position.w;
    
    output.a_Moments = ComputeMoments(depth);
    return output;
}