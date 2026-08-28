#include "LucyResource.slang"
#include "LucyDDGICommon.slang"

[[vk::binding(0, 2)]]
RaytracingAccelerationStructure u_TLAS;

struct DDGITraceData {
	DDGIRayResult* RayResults;

	float4 ProbeOriginAndMaxDistance;
	float4 ProbeSpacing;
	uint4 ProbeCountsAndRays;
};

struct DDGIRayPayload {
	float3 Radiance;
	float Distance;
};

[[vk::push_constant]]
PushConstantGlobal<DDGITraceData> PushConstants;

[shader("raygeneration")]
void RayGen() {
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    uint rayIndex = dispatchIndex.x;
    uint probeIndex = dispatchIndex.y;

    uint3 probeCounts = PushConstants.Data.ProbeCountsAndRays.xyz;
    uint raysPerProbe = PushConstants.Data.ProbeCountsAndRays.w;

    uint3 probeCoordinate = GetProbeCoordinate(probeIndex, probeCounts);
    float3 probePosition = PushConstants.Data.ProbeOriginAndMaxDistance.xyz + float3(probeCoordinate) * PushConstants.Data.ProbeSpacing.xyz;

    float3 rayDirection = GetFibonacciDirection(rayIndex, raysPerProbe);
    float maxRayDistance = PushConstants.Data.ProbeOriginAndMaxDistance.w;

    DDGIRayPayload payload;
    payload.Radiance = float3(0.0f, 0.0f, 1.0f);
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
    payload.Radiance = float3(0.0f, 1.0f, 0.0f);
    payload.Distance = 333.0f;
}

[shader("closesthit")]
void ClosestHit(inout DDGIRayPayload payload, BuiltInTriangleIntersectionAttributes attributes) {

    payload.Radiance = float3(1.0f, 0.0f, 0.0f);
    payload.Distance = 444.0f;
}

[shader("anyhit")]
void AnyHit(inout DDGIRayPayload payload, BuiltInTriangleIntersectionAttributes attributes) {

}