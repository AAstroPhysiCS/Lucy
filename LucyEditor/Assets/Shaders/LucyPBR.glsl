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

layout (location = 3) out float a_Depth;

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

	a_Depth = (u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f)).z;
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

#include "LucySamplingUtilities"

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_Normals;

layout (location = 3) in float a_Depth;

layout (location = 0) out vec4 a_Color;

const uint ROUGHNESS_MASK = 0x00000001u;
const uint METALLIC_MASK = 0x00000002u;
const uint AO_MASK = 0x00000004u;

#define NULL_TEXTURE_SLOT -1
#define NUM_CASCADES 4

#define DEBUG_CASCADED_SHADOW_MAPS false

const vec4 DEBUG_CASCADE_COLORS[NUM_CASCADES] = {
	vec4 (1.0f, 0.0f, 0.5f, 1.0f),
	vec4 (0.0f, 1.0f, 0.5f, 1.0f),
	vec4 (0.3f, 0.5f, 1.0f, 1.0f),
	vec4 (1.0f, 0.5f, 0.5f, 1.0f),
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
	vec4 DirLightShadowCascadeSplits; //x = 0, y = 1, z = 2, w = 3
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

layout (set = 0, binding = 0) uniform LucyCamera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
	vec4 u_CamPos;
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

vec4 LinearizeDepth(in vec4 fragCoord, in mat4 projMat) {
	vec4 normalizedFrag = fragCoord * 2.0 - 1.0;
	vec4 unprojected = inverse(projMat) * normalizedFrag;
	return unprojected / unprojected.w;
}

float ChebyshevUpperBound(vec2 moments, float t) {
	const float minVariance = 0.002f;

	// One-tailed inequality valid if t > Moments.x
	if (t <= moments.x)
		return 1.0f;

	// Compute variance.
	float variance = moments.y - (moments.x * moments.x);
	variance = max(variance, minVariance);

	// Compute probabilistic upper bound.
	float d = t - moments.x;
	float p_max = variance / (variance + d * d);
	return p_max;
}

float LinearStep(float min, float max, float v) {
	return clamp((v - min) / (max - min), 0.0, 1.0);
} 

float ReduceLightBleeding(float p_max, float amount) {
	return LinearStep(amount, 1, p_max);
}

float VSMContribution(vec2 posInShadowSpace, float distanceToLight, uint cascadeIndex) {
	vec2 moments = texture(u_ShadowMap, vec3(posInShadowSpace, cascadeIndex)).xy;
	return ReduceLightBleeding(ChebyshevUpperBound(moments, distanceToLight), 0.9f);
}

const mat4 g_TexScaleBias = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float BlendBetweenCascades(vec4 modelWorldPos, uint cascadeIndex, float vsmContrPrev) {
	//uint nextCascadeIndex = min(cascadeIndex + 1, NUM_CASCADES - 1);
	//mat4 nextLightMatrix = u_DirectionalLight.DirLightShadowMatrices[nextCascadeIndex];
	//
	//vec4 nextPosInShadowSpace = g_TexScaleBias * nextLightMatrix * modelWorldPos;
    //nextPosInShadowSpace /= nextPosInShadowSpace.w;
	//
	//float vsmContrAfter = VSMContribution(nextPosInShadowSpace.xy, nextPosInShadowSpace.z, nextCascadeIndex);
	//
	//
	//smoothstep();
	//return blendFactor;
	return 0.0f;
}

float CalculateShadowFactor(vec4 modelWorldPos, uint cascadeIndex) {
    
    mat4 lightMatrix = u_DirectionalLight.DirLightShadowMatrices[cascadeIndex];
    vec4 posInShadowSpace = g_TexScaleBias * lightMatrix * modelWorldPos;
    posInShadowSpace /= posInShadowSpace.w;

    // Check if position is inside shadow map
    if(any(lessThan(posInShadowSpace.xy, vec2(0.0))) || 
       any(greaterThan(posInShadowSpace.xy, vec2(1.0))) ||
       posInShadowSpace.z < 0.0 || posInShadowSpace.z > 1.0) {
        return 1.0;
    }

	return VSMContribution(posInShadowSpace.xy, posInShadowSpace.z, cascadeIndex);
}

vec4 ShadowContribution(vec4 modelWorldPos) {
	//uint cascadeIndex = 0;
	//for (uint i = 0; i < NUM_CASCADES - 1; i++) {
	//	float cascadeSplit = u_DirectionalLight.DirLightShadowCascadeSplits[i];
	//	bool condition = (-a_Depth >= cascadeSplit);
	//	cascadeIndex = min(cascadeIndex, condition ? i + 1 : NUM_CASCADES);
	//}

	uint cascadeIndex = 0;
    for(uint i = 0; i < NUM_CASCADES - 1; i++) {
        if(-a_Depth > u_DirectionalLight.DirLightShadowCascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }

	if (DEBUG_CASCADED_SHADOW_MAPS)
		return DEBUG_CASCADE_COLORS[cascadeIndex];
	return vec4(CalculateShadowFactor(modelWorldPos, cascadeIndex));
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

	vec4 modelWorldPos = u_ModelMatrix * vec4(a_Pos, 1.0f);
	vec3 modelNormalNormalized = normalize(mat3(u_ModelMatrix) * a_Normals);
	vec4 viewDirCamera = normalize(u_CamPos - modelWorldPos);

	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedoColor.rgb, metallicValue);

	vec3 viewDirectionLight = normalize(modelWorldPos.xyz - u_DirectionalLight.Direction);
	vec3 specularContribution = BRDF(viewDirectionLight, viewDirCamera.xyz, modelNormalNormalized, F0, 
							metallicValue, roughnessValue, albedoColor.rgb, u_DirectionalLight.Color);

	vec3 ambientContribution = texture(u_IrradianceMap, normalize(a_Normals)).rgb;

	vec3 directLighting = specularContribution * ShadowContribution(modelWorldPos).r;
	vec3 ambient = ambientContribution * aoValue;
	vec3 outputColor = (directLighting + ambient) * albedoColor.rgb;

	// Gamma correction
	const float gamma = 2.2f;
	outputColor = pow(outputColor, vec3(1.0f / gamma));

	a_Color = vec4(outputColor, alpha);

	if (DEBUG_CASCADED_SHADOW_MAPS)
		a_Color = ShadowContribution(modelWorldPos);
}