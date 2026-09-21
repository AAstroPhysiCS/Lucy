#ifndef LUCY_PUNCTUAL_LIGHTS_SLANG
#define LUCY_PUNCTUAL_LIGHTS_SLANG

#include "LucyResource.slang"
#include "LucySamplingUtilities.slang"

float GetPunctualLightAttenuation(PunctualLight light, float distanceToLight) {
    float distanceSquared = max(distanceToLight * distanceToLight, 0.0001f);
    float attenuation = 1.0f / distanceSquared;

    float range = light.PositionAndRange.w;
    if (range > 0.0f) {
        float distanceRatio = distanceToLight / range;
        float distanceRatioSquared = distanceRatio * distanceRatio;
        float distanceRatioFourth = distanceRatioSquared * distanceRatioSquared;

        float rangeAttenuation = saturate(1.0f - distanceRatioFourth);
        rangeAttenuation *= rangeAttenuation;

        attenuation *= rangeAttenuation;
    }

    return attenuation;
}

float GetPunctualLightVisibility(RaytracingAccelerationStructure tlas, float3 positionWS, float3 surfaceNormalWS, float3 lightDirection, float lightDistance) {
    float3 offsetNormal = dot(surfaceNormalWS, lightDirection) >= 0.0f ? surfaceNormalWS : -surfaceNormalWS;
    RayDesc ray;
    ray.Origin = positionWS + offsetNormal * 0.02f;
    ray.Direction = lightDirection;
    ray.TMin = 0.001f;
    ray.TMax = max(lightDistance - 0.02f, 0.001f);

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(tlas, RAY_FLAG_NONE, 0xFF, ray);

    while (query.Proceed()) {
    }

    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0f : 0.0f;
}

float GetSpotLightAttenuation(PunctualLight light, float3 L) {
    uint type = uint(light.ColorAndType.w);
    if (type != PunctualLightType::Spot)
        return 1.0f;

    float3 lightDirection = normalize(light.DirectionAndIntensity.xyz);

    float cosTheta = dot(-L, lightDirection);
    float cosInner = light.ConeData.x;
    float cosOuter = light.ConeData.y;

    float attenuation = saturate((cosTheta - cosOuter) / max(cosInner - cosOuter, 0.0001f));
    return attenuation * attenuation;
}

struct PunctualLightSample {
    float3 L;
    float3 Radiance;
    float Distance;
};

PunctualLightSample SamplePunctualLight(PunctualLight light, float3 positionWS) {
    PunctualLightSample result;
    result.L = float3(0.0f);
    result.Radiance = float3(0.0f);
    result.Distance = 0.0f;

    float3 toLight = light.PositionAndRange.xyz - positionWS;

    float distanceSquared = dot(toLight, toLight);
    if (distanceSquared <= 0.0000001f)
        return result;

    float distanceToLight = sqrt(distanceSquared);
    float range = light.PositionAndRange.w;
    if (range > 0.0f && distanceToLight >= range)
        return result;

    float3 L = toLight / distanceToLight;

    float attenuation = GetPunctualLightAttenuation(light, distanceToLight);
    float spotAttenuation = GetSpotLightAttenuation(light, L);

    if (spotAttenuation <= 0.0f)
        return result;

    result.L = L;
    result.Distance = distanceToLight;
    result.Radiance = light.ColorAndType.xyz * light.DirectionAndIntensity.w * attenuation * spotAttenuation;

    return result;
}

float3 EvaluatePunctualLightsDDGI(RaytracingAccelerationStructure tlas, GPUScene* scene, float3 positionWS, float3 viewDirection, float3 normalWS, float3 F0, float metallic, float roughness, float3 albedo) {
    float3 result = float3(0.0f);
    uint lightCount = min(scene->PunctualLights.LightCount, MAX_PUNCTUAL_LIGHTS);

    for (uint lightIndex = 0; lightIndex < lightCount; lightIndex++) {
        PunctualLight light = scene->PunctualLights.Lights[lightIndex];
        PunctualLightSample sample = SamplePunctualLight(light, positionWS);

        if (sample.Distance <= 0.0f || dot(normalWS, sample.L) <= 0.0f)
            continue;

        float visibility = GetPunctualLightVisibility(tlas, positionWS, normalWS, sample.L, sample.Distance);
        if (visibility <= 0.0f)
            continue;

        result += BRDF(
            sample.L,
            viewDirection,
            normalWS,
            F0,
            metallic,
            roughness,
            albedo,
            sample.Radiance
        ) * visibility;
    }

    return result;
}

float3 EvaluatePunctualLights(GPUScene* scene, float3 positionWS, float3 viewDirection, float3 normalWS, float3 F0, float metallic, float roughness, float3 albedo, float clearcoatFactor, float clearcoatRoughness) {
    float3 result = float3(0.0f);
    uint lightCount = min(scene->PunctualLights.LightCount, MAX_PUNCTUAL_LIGHTS);

    for (uint lightIndex = 0; lightIndex < lightCount; lightIndex++) {
        PunctualLight light = scene->PunctualLights.Lights[lightIndex];
        PunctualLightSample sample = SamplePunctualLight(light, positionWS);

        if (sample.Distance <= 0.0f || dot(normalWS, sample.L) <= 0.0f)
            continue;

        result += LayeredBRDF(
            sample.L,
            viewDirection,
            normalWS,
            F0,
            metallic,
            roughness,
            albedo,
            sample.Radiance,
            clearcoatFactor,
            clearcoatRoughness
        );
    }

    return result;
}

#endif