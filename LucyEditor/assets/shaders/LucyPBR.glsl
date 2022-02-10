//type vertex
#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_ID;
layout (location = 2) in vec2 a_TextureCoords;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_BiTangents;

struct LucyRendererOutput {
	vec2 TextCoords;
};

layout(location = 0) out LucyRendererOutput r_Output;

layout(set = 0, binding = 0) uniform Camera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
	mat4 u_ModelMatrix;
};

void main() {
	r_Output.TextCoords = a_TextureCoords;

	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 460 core

#define NULL_TEXTURE_SLOT -1

layout (location = 0) out vec4 a_Color;

struct LucyRendererOutput {
	vec2 TextCoords;
};

layout(location = 0) in LucyRendererOutput r_Output;

layout(set = 0, binding = 1) uniform TextureSlots {
	int u_AlbedoTextureSlot;
	int u_NormalTextureSlot;
	int u_RoughnessTextureSlot;
	int u_MetallicTextureSlot;
	int u_AOTextureSlot;
};

layout (binding = 0) uniform sampler2D u_Textures[32];

void main() {
	if (u_AlbedoTextureSlot != NULL_TEXTURE_SLOT)
		a_Color = texture(u_Textures[u_AlbedoTextureSlot], r_Output.TextCoords);
	else
		a_Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}