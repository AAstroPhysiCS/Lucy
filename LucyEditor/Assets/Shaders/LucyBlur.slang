#include "LucyCommons.slang"

[[vk::binding(0, 0)]]
Texture2DArray<float2> u_InputMoments;

[[vk::binding(1, 0)]]
RWTexture2DArray<float2> u_OutputMoments;

struct BlurData {
    int2 u_BlurDirection;
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
    uint width, height, layers;
    u_InputMoments.GetDimensions(width, height, layers);

    int2 uv = int2(dispatchID.xy);
    int layer = int(dispatchID.z);

    float2 blurredMoments = u_InputMoments.Load(int4(uv, layer, 0)) * GAUSSIAN_WEIGHTS[0];

    for (int i = 1; i <= BLUR_RADIUS; i++) {
        int2 offset = PushConstants.u_BlurDirection * i;

        int2 coordA = clamp(uv + offset, int2(0, 0), int2(int(width) - 1, int(height) - 1));
        int2 coordB = clamp(uv - offset, int2(0, 0), int2(int(width) - 1, int(height) - 1));

        blurredMoments += u_InputMoments.Load(int4(coordA, layer, 0)) * GAUSSIAN_WEIGHTS[i];
        blurredMoments += u_InputMoments.Load(int4(coordB, layer, 0)) * GAUSSIAN_WEIGHTS[i];
    }

    blurredMoments.y = max(blurredMoments.y, blurredMoments.x * blurredMoments.x + 1e-6f);
    u_OutputMoments[dispatchID] = blurredMoments;
}