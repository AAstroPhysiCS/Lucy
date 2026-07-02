struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
    [[vk::location(1)]] float3 a_ID : ID;
    [[vk::location(2)]] float2 a_TextureCoords : TEXCOORD0;
    [[vk::location(3)]] float3 a_Normals : NORMAL;
    [[vk::location(4)]] float3 a_Tangents : TANGENT;
    [[vk::location(5)]] float3 a_Bitangents : BITANGENT;
};

struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 a_IDOut : ID;
};

struct LucyCamera {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LucyCamera> Camera;

struct LocalPushConstant {
    float4x4 u_ModelMatrix;
    float u_MaterialID;
};

[[vk::push_constant]]
LocalPushConstant PushConstants;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    VertexOutput output;
    
    output.a_IDOut = input.a_ID;
    
    // Calculate position in clip space
    float4 worldPos = mul(PushConstants.u_ModelMatrix, float4(input.a_Pos, 1.0));
    float4 viewPos = mul(Camera.u_ViewMatrix, worldPos);
    float4 clipPos = mul(Camera.u_ProjMatrix, viewPos);
    
    output.position = clipPos;
    return output;
}

struct PixelInput {
    float4 position : SV_Position;
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