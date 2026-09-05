#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIProbeRelocationData {
    DDGIRayResult* RayResults;
    float4* ProbeOffsets;
    float4 *ProbeOffsetHistory;
    uint* ProbeUpdateMask;

    float4 ProbeSpacingAndMinFrontfaceDistance;
    uint4 ProbeCountsAndRays;

    // x = backface threshold
    // y = max ray distance
    // z = maximum movement per frame as fraction of probe spacing
    // w = relocation smoothing
    float4 RelocationData;

    uint FrameNumber;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIProbeRelocationData> PushConstants;

[numthreads(64, 1, 1)]
void DDGIProbeRelocationMain(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint probeIndex = dispatchThreadID.x;

    uint3 probeCounts = PushConstants.Data.ProbeCountsAndRays.xyz;
    uint raysPerProbe = PushConstants.Data.ProbeCountsAndRays.w;
    uint probeCount = probeCounts.x * probeCounts.y * probeCounts.z;

    if (probeIndex >= probeCount)
        return;

    float3 probeSpacing = PushConstants.Data.ProbeSpacingAndMinFrontfaceDistance.xyz;
    float minFrontfaceDistance = PushConstants.Data.ProbeSpacingAndMinFrontfaceDistance.w;

    float backfaceThreshold = PushConstants.Data.RelocationData.x;
    float maxRayDistance = PushConstants.Data.RelocationData.y;
    float maxMovementFraction = PushConstants.Data.RelocationData.z;
    float relocationSmoothing = PushConstants.Data.RelocationData.w;

    float minProbeSpacing = min(probeSpacing.x, min(probeSpacing.y, probeSpacing.z));

    float3 offset = float3(0.0f);
    if (PushConstants.Data.FrameNumber > 0)
        offset = PushConstants.Data.ProbeOffsetHistory[probeIndex].xyz;

    if (PushConstants.Data.ProbeUpdateMask[probeIndex] == 0) {
        PushConstants.Data.ProbeOffsets[probeIndex] = float4(offset, 0.0f);
        return;
    }

    uint firstRayIndex = probeIndex * raysPerProbe;
    uint fixedRayCount = min(DDGI_FIXED_RAY_COUNT, raysPerProbe);
    uint relocationRayCount = fixedRayCount > 1 ? fixedRayCount - 1 : 0;

    uint backfaceCount = 0;
    uint frontfaceCount = 0;

    float closestBackfaceDistance = maxRayDistance;
    uint closestBackfaceIndex = 0;

    float closestFrontfaceDistance = maxRayDistance;
    uint closestFrontfaceIndex = 0;

    for (uint rayIndex = 1; rayIndex < relocationRayCount; rayIndex++) {
        float distance = PushConstants.Data.RayResults[firstRayIndex + rayIndex].RadianceAndDistance.w;

        if (distance < 0.0f) {
            backfaceCount++;
            float backfaceDistance = -distance;
            if (backfaceDistance < closestBackfaceDistance) {
                closestBackfaceDistance = backfaceDistance;
                closestBackfaceIndex = rayIndex;
            }
            continue;
        }

        if (distance >= maxRayDistance * 0.999f)
            continue;

        frontfaceCount++;
        if (distance < closestFrontfaceDistance) {
            closestFrontfaceDistance = distance;
            closestFrontfaceIndex = rayIndex;
        }
    }

    float backfaceRatio = 0.0f;
    if (relocationRayCount > 0)
        backfaceRatio = float(backfaceCount) / float(relocationRayCount);

    float offsetLength = length(offset);

    bool returnPathBlocked = false;

    if (offsetLength > 0.0001f) {
        float returnRayDistance = PushConstants.Data.RayResults[firstRayIndex].RadianceAndDistance.w;
        if (returnRayDistance < 0.0f)
            returnPathBlocked = true;
        else if (returnRayDistance < maxRayDistance * 0.999f && returnRayDistance < offsetLength + minFrontfaceDistance)
            returnPathBlocked = true;
    }

    float3 targetOffset = offset;

    if (backfaceCount > 0 && backfaceRatio > backfaceThreshold) {
        float3 escapeDirection = GetDDGIRayDirection(closestBackfaceIndex, raysPerProbe, PushConstants.Data.FrameNumber);
        targetOffset = offset + escapeDirection * (closestBackfaceDistance + minFrontfaceDistance);
    } else if (offsetLength > 0.0001f && !returnPathBlocked) {
        targetOffset = float3(0.0f);
    } else if (frontfaceCount > 0 && closestFrontfaceDistance < minFrontfaceDistance) {
        float3 surfaceDirection = GetDDGIRayDirection(closestFrontfaceIndex, raysPerProbe, PushConstants.Data.FrameNumber);
        float distanceToMove = minFrontfaceDistance - closestFrontfaceDistance;
        targetOffset = offset - surfaceDirection * distanceToMove;
    }

    float maxOffsetLength = minProbeSpacing * 0.45f;
    float targetLength = length(targetOffset);

    if (targetLength > maxOffsetLength)
        targetOffset = targetOffset / targetLength * maxOffsetLength;

    bool returningToOriginalPosition = length(targetOffset) < 0.0001f && length(offset) > 0.0001f;
    
    float3 movement = float3(0.0f);
    float maxMovement = minProbeSpacing * maxMovementFraction;

    if (returningToOriginalPosition) {
        float offsetLength = length(offset);
        float returnMovement = maxMovement * 0.25f;

        if (offsetLength <= returnMovement) {
            PushConstants.Data.ProbeOffsets[probeIndex] = float4(0.0f);
            return;
        }

        movement = -normalize(offset) * returnMovement;
    } else {
        float3 desiredOffset = lerp(offset, targetOffset, relocationSmoothing);
        movement = desiredOffset - offset;

        float movementLength = length(movement);
        float movementEpsilon = minProbeSpacing * 0.001f;

        if (movementLength <= movementEpsilon) {
            PushConstants.Data.ProbeOffsets[probeIndex] = float4(offset, 0.0f);
            return;
        }

        if (movementLength > maxMovement)
            movement = movement / movementLength * maxMovement;
    }

    float3 newOffset = offset + movement;
    PushConstants.Data.ProbeOffsets[probeIndex] = float4(newOffset, 0.0f);
}