#include "LucyCommons.slang"

struct DDGIRayResult {
	float4 RadianceAndDistance;
};

float3 GetFibonacciDirection(uint rayIndex, uint rayCount) {
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
