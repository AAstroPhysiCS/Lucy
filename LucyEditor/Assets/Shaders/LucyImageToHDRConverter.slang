#include "LucyCommons.slang"
#include "LucyIBLUtilities.slang"
#include "LucyResource.slang"

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
};

struct LocalPushConstant {
    float4x4 u_ProjMatrix;
    uint2 u_Data; // x: environment map index, y: sampler index
};

[vk::push_constant]
PushConstantGlobal<LocalPushConstant> PushConstants;

[shader("vertex")]
VertexOutput main(uint vertexIndex: SV_VulkanVertexID, uint32_t viewIndex: SV_ViewID) {
    GPUVertex input = PushConstants.Root->Addresses.Vertices[vertexIndex];

    float4x4 viewMatrix = Inverse(CaptureViews[viewIndex]);
    float4x4 viewProjMatrix = mul(PushConstants.Data.u_ProjMatrix, viewMatrix);

    VertexOutput output;
    output.a_PosOut = input.Position;
    output.a_Pos = mul(viewProjMatrix, float4(input.Position, 1.0));
    return output;
}

struct PixelInput {
    [[vk::location(0)]] float3 a_PosOut : ID;
};

struct PixelOutput {
    float4 a_LayeredColorAttachments : SV_Target0;
};

float2 SampleSphericalMap(float3 v) {
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= float2(0.1591, 0.3183);
    uv += 0.5f;
    return uv;
}

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;

    float4 color = BindlessTable::Textures2D[PushConstants.Data.u_Data.x].Sample(BindlessTable::Samplers[PushConstants.Data.u_Data.y], SampleSphericalMap(normalize(input.a_PosOut)));
    output.a_LayeredColorAttachments = color;

    return output;
}
