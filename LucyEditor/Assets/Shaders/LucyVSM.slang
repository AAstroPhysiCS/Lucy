#include "LucyCommons.slang"

struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
};

struct VertexOutput {
    float4 position : SV_Position;
};

struct LucyCamera {
    float4x4[NUM_CASCADES] u_LightMatrix;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LucyCamera> ShadowCameraVPs;

struct LocalPushConstant {
    float4x4 u_ModelMatrix;
};

[[vk::push_constant]]
LocalPushConstant PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input, uint viewIndex : SV_ViewID) {
    VertexOutput output;

    float4 worldPos = mul(PushConstants.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float4 projPos = mul(ShadowCameraVPs.u_LightMatrix[viewIndex], worldPos);
    
    output.position = projPos;
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