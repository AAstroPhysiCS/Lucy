//type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_ID;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_BiTangents;

layout(set = 0, binding = 0) uniform LucyCamera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

layout (location = 0) out vec3 o_ID;

void main() {
	o_ID = a_ID;
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

layout (location = 0) out vec4 idColor;

layout (location = 0) in vec3 o_ID;

void main() {
	idColor = vec4(round(o_ID / 255 * 10e4) / 10e4, 1.0f);
	//we dont need to do anything with the depth
}