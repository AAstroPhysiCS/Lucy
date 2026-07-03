#include "LucyCommons.slang"
#include "LucyIBLUtilities.slang"

struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
};

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
};

struct LocalPushConstant {
    float4x4 u_ProjMatrix;
};

[[vk::push_constant]]
LocalPushConstant PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input, uint32_t viewIndex: SV_ViewID) {
    VertexOutput output;
    output.a_PosOut = input.a_Pos;

    float4x4 viewMatrix = Inverse(CaptureViews[viewIndex]);
    float4x4 viewProjMatrix = mul(PushConstants.u_ProjMatrix, viewMatrix);

    output.a_Pos = mul(viewProjMatrix, float4(input.a_Pos, 1.0));
    return output;
}

struct PixelInput {
    [[vk::location(0)]] float3 a_PosOut : ID;
};

struct PixelOutput {
    float4 a_LayeredColorAttachments : SV_Target0;
};

[[vk::binding(0, 0)]]
Sampler2D<float4> u_EquirectangularMap;

float2 SampleSphericalMap(float3 v) {
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= float2(0.1591, 0.3183);
    uv += 0.5f;
    return uv;
}

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;

    float4 color = u_EquirectangularMap.Sample(SampleSphericalMap(normalize(input.a_PosOut)));
    output.a_LayeredColorAttachments = color;

    return output;
}
