#ifndef LUCY_PUNCTUAL_LIGHTS_SLANG
#define LUCY_PUNCTUAL_LIGHTS_SLANG

#include "LucyResource.slang"
#include "LucySamplingUtilities.slang"

float GetPunctualLightAttenuation(PunctualLight light, float distanceToLight) {
    //exactly how assimp defines attenuation
    float attenuation = light.ColorAndConstantAttenuation.w + light.ConeAndAttenuation.z * distanceToLight + light.ConeAndAttenuation.w * distanceToLight * distanceToLight;
    return 1.0f / max(attenuation, 0.0001f);
}

float GetSpotLightAttenuation(PunctualLight light, float3 L) {
    uint type = uint(light.DirectionAndType.w);

    if (type != PunctualLightType::Spot)
        return 1.0f;

    float3 lightDirection = normalize(light.DirectionAndType.xyz);

    float cosTheta = dot(-L, lightDirection);
    float cosInner = light.ConeAndAttenuation.x;
    float cosOuter = light.ConeAndAttenuation.y;

    return smoothstep(cosOuter, cosInner, cosTheta);
}

float3 EvaluatePunctualLights(GPUScene* scene, float3 positionWS, float3 viewDirection, float3 normalWS, float3 F0, float metallic, float roughness, float3 albedo) {
    float3 result = float3(0.0f);
    uint lightCount = min(scene->PunctualLights.LightCount, MAX_PUNCTUAL_LIGHTS);

    for (uint lightIndex = 0; lightIndex < lightCount; lightIndex++) {
        PunctualLight light = scene->PunctualLights.Lights[lightIndex];
        float3 toLight = light.PositionAndRange.xyz - positionWS;

        float distanceSquared = dot(toLight, toLight);
        if (distanceSquared <= 0.0000001f)
            continue;

        float distanceToLight = sqrt(distanceSquared);
        float range = light.PositionAndRange.w;
        if (range > 0.0f && distanceToLight >= range)
            continue;

        float3 L = toLight / distanceToLight;
        float attenuation = GetPunctualLightAttenuation(light, distanceToLight);
        float spotAttenuation = GetSpotLightAttenuation(light, L);

        float3 radiance = light.ColorAndConstantAttenuation.xyz * attenuation * spotAttenuation;

        result += BRDF(
            L,
            viewDirection,
            normalWS,
            F0,
            metallic,
            roughness,
            albedo,
            radiance
        );
    }

    return result;
}

#endif