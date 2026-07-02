#ifndef LUCY_SAMPLING_UTILITIES_SLANG
#define LUCY_SAMPLING_UTILITIES_SLANG
#include "LucyCommons.slang"

// Van Der Corpus sequence
// @see http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float VdcSequence(uint bits) {
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N) {
	return float2(float(i)/float(N), VdcSequence(i));
}

float2 HammersleyOnADisk(uint i, uint N) {
    float r = sqrt((float(i) + 0.5f) / float(N));
    float phi = VdcSequence(i) * 2.0f * PI;
    return float2(r * cos(phi), r * sin(phi));
}

// Based on Karis 2014
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float DistributionGGX(float dotNH, float roughness) {
    float r2 = roughness * roughness;
    float r22 = r2 * r2;
    float dotNH2 = dotNH*dotNH;

    float nom = r22;
    float denom = (dotNH2 * (r22 - 1.0) + 1.0);

    return nom / (PI * denom * denom);
}

/*
    Geometric Shadowing function (Microfacets shadowing)
*/
float GeometrySchlickGGX(float dot, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // Disney remapping
    return dot / (dot * (1.0 - k) + k);
}

float GeometrySchlickGGXBRDF(float dot, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    float nom = dot;
    float denom = dot * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float GeometrySmithBRDF(float NdotV, float NdotL, float roughness) {
    float ggx2 = GeometrySchlickGGXBRDF(NdotV, roughness);
    float ggx1 = GeometrySchlickGGXBRDF(NdotL, roughness);

    return ggx1 * ggx2;
}

/*
	Fresnel function (Reflectance depending on angle of incidence)
*/
float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness) {
	return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 F_Schlick(float cosTheta, float3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

/*
	------------------ BRDF ------------------
	Cook-Torrance Microfacet BRDF (Bidirectional Reflectance Distribution Function)
	Specular Contribution
	BRDF consists of mainly 2 parts:
	1. Diffuse Part
	2. Specular Part

	Diffuse Part: Pd / PI
	Specular Part: F(v, h) * D(h) * G(l, v) / 4.0 * dot(N, L) * dot(N, V)

	We add both of them together and get the wanted BRDF

	V is the view direction
	L is the light direction
	n is the surface normal
	h is the halfway vector

	F(v, h) is the fresnel reflectance
	D(h) is the normal distribution function
	G(l, v) is the geometry term

	For these functions, we typically use an "approximation", since those are the fastest to compute
*/
float3 BRDF(float3 L, float3 V, float3 N, float3 F0, float metallic, float roughness, float3 albedoColor, float3 lightColor) {
    float3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);

    float D = DistributionGGX(dotNH, roughness);
    float G = GeometrySmith(dotNV, dotNL, roughness);
    float3 F = F_Schlick(dotHV, F0);

    float3 spec = (D * G * F) / max(4.0 * dotNL * dotNV, 1e-6);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    return (kD * albedoColor / PI + spec) * dotNL * lightColor;
}
#endif // LUCY_SAMPLING_UTILITIES_SLANG