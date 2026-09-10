#include "LucyResource.slang"

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_IDOut : ID;
};

struct LocalPushConstant {
    GPUVisibleDraw* VisibleDraws;
};

[vk::push_constant]
PushConstantGlobal<LocalPushConstant> PushConstants;

[shader("vertex")]
VertexOutput main(uint vertexIndex: SV_VulkanVertexID, uint drawIndex: SV_VulkanInstanceID) {
    GPUVisibleDraw visibleDraw = PushConstants.Data.VisibleDraws[drawIndex];
    GPUObject object = PushConstants.Root->Addresses.Objects[visibleDraw.ObjectIndex];
    GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[visibleDraw.SubmeshIndex];
    GPUVertex vertex = PushConstants.Root->Addresses.Vertices[vertexIndex];

    float4x4 modelMatrix = object.Transform * submesh.Transform;
    float4 worldPosition = modelMatrix * float4(vertex.Position, 1.0f);

    VertexOutput output;
    output.a_Pos = PushConstants.Root->Camera.u_ProjMatrix * PushConstants.Root->Camera.u_ViewMatrix * worldPosition;
    output.a_IDOut = object.ObjectID.xyz;

    return output;
}

struct PixelInput {
    [[vk::location(0)]] float3 a_IDOut : ID;
};

struct PixelOutput {
    float4 a_Color : SV_Target0;
};

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;
    output.a_Color = float4(input.a_IDOut, 1.0);
    return output;
}