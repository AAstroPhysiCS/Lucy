#include "LucyResource.slang"

struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
    [[vk::location(1)]] float3 a_ID : ID;
};

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_IDOut : ID;
};

struct LocalPushConstant {
    float4x4 u_ModelMatrix;
};

[vk::push_constant]
PushConstantGlobal<LocalPushConstant> PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    SceneCamera camera = PushConstants.Root->Camera;

    VertexOutput output;
    output.a_IDOut = input.a_ID;
    
    // Calculate position in clip space
    float4 worldPos = mul(PushConstants.Data.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float4 viewPos = mul(camera.u_ViewMatrix, worldPos);
    float4 clipPos = mul(camera.u_ProjMatrix, viewPos);

    output.a_Pos = clipPos;
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
    
    const float3 scaledID = input.a_IDOut / 255.0;
    const float3 roundedID = round(scaledID * 10000.0) / 10000.0;
    
    output.a_Color = float4(roundedID, 1.0);
    return output;
}