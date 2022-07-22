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
};

layout (location = 0) out LucyOutput r_Output;

layout (set = 0, binding = 0) uniform Camera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

void main() {
	r_Output.TextCoords = a_TextureCoords;

	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

#extension GL_EXT_nonuniform_qualifier : require

#define NULL_TEXTURE_SLOT -1

layout (location = 0) out vec4 a_Color;

struct LucyOutput {
	vec2 TextCoords;
};

struct MaterialAttributes {
	float AlbedoSlot;
	float NormalSlot;
	float RoughnessSlot;
	float MetallicSlot;

	vec4 BaseDiffuseColor;
	float Shininess;
	float Roughness;
	float Reflectivity;
	float AOSlot;
};

layout (location = 0) in LucyOutput r_Output;

layout (push_constant) uniform LocalPushConstant {
	layout (offset = 64) float u_MaterialID;
};

layout (set = 0, binding = 1) readonly buffer LucyMaterialAttributes {
	MaterialAttributes b_MaterialAttributes[];
};

layout (set = 1, binding = 0) uniform sampler2D u_Textures[];

void main() {
	float albedo = b_MaterialAttributes[int(u_MaterialID)].AlbedoSlot;

	if (albedo != NULL_TEXTURE_SLOT)
		a_Color = texture(u_Textures[int(albedo)], r_Output.TextCoords);
	else
		a_Color = vec4(albedo);
}