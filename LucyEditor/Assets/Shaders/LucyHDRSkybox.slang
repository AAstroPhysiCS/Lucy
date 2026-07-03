struct VertexInput {
    [[vk::location(0)]] float3 a_Pos : POSITION;
};

struct VertexOutput {
    float4 a_Pos : SV_Position;
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
};

struct LucyCamera {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LucyCamera> Camera;

[shader("vertex")]
VertexOutput main(VertexInput input) {
    VertexOutput output;
    
    output.a_PosOut = input.a_Pos;
    output.a_PosOut.xy *= -1.0f;

    float4 posWorld = float4(input.a_Pos, 1.0);

    float4x4 viewMatrix = float4x4(
        float4(Camera.u_ViewMatrix[0].xyz, 0.0f),
        float4(Camera.u_ViewMatrix[1].xyz, 0.0f),
        float4(Camera.u_ViewMatrix[2].xyz, 0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    );

    float4 posView = mul(viewMatrix, posWorld);
    float4 posClip = mul(Camera.u_ProjMatrix, posView);

    // Use .xyww for skybox rendering (depth = 1.0)
    output.a_Pos = posClip.xyww;
    
    return output;
}

#include "LucyCommons.slang"

struct PixelInput {
    [[vk::location(0)]] float3 a_PosOut : POSITION0;
};

struct PixelOutput {
    float4 a_Color : SV_Target0;
};

struct Parameters {
    float u_MipLevelToSampleFrom;
};

[[vk::binding(1, 0)]]
SamplerCube<float4> u_EnvironmentMap;

[[vk::binding(2, 0)]]
ConstantBuffer<Parameters> Params;

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;

    //uint faceWidth = 0, faceHeight = 0, elements = 0;
    //u_EnvironmentMap.GetDimensions(faceWidth, faceHeight, elements);
    //float2 texelSize = 1.0 / float2(faceWidth, faceHeight);

    //float3 uvWithFaceIndex = Sampler2DArrayToUV(input.a_PosOut);
    //float2 uv = uvWithFaceIndex.xy;
    //uv = uv * (1.0 - 2.0 * texelSize) + texelSize;

    float3 envColor = u_EnvironmentMap.SampleLevel(input.a_PosOut, Params.u_MipLevelToSampleFrom).rgb;

    const float gamma = 2.2;
    envColor = pow(envColor, 1.0 / gamma);
    
    output.a_Color = float4(envColor, 1.0);
    return output;
}
