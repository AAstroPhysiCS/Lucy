#include "LucyResource.slang"

struct HiZData {
    GPUTextureResource Source;
    GPUTextureResource Destination;

    //x, y = Destination Width/Height, z = SourceMip, w = DestinationMip
    uint4 HiZData;
};

[[vk::push_constant]]
PushConstantGlobal<HiZData> PushConstants;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    if (dispatchID.x >= PushConstants.Data.HiZData.x || dispatchID.y >= PushConstants.Data.HiZData.y)
        return;

    Texture2D<float> source = BindlessTable::Textures2D_Float[PushConstants.Data.Source.TextureResource.Index];
    RWTexture2D<float> destination = BindlessTable::StorageTextures2D_Float[PushConstants.Data.Destination.TextureResource.Index];

    const uint2 sourcePosition = dispatchID.xy * 2u;

    const float depth0 = source.Load(int3(sourcePosition + uint2(0, 0), PushConstants.Data.HiZData.z));
    const float depth1 = source.Load(int3(sourcePosition + uint2(1, 0), PushConstants.Data.HiZData.z));
    const float depth2 = source.Load(int3(sourcePosition + uint2(0, 1), PushConstants.Data.HiZData.z));
    const float depth3 = source.Load(int3(sourcePosition + uint2(1, 1), PushConstants.Data.HiZData.z));

    //ReversedZ aka. min
    const float result = min(min(depth0, depth1), min(depth2, depth3));
    destination[dispatchID.xy] = result;
}