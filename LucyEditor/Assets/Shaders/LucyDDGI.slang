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

float3 GetFibonacciDirection(uint rayIndex, uint rayCount) {
    static const float PI = 3.14159265359f;
    static const float GOLDEN_RATIO = 1.61803398875f;

    float index = float(rayIndex) + 0.5f;
    float cosTheta = 1.0f - 2.0f * index / float(rayCount);

    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * PI * frac(float(rayIndex) / GOLDEN_RATIO);

    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

uint3 GetProbeCoordinate(uint probeIndex, uint3 probeCounts) {
    uint3 coordinate;
    coordinate.x = probeIndex % probeCounts.x;
    coordinate.y = (probeIndex / probeCounts.x) % probeCounts.y;
    coordinate.z = probeIndex / (probeCounts.x * probeCounts.y);

    return coordinate;
}

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
    payload.Radiance = float3(0.0f);
    payload.Distance = maxRayDistance;

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
    payload.Radiance = float3(0.0f);
}

[shader("closesthit")]
void ClosestHit(inout DDGIRayPayload payload, BuiltInTriangleIntersectionAttributes attributes) {
    payload.Radiance = float3(1.0f);
    payload.Distance = RayTCurrent();
}

[shader("anyhit")]
void AnyHit() {

}