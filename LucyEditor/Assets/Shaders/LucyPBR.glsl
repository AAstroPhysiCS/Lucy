//type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_ID;
layout (location = 2) in vec2 a_TextureCoords;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_Bitangents;

layout (location = 0) out vec3 a_PosOut;
layout (location = 1) out vec2 a_TextureCoordsOut;
layout (location = 2) out vec3 a_NormalsOut;

layout (set = 0, binding = 0) uniform LucyCamera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
	vec4 u_CamPos;
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

void main() {
	a_PosOut = a_Pos;
	a_TextureCoordsOut = a_TextureCoords;
	a_NormalsOut = a_Normals;

	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_Normals;

layout (location = 0) out vec4 a_Color;

const uint ROUGHNESS_MASK = 0x00000001u;
const uint METALLIC_MASK = 0x00000002u;
const uint AO_MASK = 0x00000004u;

#define PI 3.1415926535897932384626433832795f

#define NULL_TEXTURE_SLOT -1
#define NUM_CASCADES 4

#define DEBUG_CASCADED_SHADOW_MAPS 1

const vec4 DEBUG_CASCADE_COLORS[NUM_CASCADES] = {
	vec4 (1.0f, 0.0f, 0.5f, 1.0f),
	vec4 (0.0f, 1.0f, 0.5f, 1.0f),
	vec4 (0.0f, 0.5f, 1.0f, 1.0f),
	vec4 (1.0f, 0.5f, 0.0f, 1.0f),
};

struct MaterialAttributes {
	float AlbedoSlot;
	float NormalSlot;
	float RoughnessSlot;
	float MetallicSlot;

	vec4 BaseDiffuseColor;
	float BaseRoughnessValue;
	float BaseMetallicValue;
	float BaseAOValue;
	float AOSlot;
};

struct DirectionalLight {
	vec3 Direction;
	float _padding0;
	vec3 Color;
	float _padding1;
	mat4 DirLightShadowMatrices[NUM_CASCADES];
	float DirLightShadowFarPlanes[NUM_CASCADES];
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

layout (set = 0, binding = 0) uniform LucyCamera {
	layout (offset = 128) vec4 u_CamPos;
};

layout (set = 0, binding = 1) uniform LucyLightningValues {
	DirectionalLight u_DirectionalLight;
};

layout (set = 0, binding = 2) readonly buffer LucyMaterialAttributes {
	MaterialAttributes b_MaterialAttributes[];
};

layout (set = 0, binding = 3) uniform samplerCube u_IrradianceMap;
layout (set = 0, binding = 4) uniform sampler2DArray u_ShadowMap;

layout (set = 1, binding = 0) uniform sampler2D u_Textures[];

void GetAttributeColor(int slot, vec4 baseColor, out vec4 outColor) {
	if (slot != NULL_TEXTURE_SLOT)									
		outColor = texture(u_Textures[slot], a_TextureCoords);
	else														
		outColor = baseColor;
}

void GetAttributeValue(int slot, float baseValue, out float outValue, uint mask) {
	if (slot != NULL_TEXTURE_SLOT) {
		switch (mask) {															
			case ROUGHNESS_MASK:												
				outValue = texture(u_Textures[slot], a_TextureCoords).r;		
				break;
			case METALLIC_MASK:												
				outValue = texture(u_Textures[slot], a_TextureCoords).g;		
				break;
			case AO_MASK:												
				outValue = texture(u_Textures[slot], a_TextureCoords).b;		
				break;
		}
	} else {											
		outValue = baseValue;
	}
}

/*
	Normal Distribution function (Distribution of the microfacets)
*/
float D_GGX(float dotNH, float roughness) {
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

/*
	Geometric Shadowing function (Microfacets shadowing)
*/
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

/*
	Fresnel function (Reflectance depending on angle of incidence)
*/
vec3 F_Schlick(float cosTheta, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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
vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedoColor, vec3 lightColor) {
	// Precalculation...	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		float D = D_GGX(dotNH, roughness); 
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		vec3 F = F_Schlick(dotNV, F0, roughness);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);	

		color += (kD * albedoColor / PI + spec) * dotNL * lightColor;
	}

	return color;
}

float isInShadow(vec3 modelWorldPos) {
	for (uint i = 0; i < NUM_CASCADES; i++) {
		uint cascadeIndex = i + 1;
		
		mat4 lightMatrix = u_DirectionalLight.DirLightShadowMatrices[cascadeIndex];

		vec4 posInShadowSpace = lightMatrix * vec4(modelWorldPos, 1.0f);
		posInShadowSpace = vec4(posInShadowSpace.xyz / posInShadowSpace.w, 1.0f);
		posInShadowSpace = posInShadowSpace * 0.5f + 0.5f;
		
		float farPlane = u_DirectionalLight.DirLightShadowFarPlanes[cascadeIndex];

		if (farPlane > abs(posInShadowSpace.z)) {
			float dirLightDepthValue = texture(u_ShadowMap, vec3(posInShadowSpace.xy, cascadeIndex)).r;
			return posInShadowSpace.z < dirLightDepthValue ? 0.0f : 1.0f;
		}
	}
}

void DebugCascadedShadowMaps(inout vec4 colorOut, vec3 modelWorldPos) {
	for (uint i = 0; i < NUM_CASCADES - 1; i++) {
		uint cascadeIndex = i + 1;

		mat4 lightMatrix = u_DirectionalLight.DirLightShadowMatrices[cascadeIndex];
		float farPlane = u_DirectionalLight.DirLightShadowFarPlanes[cascadeIndex];

		vec4 posInShadowSpace = lightMatrix * vec4(modelWorldPos, 1.0f);
		posInShadowSpace = vec4(posInShadowSpace.xyz / posInShadowSpace.w, 1.0f);
		posInShadowSpace = posInShadowSpace * 0.5f + 0.5f;

		debugPrintfEXT("%d, %f", cascadeIndex, farPlane);

		if (farPlane > abs(posInShadowSpace.z)) {
			colorOut *= DEBUG_CASCADE_COLORS[cascadeIndex];
			return;
		}
	}
}

void main() {

	float alpha = 1.0f;
	MaterialAttributes attributes = b_MaterialAttributes[int(u_MaterialID)];

	int albedoSlot			= int(attributes.AlbedoSlot);
	int normalSlot			= int(attributes.NormalSlot);
	int metallicSlot		= int(attributes.MetallicSlot);
	int roughnessSlot		= int(attributes.RoughnessSlot);
	int aoSlot				= int(attributes.AOSlot);

	vec4 albedoColor		= vec4(0.0f, 0.0f, 0.0f, alpha);
	float metallicValue		= 0.0f;
	float roughnessValue	= 0.0f;
	float aoValue			= 0.0f;

	GetAttributeColor(albedoSlot, attributes.BaseDiffuseColor, albedoColor);
	GetAttributeValue(metallicSlot, attributes.BaseMetallicValue, metallicValue, METALLIC_MASK);
	GetAttributeValue(roughnessSlot, attributes.BaseRoughnessValue, roughnessValue, ROUGHNESS_MASK);
	GetAttributeValue(aoSlot, attributes.BaseAOValue, aoValue, AO_MASK);

	alpha = albedoColor.w;
	if (alpha < 0.5f)
		discard;

	vec3 modelWorldPos = vec3(mat3(u_ModelMatrix) * a_Pos);
	vec3 modelNormalNormalized = normalize(mat3(u_ModelMatrix) * a_Normals);
	vec3 viewDirCamera = normalize(u_CamPos.xyz - modelWorldPos);

	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedoColor.xyz, metallicValue);

	vec3 specularContribution = vec3(0.0f);
	vec3 viewDirectionLight = normalize(u_DirectionalLight.Direction - modelWorldPos);
	specularContribution += BRDF(viewDirectionLight, viewDirCamera, modelNormalNormalized, F0, metallicValue, roughnessValue, albedoColor.xyz, u_DirectionalLight.Color);
	
	vec3 ambientContribution = texture(u_IrradianceMap, normalize(a_Normals)).rgb;

	vec3 outputColor = (ambientContribution + (1.0 - isInShadow(modelWorldPos)) * albedoColor.rgb + specularContribution) * albedoColor.rgb;

	// Gamma correction
	const float gamma = 2.2f;
	outputColor = pow(outputColor, vec3(1.0f / gamma));

	a_Color = vec4(outputColor, alpha);
	
	if (DEBUG_CASCADED_SHADOW_MAPS == 1)
		DebugCascadedShadowMaps(a_Color, modelWorldPos);
}