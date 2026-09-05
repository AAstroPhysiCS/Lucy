#include "LucyResource.slang"
#include "LucySamplingUtilities.slang"
#include "LucyDDGICommon.slang"

[[vk::binding(0, 2)]]
RaytracingAccelerationStructure u_TLAS;

struct DDGITraceData {
    DDGIRayResult* RayResults;
    float4* ProbeOffsets;
    uint* ProbeUpdateList;

    GPUTextureResource EnvironmentMap;
    GPUTextureResource IrradianceHistory;
    GPUTextureResource DepthHistory;

    float4 ProbeOriginAndMaxDistance;
    float4 ProbeSpacing;
    uint4 ProbeCountsAndRays;

    uint FrameNumber;
};

struct DDGIRayPayload {
	float3 Radiance;
	float Distance;
};

[[vk::push_constant]]
PushConstantGlobal<DDGITraceData> PushConstants;

//shadow ray query
float GetDirectionalLightVisibility(float3 positionWS, float3 surfaceNormalWS, float3 lightDirection) {
    float3 offsetNormal = dot(surfaceNormalWS, lightDirection) >= 0.0f ? surfaceNormalWS : -surfaceNormalWS;

    RayDesc ray;
    ray.Origin = positionWS + offsetNormal * 0.02f;
    ray.Direction = lightDirection;
    ray.TMin = 0.001f;
    ray.TMax = PushConstants.Data.ProbeOriginAndMaxDistance.w;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(u_TLAS, RAY_FLAG_NONE, 0xFF, ray);

    while (query.Proceed()) {
    }

    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0f : 0.0f;
}

[shader("raygeneration")]
void RayGen() {
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    uint rayIndex = dispatchIndex.x;
    uint updateIndex = dispatchIndex.y;
    uint probeIndex = PushConstants.Data.ProbeUpdateList[updateIndex];

    uint3 probeCounts = PushConstants.Data.ProbeCountsAndRays.xyz;
    uint raysPerProbe = PushConstants.Data.ProbeCountsAndRays.w;

    uint3 probeCoordinate = GetProbeCoordinate(probeIndex, probeCounts);
    float3 probeOffset = float3(0.0f);
    if (PushConstants.Data.FrameNumber > 0)
        probeOffset = PushConstants.Data.ProbeOffsets[probeIndex].xyz;
    float3 probePosition = PushConstants.Data.ProbeOriginAndMaxDistance.xyz + float3(probeCoordinate) * PushConstants.Data.ProbeSpacing.xyz + probeOffset;

    float3 rayDirection;
    if (rayIndex == 0 && length(probeOffset) > 0.0001f)
        rayDirection = normalize(-probeOffset);
    else
        rayDirection = GetDDGIRayDirection(rayIndex, raysPerProbe, PushConstants.Data.FrameNumber);

    float maxRayDistance = PushConstants.Data.ProbeOriginAndMaxDistance.w;

    DDGIRayPayload payload;
    payload.Radiance = float3(0.0f);
    payload.Distance = 111.0f;

    RayDesc ray;
    ray.Origin = probePosition;
    ray.Direction = rayDirection;
    ray.TMin = 0.01f;
    ray.TMax = maxRayDistance;

    TraceRay(u_TLAS, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    uint resultIndex = probeIndex * raysPerProbe + rayIndex;
    PushConstants.Data.RayResults[resultIndex].RadianceAndDistance = float4(payload.Radiance, payload.Distance);
}

[shader("miss")]
void Miss(inout DDGIRayPayload payload) {
    float3 rayDirection = normalize(-WorldRayDirection());

    float environmentIntensity = PushConstants.Data.ProbeSpacing.w;

    payload.Radiance = PushConstants.Data.EnvironmentMap.SampleCubeTextureLevel(rayDirection, 0.0f).rgb * environmentIntensity;
    payload.Distance = PushConstants.Data.ProbeOriginAndMaxDistance.w;
}

[shader("closesthit")]
void ClosestHit(inout DDGIRayPayload payload, BuiltInTriangleIntersectionAttributes attributes) {
    DDGISurface surface = GetDDGISurface(PushConstants.Root, attributes);

    PBRMaterial material = PushConstants.Root->Addresses.PBRMaterials[surface.MaterialIndex];
    PBRMaterialSample materialSample = material.SampleLevel(surface.UV, 0.0f);

    float3 surfaceNormalWS = surface.NormalWS;
    float3 normalWS = GetDDGISurfaceNormal(surface, material, materialSample);
    float3 viewDirection = normalize(-WorldRayDirection());

    float3 lightDirection = normalize(-PushConstants.Root->DirectionalLight.Direction);

    float visibility = GetDirectionalLightVisibility(surface.PositionWS, surfaceNormalWS, lightDirection);

    float3 F0 = float3(DIELECTRIC_SPECULAR);
    F0 = lerp(F0, materialSample.BaseColor.rgb, materialSample.Metallic);

    float3 directLighting = BRDF(
        lightDirection,
        viewDirection,
        normalWS,
        F0,
        materialSample.Metallic,
        materialSample.Roughness,
        materialSample.BaseColor.rgb,
        PushConstants.Root->DirectionalLight.Color
    ) * visibility;

    float3 multiBounceLighting = float3(0.0f);

    if (PushConstants.Data.FrameNumber > 0) {
        DDGISamplingData ddgiData;
        ddgiData.IrradianceAtlas = PushConstants.Data.IrradianceHistory;
        ddgiData.ProbeOffsets = PushConstants.Data.ProbeOffsets;
        ddgiData.DepthAtlas = PushConstants.Data.DepthHistory;
        ddgiData.ProbeOriginAndMaxDistance = PushConstants.Data.ProbeOriginAndMaxDistance;
        ddgiData.ProbeSpacingAndBias = PushConstants.Data.ProbeSpacing;
        ddgiData.ProbeCounts = PushConstants.Data.ProbeCountsAndRays.xyz;

        if (IsInsideDDGIVolume(ddgiData, surface.PositionWS)) {
            float3 previousIrradiance = SampleDDGIIrradiance(ddgiData, surface.PositionWS, normalWS);
            float3 diffuseReflectance = materialSample.BaseColor.rgb * (1.0f - materialSample.Metallic);

            multiBounceLighting = previousIrradiance * diffuseReflectance / PI;
        }
    }

    float3 emissive = materialSample.EmissiveColorAndStrength.rgb * materialSample.EmissiveColorAndStrength.a;
    bool backfaceHit = dot(WorldRayDirection(), surface.GeometricNormalWS) > 0.0f;

    payload.Radiance = directLighting + multiBounceLighting + emissive;
    payload.Distance = backfaceHit ? -RayTCurrent() : RayTCurrent();
}

[shader("anyhit")]
void AnyHit(inout DDGIRayPayload payload, BuiltInTriangleIntersectionAttributes attributes) {

}