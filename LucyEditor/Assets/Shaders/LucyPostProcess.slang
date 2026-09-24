#include "LucyResource.slang"

struct PostProcessData {
    GPUTextureResource SceneColor;

    float Exposure;
};

[[vk::push_constant]]
PostProcessData PushConstants;

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

[shader("vertex")]
VertexOutput main(uint vertexIndex : SV_VertexID) {
    static const float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2( 3.0f, -1.0f),
        float2(-1.0f,  3.0f)
    };

    float2 position = positions[vertexIndex];

    VertexOutput output;
    output.Position = float4(position, 0.0f, 1.0f);
    output.UV = position * 0.5f + 0.5f;

    return output;
}

struct PixelInput {
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

struct PixelOutput {
    float4 Color : SV_Target0;
};

float3 LinearToSRGB(float3 color) {
    color = max(color, 0.0f);
    float3 low = color * 12.92f;
    float3 high = 1.055f * pow(color, float3(1.0f / 2.4f)) - 0.055f;
    return lerp(low, high, step(float3(0.0031308f), color));
}

float3 ACESFilm(float3 x) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("fragment")]
PixelOutput main(PixelInput input) {
    PixelOutput output;
    float4 sceneColor = PushConstants.SceneColor.Sample2DTexture(input.UV);
    float3 color = sceneColor.rgb;
    color *= PushConstants.Exposure;
    color = ACESFilm(color);
    color = LinearToSRGB(color);

    output.Color = float4(color, sceneColor.a);

    return output;
}