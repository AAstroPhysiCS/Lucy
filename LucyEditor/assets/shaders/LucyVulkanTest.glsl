//type vertex
#version 460 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_TextureCoords;

layout(set = 0, binding = 0) uniform Camera {
	mat4 u_ModelMatrix;
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

void main() {
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0);
}

//type fragment
#version 460 core

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.5f, 1.0f, 1.0f, 1.0f);
}