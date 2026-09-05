#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIProbePriorityData {
    uint* ProbePriorities;
    uint* ProbeUpdateMask;

    uint* LastUpdateFrames;
    uint* LastUpdateFrameHistory;

    float4* ProbeOffsetHistory;

    float4 ProbeOriginAndMaxDistance;
    float4 ProbeSpacingAndNearDistance;

    // x = mid probe distance
    // y = max probe age
    // zw = unused
    float4 SchedulingData;

    uint4 ProbeCountsAndFrame;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIProbePriorityData> PushConstants;

bool IsProbeVisible(float3 probePosition) {
    float4 viewPosition = mul(
        PushConstants.Root->Camera.u_ViewMatrix,
        float4(probePosition, 1.0f)
    );

    float4 clipPosition = mul(
        PushConstants.Root->Camera.u_ProjMatrix,
        viewPosition
    );

    if (clipPosition.w <= 0.0f)
        return false;

    if (abs(clipPosition.x) > clipPosition.w)
        return false;

    if (abs(clipPosition.y) > clipPosition.w)
        return false;

    if (clipPosition.z < 0.0f || clipPosition.z > clipPosition.w)
        return false;

    return true;
}

[numthreads(64, 1, 1)]
void DDGIProbePriorityMain(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint probeIndex = dispatchThreadID.x;

    uint3 probeCounts = PushConstants.Data.ProbeCountsAndFrame.xyz;
    uint frameNumber = PushConstants.Data.ProbeCountsAndFrame.w;

    uint probeCount = probeCounts.x * probeCounts.y * probeCounts.z;

    if (probeIndex >= probeCount)
        return;

    uint3 probeCoordinate = GetProbeCoordinate(probeIndex, probeCounts);

    float3 probeOffset = float3(0.0f);

    if (frameNumber > 0)
        probeOffset = PushConstants.Data.ProbeOffsetHistory[probeIndex].xyz;

    float3 probePosition =
        PushConstants.Data.ProbeOriginAndMaxDistance.xyz +
        float3(probeCoordinate) * PushConstants.Data.ProbeSpacingAndNearDistance.xyz +
        probeOffset;

    float3 cameraPosition = PushConstants.Root->Camera.u_CamPos.xyz;
    float distanceToCamera = length(probePosition - cameraPosition);

    float nearDistance = PushConstants.Data.ProbeSpacingAndNearDistance.w;
    float midDistance = PushConstants.Data.SchedulingData.x;

    uint maxProbeAge = uint(PushConstants.Data.SchedulingData.y);

    uint lastUpdateFrame = 0;

    if (frameNumber > 0)
        lastUpdateFrame = PushConstants.Data.LastUpdateFrameHistory[probeIndex];

    PushConstants.Data.LastUpdateFrames[probeIndex] = lastUpdateFrame;
    PushConstants.Data.ProbeUpdateMask[probeIndex] = 0;

    uint probeAge = frameNumber - lastUpdateFrame;

    bool visible = IsProbeVisible(probePosition);
    bool relocated = length(probeOffset) > 0.0001f;

    uint priority = 0;

    if (probeAge >= maxProbeAge) {
        priority = 3;
    }
    else if (relocated) {
        priority = 3;
    }
    else if (visible && distanceToCamera <= nearDistance) {
        priority = 3;
    }
    else if (visible) {
        priority = 2;
    }
    else if (distanceToCamera <= midDistance) {
        priority = 1;
    }

    PushConstants.Data.ProbePriorities[probeIndex] = priority;
}