//type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec4 a_ID;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_BiTangents;

struct LucyOutput {
	vec2 TextCoords;
	vec3 ModelNormals;
	vec3 APos;
};

layout (location = 0) out LucyOutput r_Output;

layout (set = 0, binding = 0) uniform LucyCamera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
	vec3 u_CamPos;
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

void main() {
	r_Output.APos = a_Pos;
	r_Output.TextCoords = a_TextureCoords;
	r_Output.ModelNormals = a_Normals * mat3(u_ModelMatrix);

	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450
#extension GL_EXT_nonuniform_qualifier : require

const uint ROUGHNESS_MASK = 0x00000001u;
const uint METALLIC_MASK = 0x00000002u;
const uint AO_MASK = 0x00000004u;

const float PI = 3.14159265359f;

#define NULL_TEXTURE_SLOT -1

struct LucyOutput {
	vec2 TextCoords;
	vec3 ModelNormals;
	vec3 APos;
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
};

layout (location = 0) out vec4 a_Color;

layout (location = 0) in LucyOutput r_Output;

layout (push_constant) uniform LocalPushConstant {
	layout (offset = 64) float u_MaterialID;
};

layout (set = 0, binding = 0) uniform LucyCamera {
	layout (offset = 128) vec3 u_CamPos;
};

layout (set = 0, binding = 1) uniform LucyLightningValues {
	DirectionalLight u_DirectionalLight;
};

layout (set = 0, binding = 2) readonly buffer LucyMaterialAttributes {
	MaterialAttributes b_MaterialAttributes[];
};

layout (set = 1, binding = 0) uniform sampler2D u_Textures[];

void GetAttributeColor(int slot, vec4 baseColor, out vec4 outColor) {
	if (slot != NULL_TEXTURE_SLOT)									
		outColor = texture(u_Textures[slot], r_Output.TextCoords);
	else														
		outColor = baseColor;
}

void GetAttributeValue(int slot, float baseValue, out float outValue, uint mask) {
	if (slot != NULL_TEXTURE_SLOT) {
		switch (mask) {															
			case ROUGHNESS_MASK:												
				outValue = texture(u_Textures[slot], r_Output.TextCoords).r;		
				break;
			case METALLIC_MASK:												
				outValue = texture(u_Textures[slot], r_Output.TextCoords).g;		
				break;
			case AO_MASK:												
				outValue = texture(u_Textures[slot], r_Output.TextCoords).b;		
				break;
		}
	} else {											
		outValue = baseValue;
	}
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 materialColor, float metallic)
{
	vec3 F0 = mix(vec3(0.04), materialColor, metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;
}

// Specular BRDF composition --------------------------------------------
vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 lightColor, vec3 materialColor)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, materialColor, metallic);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

void main() {
	MaterialAttributes attributes = b_MaterialAttributes[int(u_MaterialID)];

	int albedoSlot			= int(attributes.AlbedoSlot);
	int normalSlot			= int(attributes.NormalSlot);
	int metallicSlot		= int(attributes.MetallicSlot);
	int roughnessSlot		= int(attributes.RoughnessSlot);
	int aoSlot				= int(attributes.AOSlot);

	vec4 albedoColor		= vec4(0.0f);
	float metallicValue		= 0.0f;
	float roughnessValue	= 0.0f;
	float aoValue			= 0.0f;

	GetAttributeColor(albedoSlot, attributes.BaseDiffuseColor, albedoColor);
	GetAttributeValue(metallicSlot, attributes.BaseMetallicValue, metallicValue, METALLIC_MASK);
	GetAttributeValue(roughnessSlot, attributes.BaseRoughnessValue, roughnessValue, ROUGHNESS_MASK);
	GetAttributeValue(aoSlot, attributes.BaseAOValue, aoValue, AO_MASK);

	vec3 modelNormalNormalized = normalize(r_Output.ModelNormals);
	vec3 viewDirCamera = normalize(u_CamPos - r_Output.APos);

	vec3 Lo = vec3(0.0f);

	vec3 viewDirectionLight = normalize(u_DirectionalLight.Direction - r_Output.APos);
	Lo += BRDF(viewDirectionLight, viewDirCamera, modelNormalNormalized, metallicValue, roughnessValue, u_DirectionalLight.Color, albedoColor.xyz);

	a_Color = albedoColor * 0.02f + vec4(Lo, 1.0f);
}