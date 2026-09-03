#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

struct DDGIProbeUpdateData {
    DDGIRayResult* RayResults;

    // x = current irradiance
    // y = current depth
    // z = history irradiance
    // w = history depth
    uint4 AtlasIndices;

    uint4 ProbeCountsAndRays;

    // x = irradiance texels
    // y = irradiance tile size
    // z = depth texels
    // w = depth tile size
    uint4 AtlasTexelData;

    // x = max ray distance
    // y = depth sharpness
    float2 DistanceData;

    // x = irradiance hysteresis
    // y = depth hysteresis
    // z = history valid
    float3 TemporalData;

    uint FrameNumber;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIProbeUpdateData> PushConstants;

float3 CalculateProbeIrradiance(uint probeIndex, float3 texelDirection) {
    uint raysPerProbe = PushConstants.Data.ProbeCountsAndRays.w;
    uint firstRayIndex = probeIndex * raysPerProbe;

    float3 irradiance = float3(0.0f);

    // Rays are uniformly distributed over the sphere.
    //
    // Integral estimator:
    //
    // E(n) = 4*pi/N Sum{ Li * max(n dot wi, 0) }

    for (uint rayIndex = 0; rayIndex < raysPerProbe; rayIndex++) {
        float3 rayDirection = GetDDGIRayDirection(rayIndex, raysPerProbe, PushConstants.Data.FrameNumber);
        
        float weight = max(dot(texelDirection, rayDirection), 0.0f);
        
        float3 radiance = PushConstants.Data.RayResults[firstRayIndex + rayIndex].RadianceAndDistance.xyz;
        irradiance += radiance * weight;
    }

    irradiance *= (4.0f * PI) / float(raysPerProbe);
    return irradiance;
}

// adaptive hysteresis
float3 ApplyIrradianceHistory(uint2 atlasCoordinate, float3 currentIrradiance) {
    bool historyValid = PushConstants.Data.TemporalData.z > 0.5f;

    if (!historyValid)
        return currentIrradiance;

    uint historyIndex = PushConstants.Data.AtlasIndices.z;

    float3 history = BindlessTable::Textures2D[NonUniformResourceIndex(historyIndex)].Load(int3(atlasCoordinate, 0)).rgb;

    float baseHysteresis = PushConstants.Data.TemporalData.x;

    static const float minHysteresis = 0.70f;
    static const float changeThreshold = 0.10f;
    static const float fullResponseThreshold = 0.75f;
    static const float epsilon = 0.01f;

    float3 difference = abs(currentIrradiance - history);
    float maxDifference = max(difference.r, max(difference.g, difference.b));

    float currentMagnitude = max(currentIrradiance.r, max(currentIrradiance.g, currentIrradiance.b));
    float historyMagnitude = max(history.r, max(history.g, history.b));

    float referenceMagnitude = max(max(currentMagnitude, historyMagnitude), epsilon);
    float relativeChange = maxDifference / referenceMagnitude;

    float response = smoothstep(changeThreshold, fullResponseThreshold, relativeChange);
    float hysteresis = lerp(baseHysteresis, minHysteresis, response);

    return lerp(currentIrradiance, history, hysteresis);
}

float2 ApplyDepthHistory(uint2 atlasCoordinate, float2 currentMoments) {
    bool historyValid = PushConstants.Data.TemporalData.z > 0.5f;
    if (!historyValid)
        return currentMoments;

    uint historyIndex = PushConstants.Data.AtlasIndices.w;

    float2 history = BindlessTable::Textures2D_Float2[NonUniformResourceIndex(historyIndex)].Load(int3(atlasCoordinate, 0));
    float hysteresis = PushConstants.Data.TemporalData.y;

    return lerp(currentMoments, history, hysteresis);
}

float2 CalculateProbeDepthMoments(uint probeIndex, float3 texelDirection) {
    uint raysPerProbe = PushConstants.Data.ProbeCountsAndRays.w;
    uint firstRayIndex = probeIndex * raysPerProbe;

    float maxRayDistance = PushConstants.Data.DistanceData.x;
    float depthSharpness = PushConstants.Data.DistanceData.y;

    float distanceSum = 0.0f;
    float distanceSquaredSum = 0.0f;
    float weightSum = 0.0f;

    for (uint rayIndex = 0; rayIndex < raysPerProbe; rayIndex++) {
        float3 rayDirection = GetDDGIRayDirection(rayIndex, raysPerProbe, PushConstants.Data.FrameNumber);

        float cosine = max(dot(texelDirection, rayDirection), 0.0f);
        float weight = pow(cosine, depthSharpness);

        float distance = PushConstants.Data.RayResults[firstRayIndex + rayIndex].RadianceAndDistance.w;

        float normalizedDistance = saturate(distance / maxRayDistance);
        distanceSum += normalizedDistance * weight;
        distanceSquaredSum += normalizedDistance * normalizedDistance * weight;

        weightSum += weight;
    }

    if (weightSum <= 0.000001f)
        return float2(1.0f, 1.0f);
    return float2(distanceSum / weightSum, distanceSquaredSum / weightSum);
}

// one workgroup represents one probe
[numthreads(18, 18, 1)]
void DDGIProbeUpdateMain(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID) {
    uint probeIndex = groupID.x;

    uint3 probeCounts = PushConstants.Data.ProbeCountsAndRays.xyz;

    uint probeCount = probeCounts.x * probeCounts.y * probeCounts.z;

    if (probeIndex >= probeCount)
        return;

    uint irradianceTexels = PushConstants.Data.AtlasTexelData.x;
    uint irradianceTileSize = PushConstants.Data.AtlasTexelData.y;
    uint depthTexels = PushConstants.Data.AtlasTexelData.z;
    uint depthTileSize = PushConstants.Data.AtlasTexelData.w;

    uint3 probeCoordinate = GetProbeCoordinate(probeIndex, probeCounts);
    uint2 atlasTile = GetDDGIProbeAtlasTile(probeCoordinate, probeCounts);
    uint2 localTexel = groupThreadID.xy;

    // Irradiance atlas
    // 8x8 data + 1 pixel border on every side
    // => 10x10
    if (localTexel.x < irradianceTileSize && localTexel.y < irradianceTileSize) {
        float3 texelDirection = GetProbeTexelDirection(localTexel, irradianceTexels);
        float3 irradiance = CalculateProbeIrradiance(probeIndex, texelDirection);
        uint2 atlasCoordinate = atlasTile * irradianceTileSize + localTexel;

        float3 accumulatedIrradiance = ApplyIrradianceHistory(atlasCoordinate, irradiance);
        BindlessTable::StorageTextures2D[NonUniformResourceIndex(PushConstants.Data.AtlasIndices.x)][atlasCoordinate] = float4(accumulatedIrradiance, 1.0f);
    }

    // Visibility / depth atlas
    // 16x16 data + border
    // => 18x18
    if (localTexel.x < depthTileSize && localTexel.y < depthTileSize) {
        float3 texelDirection = GetProbeTexelDirection(localTexel, depthTexels);
        float2 depthMoments = CalculateProbeDepthMoments(probeIndex, texelDirection);
        uint2 atlasCoordinate = atlasTile * depthTileSize + localTexel;

        float2 accumulatedDepth = ApplyDepthHistory(atlasCoordinate, depthMoments);
        BindlessTable::StorageTextures2D_Float2[NonUniformResourceIndex(PushConstants.Data.AtlasIndices.y)][atlasCoordinate] = accumulatedDepth;
    }
}