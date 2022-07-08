//type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_ID;
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
};

//for fragment shader
struct TextureIndices {
	int AlbedoTextureIndex;
	int NormalTextureIndex;
	int RoughnessTextureIndex;
	int MetallicTextureIndex;
	int AOTextureIndex;
};

//Max Size: 128 bytes
layout(push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	TextureIndices u_TextureIndices; //for fragment shader
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

struct LucyRendererOutput {
	vec2 TextCoords;
};

layout(location = 0) in LucyRendererOutput r_Output;

//Max Size: 128 bytes
layout(push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix; //unused in fragment shader
	int u_MaterialIndex;
};

layout (set = 1, binding = 0) uniform sampler2D u_Textures[];

void main() {
	//if (u_TextureIndices.AlbedoTextureIndex != NULL_TEXTURE_SLOT)
		a_Color = texture(u_Textures[u_MaterialIndex], r_Output.TextCoords);
	//else
	//a_Color = vec4(u_MaterialIndex, 0.0f, 0.0f, 1.0f);
}