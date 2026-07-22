#include "LucyCommons.slang"
#include "LucyResource.slang"

struct BlurData {
    int4 u_BlurData;            // x, y, width, height
    uint4 u_TextureIndicesData; // x: input texture index, y: output texture index, z: sampler index, w: unused
};

[[vk::push_constant]]
BlurData PushConstants;

// 9-tap separable Gaussian weights
static const int BLUR_RADIUS = 4;
static const float GAUSSIAN_WEIGHTS[5] = {
    0.2270270270f,
    0.1945945946f,
    0.1216216216f,
    0.0540540541f,
    0.0162162162f
};

[numthreads(8, 8, 1)]
[shader("compute")]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    uint width = PushConstants.u_BlurData.z;
    uint height = PushConstants.u_BlurData.w;

    int2 uv = int2(dispatchID.xy);
    int layer = int(dispatchID.z);

    Texture2DArray<float2> inputTexture = BindlessTable::TextureArrays2D_Float2[NonUniformResourceIndex(PushConstants.u_TextureIndicesData.x)];
    RWTexture2DArray<float2> outputTexture = BindlessTable::StorageTextureArrays2D_Float2[NonUniformResourceIndex(PushConstants.u_TextureIndicesData.y)];

    float2 blurredMoments = inputTexture.Load(int4(uv, layer, 0)) * GAUSSIAN_WEIGHTS[0];

    for (int i = 1; i <= BLUR_RADIUS; i++) {
        int2 offset = PushConstants.u_BlurData.xy * i;

        int2 coordA = clamp(uv + offset, int2(0, 0), int2(int(width) - 1, int(height) - 1));
        int2 coordB = clamp(uv - offset, int2(0, 0), int2(int(width) - 1, int(height) - 1));

        blurredMoments += inputTexture.Load(int4(coordA, layer, 0)) * GAUSSIAN_WEIGHTS[i];
        blurredMoments += inputTexture.Load(int4(coordB, layer, 0)) * GAUSSIAN_WEIGHTS[i];
    }

    blurredMoments.y = max(blurredMoments.y, blurredMoments.x * blurredMoments.x + 1e-6f);
    outputTexture[dispatchID] = blurredMoments;
}