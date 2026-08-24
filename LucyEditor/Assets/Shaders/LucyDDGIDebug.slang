#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIDebugData {
	DDGIRayResult* RayResults;
    uint RayResultTextureIndex;
    uint MaxRayDistance;
	uint RaysPerProbe;
	uint ProbeCount;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIDebugData> PushConstants;

[numthreads(8, 8, 1)]
void DDGIDebugMain(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint rayIndex = dispatchThreadID.x;
    uint probeIndex = dispatchThreadID.y;

    if (rayIndex >= PushConstants.Data.RaysPerProbe)
        return;

    if (probeIndex >= PushConstants.Data.ProbeCount)
        return;

    uint resultIndex = probeIndex * PushConstants.Data.RaysPerProbe + rayIndex;

    float4 result = PushConstants.Data.RayResults[resultIndex].RadianceAndDistance;

    float normalizedDistance = saturate(result.w / PushConstants.Data.MaxRayDistance);

    float hit = result.x > 0.5f ? 1.0f : 0.0f;

    float3 debugColor = lerp(float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), normalizedDistance);
    debugColor *= hit;

    BindlessTable::StorageTextures2D[PushConstants.Data.RayResultTextureIndex][dispatchThreadID.xy] = float4(debugColor, 1.0f);
}